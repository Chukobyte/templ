#include "template.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "seika/file_system.h"
#include "seika/logger.h"
#include "platform.h"

#define TPL_MAX_DEPTH 8
#define TPL_TAG_MAX 256
#define TPL_LAYOUT_NAME_MAX 128

TplContext* tpl_ctx_create(void) {
    return ska_string_hash_map_init(32);
}

void tpl_ctx_set(TplContext* ctx, const char* key, const char* value) {
    ska_string_hash_map_add(ctx, key, value, strlen(value) + 1);
}

const char* tpl_ctx_get(const TplContext* ctx, const char* key) {
    return (const char*)ska_string_hash_map_get((SkaStringHashMap*)ctx, key);
}

void tpl_ctx_free(TplContext* ctx) {
    ska_string_hash_map_deinit(ctx);
}

// ---- HTML escape ------------------------------------------------------------

static void html_escape_append(SkaString* out, const char* s, usize len) {
    for (usize i = 0; i < len; i++) {
        switch (s[i]) {
            case '&':  ska_string_append(out, "&amp;",  5); break;
            case '<':  ska_string_append(out, "&lt;",   4); break;
            case '>':  ska_string_append(out, "&gt;",   4); break;
            case '"':  ska_string_append(out, "&quot;", 6); break;
            default:   ska_string_append(out, s + i,    1); break;
        }
    }
}

// ---- Tag parsing ------------------------------------------------------------

// Finds the end of a {{ ... }} or {{{ ... }}} block starting at pos.
// Returns the position after the closing braces, or 0 if not found.
static usize find_tag_end(const char* src, usize src_len, usize pos, bool triple) {
    const char* needle = triple ? "}}}" : "}}";
    usize nlen = triple ? 3 : 2;
    while (pos + nlen <= src_len) {
        if (memcmp(src + pos, needle, nlen) == 0) return pos + nlen;
        pos++;
    }
    return 0;
}

// Copies tag content (trimmed) into buf.
static void extract_tag(const char* src, usize start, usize end, char* buf, usize buf_size) {
    // start/end are indices into the tag content (after {{ or {{{, before }} or }}})
    while (start < end && isspace((unsigned char)src[start])) start++;
    while (end > start && isspace((unsigned char)src[end - 1])) end--;
    usize len = end - start;
    if (len >= buf_size) len = buf_size - 1;
    memcpy(buf, src + start, len);
    buf[len] = '\0';
}

// ---- Block search -----------------------------------------------------------

// Finds the first occurrence of {{/tag_name}} at or after pos, returning the
// index of the {{ in the source. Accounts for nested {{#tag_name}} opens.
// Returns src_len if not found.
static usize find_closing_tag(const char* src, usize src_len, usize pos, const char* tag_name) {
    char open_pat[TPL_TAG_MAX + 8];
    char close_pat[TPL_TAG_MAX + 8];
    snprintf(open_pat,  sizeof(open_pat),  "{{#%s", tag_name);
    snprintf(close_pat, sizeof(close_pat), "{{/%s}}", tag_name);
    usize open_len  = strlen(open_pat);
    usize close_len = strlen(close_pat);
    int depth = 0;

    while (pos < src_len) {
        if (pos + open_len <= src_len && memcmp(src + pos, open_pat, open_len) == 0) {
            depth++;
            pos += open_len;
            continue;
        }
        if (pos + close_len <= src_len && memcmp(src + pos, close_pat, close_len) == 0) {
            if (depth == 0) return pos;
            depth--;
            pos += close_len;
            continue;
        }
        pos++;
    }
    return src_len;
}

// ---- Extends / block extraction ---------------------------------------------

// Returns pointer to content of the first {{block name}}...{{/block}} in src,
// setting *content_start and *content_len. Returns false if not found.
static bool extract_block(const char* src, usize src_len, const char* block_name,
                          usize* content_start, usize* content_len) {
    char open_tag[TPL_TAG_MAX + 16];
    snprintf(open_tag, sizeof(open_tag), "{{block %s}}", block_name);
    usize open_len = strlen(open_tag);

    for (usize i = 0; i + open_len <= src_len; i++) {
        if (memcmp(src + i, open_tag, open_len) == 0) {
            usize inner_start = i + open_len;
            usize close_pos = find_closing_tag(src, src_len, inner_start, "block");
            *content_start = inner_start;
            *content_len   = close_pos - inner_start;
            return true;
        }
    }
    return false;
}

// ---- Main renderer ----------------------------------------------------------

static bool render_internal(const char* src, usize src_len,
                            const TplContext* ctx,
                            const TplRenderOpts* opts,
                            SkaString* out);

static bool render_partial(const char* name, const TplContext* ctx,
                           const TplRenderOpts* opts, SkaString* out) {
    if (opts->depth >= TPL_MAX_DEPTH) {
        ska_logger_warn("templ: partial recursion limit hit for '%s'", name);
        return false;
    }
    char path[TPL_PATH_MAX * 2];
    snprintf(path, sizeof(path), "%s/partials/%s.html", opts->templates_dir, name);

    usize sz = 0;
    char* src = ska_fs_read_file_contents(path, &sz);
    if (!src) {
        ska_logger_warn("templ: partial not found: '%s'", path);
        return false;
    }
    TplRenderOpts inner = *opts;
    inner.depth++;
    bool ok = render_internal(src, sz, ctx, &inner, out);
    free(src);
    return ok;
}

static bool render_internal(const char* src, usize src_len,
                            const TplContext* ctx,
                            const TplRenderOpts* opts,
                            SkaString* out) {
    // --- extends pre-pass ---
    // Check for {{extends layout_name}} at the start (ignoring leading whitespace/newlines)
    {
        usize start = 0;
        while (start < src_len && (src[start] == ' ' || src[start] == '\t' ||
               src[start] == '\r' || src[start] == '\n')) start++;
        const char* extends_prefix = "{{extends ";
        usize ext_len = strlen(extends_prefix);
        if (start + ext_len < src_len && memcmp(src + start, extends_prefix, ext_len) == 0) {
            usize tag_start = start + ext_len;
            usize tag_end = tag_start;
            while (tag_end < src_len && src[tag_end] != '}') tag_end++;
            char layout_name[TPL_LAYOUT_NAME_MAX];
            usize nlen = tag_end - tag_start;
            if (nlen >= sizeof(layout_name)) nlen = sizeof(layout_name) - 1;
            memcpy(layout_name, src + tag_start, nlen);
            layout_name[nlen] = '\0';
            // trim
            while (nlen > 0 && isspace((unsigned char)layout_name[nlen - 1])) layout_name[--nlen] = '\0';

            // Load base layout
            char layout_path[TPL_PATH_MAX * 2];
            snprintf(layout_path, sizeof(layout_path), "%s/%s.html", opts->layouts_dir, layout_name);
            usize base_sz = 0;
            char* base_src = ska_fs_read_file_contents(layout_path, &base_sz);
            if (!base_src) {
                ska_logger_error("templ: layout not found: '%s'", layout_path);
                return false;
            }

            // Build merged template: for each {{block name}} in base, find matching
            // block in child and substitute it.
            SkaString merged;
            ska_string_init_empty(&merged);

            usize bi = 0;
            while (bi < base_sz) {
                const char* block_open = "{{block ";
                usize bo_len = strlen(block_open);
                // Find next {{block ...}} in base
                bool found_block = false;
                for (usize s = bi; s + bo_len <= base_sz; s++) {
                    if (memcmp(base_src + s, block_open, bo_len) == 0) {
                        // Flush up to here
                        ska_string_append(&merged, base_src + bi, s - bi);

                        // Extract block name
                        usize name_start = s + bo_len;
                        usize name_end = name_start;
                        while (name_end < base_sz && base_src[name_end] != '}') name_end++;
                        char bname[TPL_TAG_MAX];
                        usize bnlen = name_end - name_start;
                        if (bnlen >= sizeof(bname)) bnlen = sizeof(bname) - 1;
                        memcpy(bname, base_src + name_start, bnlen);
                        bname[bnlen] = '\0';
                        while (bnlen > 0 && isspace((unsigned char)bname[bnlen - 1])) bname[--bnlen] = '\0';

                        // Skip past the {{block name}} tag in base
                        usize base_inner_start = name_end + 2; // past }}
                        usize base_close = find_closing_tag(base_src, base_sz, base_inner_start, "block");

                        // Try to find matching block in child
                        usize child_cs = 0, child_cl = 0;
                        if (extract_block(src, src_len, bname, &child_cs, &child_cl)) {
                            ska_string_append(&merged, src + child_cs, child_cl);
                        } else {
                            // Use base default content
                            ska_string_append(&merged, base_src + base_inner_start, base_close - base_inner_start);
                        }

                        // Skip past {{/block}} in base
                        bi = base_close + strlen("{{/block}}");
                        s = bi - 1; // will be incremented by loop
                        found_block = true;
                        break;
                    }
                }
                if (!found_block) {
                    // No more blocks, flush rest of base
                    ska_string_append(&merged, base_src + bi, base_sz - bi);
                    bi = base_sz;
                }
            }

            free(base_src);

            TplRenderOpts inner = *opts;
            inner.depth++;
            bool ok = render_internal(ska_string_as_slice(&merged), merged.len, ctx, &inner, out);
            ska_string_deinit(&merged);
            return ok;
        }
    }

    // --- normal rendering ---
    usize i = 0;
    const char* flush_start = src;

    while (i < src_len) {
        if (src[i] != '{' || i + 1 >= src_len || src[i + 1] != '{') {
            i++;
            continue;
        }

        // Flush literal text up to this point
        ska_string_append(out, flush_start, (usize)(src + i) - (usize)flush_start);

        bool triple = (i + 2 < src_len && src[i + 2] == '{');
        usize content_start = i + (triple ? 3 : 2);
        usize tag_end = find_tag_end(src, src_len, content_start, triple);
        if (!tag_end) {
            // Unclosed tag — emit literally and move on
            ska_string_append(out, src + i, 2);
            i += 2;
            flush_start = src + i;
            continue;
        }

        usize content_end = tag_end - (triple ? 3 : 2);
        char tag[TPL_TAG_MAX];
        extract_tag(src, content_start, content_end, tag, sizeof(tag));

        i = tag_end;
        flush_start = src + i;

        // Dispatch
        if (tag[0] == '>') {
            // Partial: {{> name}}
            char pname[TPL_TAG_MAX];
            usize p = 1;
            while (p < sizeof(tag) && isspace((unsigned char)tag[p])) p++;
            snprintf(pname, sizeof(pname), "%s", tag + p);
            render_partial(pname, ctx, opts, out);

        } else if (strncmp(tag, "#if ", 4) == 0) {
            // Conditional: {{#if var}}...{{/if}}
            char var_name[TPL_TAG_MAX];
            snprintf(var_name, sizeof(var_name), "%s", tag + 4);
            usize close_pos = find_closing_tag(src, src_len, i, "if");
            usize block_content_len = close_pos - i;

            const char* val = tpl_ctx_get(ctx, var_name);
            bool truthy = val && val[0] != '\0';
            if (truthy) {
                TplRenderOpts inner = *opts;
                inner.depth++;
                render_internal(src + i, block_content_len, ctx, &inner, out);
            }
            i = close_pos + strlen("{{/if}}");
            flush_start = src + i;

        } else if (strncmp(tag, "#unless ", 8) == 0) {
            char var_name[TPL_TAG_MAX];
            snprintf(var_name, sizeof(var_name), "%s", tag + 8);
            usize close_pos = find_closing_tag(src, src_len, i, "unless");
            usize block_content_len = close_pos - i;

            const char* val = tpl_ctx_get(ctx, var_name);
            bool falsy = !val || val[0] == '\0';
            if (falsy) {
                TplRenderOpts inner = *opts;
                inner.depth++;
                render_internal(src + i, block_content_len, ctx, &inner, out);
            }
            i = close_pos + strlen("{{/unless}}");
            flush_start = src + i;

        } else if (strncmp(tag, "#each ", 6) == 0) {
            // Loop: {{#each collection_name}}...{{/each}}
            char coll_name[TPL_TAG_MAX];
            snprintf(coll_name, sizeof(coll_name), "%s", tag + 6);
            // Strip filter suffix (.all, .latest(3), .featured, etc.)
            char* filter = strchr(coll_name, '.');
            if (filter) *filter = '\0';

            usize close_pos = find_closing_tag(src, src_len, i, "each");
            usize block_len = close_pos - i;

            char count_key[TPL_TAG_MAX + 16];
            snprintf(count_key, sizeof(count_key), "%s.count", coll_name);
            const char* count_str = tpl_ctx_get(ctx, count_key);
            usize item_count = count_str ? (usize)atoi(count_str) : 0;

            for (usize ci = 0; ci < item_count; ci++) {
                TplContext* inner = tpl_ctx_create();
                // Add item-specific keys first (title, url, etc.) so they win
                // over identically-named keys in the parent context.
                // ska_string_hash_map_add is no-op for existing keys, so order matters.
                char prefix[TPL_TAG_MAX + 32];
                snprintf(prefix, sizeof(prefix), "%s.%zu.", coll_name, ci);
                usize prefix_len = strlen(prefix);
                SKA_STRING_HASH_MAP_FOR_EACH((SkaStringHashMap*)ctx, iit) {
                    if (strncmp(iit.key, prefix, prefix_len) == 0) {
                        tpl_ctx_set(inner, iit.key + prefix_len,
                                    (const char*)iit.value);
                    }
                }
                // Fill remaining parent keys that aren't already in inner
                SKA_STRING_HASH_MAP_FOR_EACH((SkaStringHashMap*)ctx, pit) {
                    if (!ska_string_hash_map_has(inner, pit.key)) {
                        tpl_ctx_set(inner, pit.key, (const char*)pit.value);
                    }
                }
                TplRenderOpts inner_opts = *opts;
                inner_opts.depth++;
                render_internal(src + i, block_len, inner, &inner_opts, out);
                tpl_ctx_free(inner);
            }

            i = close_pos + strlen("{{/each}}");
            flush_start = src + i;

        } else if (strncmp(tag, "/", 1) == 0 || strncmp(tag, "block ", 6) == 0 ||
                   strncmp(tag, "extends ", 8) == 0) {
            // These are handled structurally; skip if encountered here
            (void)0;

        } else {
            // Variable lookup
            const char* val = tpl_ctx_get(ctx, tag);
            if (val) {
                if (triple) {
                    ska_string_append(out, val, strlen(val));
                } else {
                    html_escape_append(out, val, strlen(val));
                }
            }
            // silently output nothing for unknown vars
        }
    }

    // Flush remaining text
    ska_string_append(out, flush_start, (usize)(src + src_len) - (usize)flush_start);
    return true;
}

bool tpl_render(const char* src, usize src_len,
                  const TplContext* ctx,
                  const TplRenderOpts* opts,
                  SkaString* out) {
    return render_internal(src, src_len, ctx, opts, out);
}
