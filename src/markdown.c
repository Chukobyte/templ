#include "markdown.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>

// Root prefix prepended to root-relative image/link src (e.g. "../.." for depth-2 pages).
// NULL = no rewriting. Set by tpl_markdown_set_img_root() before calling tpl_markdown_to_html().
static const char* s_img_root = NULL;

void tpl_markdown_set_img_root(const char* root) { s_img_root = root; }

// Nesting depth for tpl_markdown_to_html (blockquotes recurse into it) — used
// to reset per-document heading-slug dedup state only on the outermost call.
static int s_md_depth = 0;

#define MD_USED_SLUGS_MAX 256
static char s_used_slugs[MD_USED_SLUGS_MAX][TPL_MARKDOWN_TOC_TEXT_MAX];
static usize s_used_slug_count = 0;

void tpl_markdown_slugify(const char* text, usize len, char* out, usize out_size) {
    usize o = 0;
    bool prev_dash = true; // true so a leading run of non-alnum chars is dropped
    for (usize i = 0; i < len && o + 1 < out_size; i++) {
        unsigned char c = (unsigned char)text[i];
        if (isalnum(c)) {
            out[o++] = (char)tolower(c);
            prev_dash = false;
        } else if (!prev_dash) {
            out[o++] = '-';
            prev_dash = true;
        }
    }
    while (o > 0 && out[o - 1] == '-') o--; // trim trailing dash
    out[o] = '\0';
}

// Appends a numeric suffix ("-2", "-3", ...) to `base` if it collides with a
// slug already used earlier in the current top-level document.
static void dedup_slug(const char* base, char* out, usize out_size) {
    snprintf(out, out_size, "%s", base);
    int suffix = 1;
    while (true) {
        bool collision = false;
        for (usize i = 0; i < s_used_slug_count; i++) {
            if (strcmp(s_used_slugs[i], out) == 0) { collision = true; break; }
        }
        if (!collision) break;
        suffix++;
        snprintf(out, out_size, "%s-%d", base, suffix);
    }
    if (s_used_slug_count < MD_USED_SLUGS_MAX) {
        snprintf(s_used_slugs[s_used_slug_count++], TPL_MARKDOWN_TOC_TEXT_MAX, "%s", out);
    }
}

// ---- helpers ----------------------------------------------------------------

static void str_append(SkaString* s, const char* text, usize len) {
    ska_string_append(s, text, len);
}

static void str_append_cstr(SkaString* s, const char* text) {
    ska_string_append(s, text, strlen(text));
}

static void html_escape_append(SkaString* s, const char* text, usize len) {
    for (usize i = 0; i < len; i++) {
        switch (text[i]) {
            case '&':  str_append_cstr(s, "&amp;");  break;
            case '<':  str_append_cstr(s, "&lt;");   break;
            case '>':  str_append_cstr(s, "&gt;");   break;
            case '"':  str_append_cstr(s, "&quot;"); break;
            default:   str_append(s, text + i, 1);  break;
        }
    }
}

// ---- inline rendering -------------------------------------------------------

static void render_inline(const char* s, usize len, SkaString* out);

static usize try_inline_code(const char* s, usize len, usize i, SkaString* out) {
    if (s[i] != '`') return 0;
    usize end = i + 1;
    while (end < len && s[end] != '`') end++;
    if (end >= len) return 0;
    str_append_cstr(out, "<code>");
    html_escape_append(out, s + i + 1, end - i - 1);
    str_append_cstr(out, "</code>");
    return end - i + 1; // consumed chars including both backticks
}

static usize try_emphasis(const char* s, usize len, usize i, char delim, bool want_double, const char* open_tag, const char* close_tag, SkaString* out) {
    if (s[i] != delim) return 0;
    bool double_delim = (i + 1 < len && s[i + 1] == delim);
    if (want_double != double_delim) return 0;
    usize start = double_delim ? i + 2 : i + 1;
    const char* needle = double_delim ? (delim == '*' ? "**" : "__") : (delim == '*' ? "*" : "_");
    usize nlen = strlen(needle);
    const char* found = NULL;
    for (usize j = start; j + nlen - 1 < len; j++) {
        if (memcmp(s + j, needle, nlen) == 0) { found = s + j; break; }
    }
    if (!found) return 0;
    str_append_cstr(out, open_tag);
    render_inline(s + start, (usize)(found - s) - start, out);
    str_append_cstr(out, close_tag);
    return (usize)(found - s) - i + nlen;
}

static usize try_link_or_image(const char* s, usize len, usize i, SkaString* out) {
    bool is_img = (s[i] == '!');
    usize j = is_img ? i + 1 : i;
    if (j >= len || s[j] != '[') return 0;
    // find ]
    usize text_start = j + 1;
    usize text_end = text_start;
    while (text_end < len && s[text_end] != ']') text_end++;
    if (text_end >= len) return 0;
    if (text_end + 1 >= len || s[text_end + 1] != '(') return 0;
    usize href_start = text_end + 2;
    usize href_end = href_start;
    while (href_end < len && s[href_end] != ')') href_end++;
    if (href_end >= len) return 0;

    if (is_img) {
        str_append_cstr(out, "<img src=\"");
        if (s_img_root && s[href_start] == '/') {
            // rewrite /foo → {root}/foo so root-relative paths work in file://
            str_append_cstr(out, s_img_root);
            str_append_cstr(out, "/");
            str_append(out, s + href_start + 1, href_end - href_start - 1);
        } else {
            str_append(out, s + href_start, href_end - href_start);
        }
        str_append_cstr(out, "\" alt=\"");
        html_escape_append(out, s + text_start, text_end - text_start);
        str_append_cstr(out, "\">");
    } else {
        str_append_cstr(out, "<a href=\"");
        str_append(out, s + href_start, href_end - href_start);
        str_append_cstr(out, "\">");
        render_inline(s + text_start, text_end - text_start, out);
        str_append_cstr(out, "</a>");
    }
    return href_end - i + 1;
}

static void render_inline(const char* s, usize len, SkaString* out) {
    usize i = 0;
    while (i < len) {
        // hard line break: two spaces + \n
        if (s[i] == ' ' && i + 2 < len && s[i + 1] == ' ' && s[i + 2] == '\n') {
            str_append_cstr(out, "<br>\n");
            i += 3;
            continue;
        }
        if (s[i] == '`') {
            usize consumed = try_inline_code(s, len, i, out);
            if (consumed) { i += consumed; continue; }
        }
        if ((s[i] == '!' || s[i] == '[')) {
            usize consumed = try_link_or_image(s, len, i, out);
            if (consumed) { i += consumed; continue; }
        }
        if (s[i] == '*') {
            usize consumed = try_emphasis(s, len, i, '*', true,  "<strong>", "</strong>", out);
            if (!consumed) consumed = try_emphasis(s, len, i, '*', false, "<em>", "</em>", out);
            if (consumed) { i += consumed; continue; }
        }
        if (s[i] == '_') {
            usize consumed = try_emphasis(s, len, i, '_', true,  "<strong>", "</strong>", out);
            if (!consumed) consumed = try_emphasis(s, len, i, '_', false, "<em>", "</em>", out);
            if (consumed) { i += consumed; continue; }
        }
        html_escape_append(out, s + i, 1);
        i++;
    }
}

// ---- line helpers -----------------------------------------------------------

typedef struct { const char* data; usize len; usize raw_len; } Slice;

static Slice get_line(const char* s, usize total_len, usize pos) {
    Slice sl;
    sl.data = s + pos;
    usize end = pos;
    while (end < total_len && s[end] != '\n') end++;
    sl.raw_len = end - pos;
    sl.len = sl.raw_len;
    // strip trailing \r (content length only — raw_len still spans to the '\n')
    if (sl.len > 0 && sl.data[sl.len - 1] == '\r') sl.len--;
    return sl;
}

static bool is_blank(Slice sl) {
    for (usize i = 0; i < sl.len; i++) {
        if (!isspace((unsigned char)sl.data[i])) return false;
    }
    return true;
}

static int heading_level(Slice sl) {
    int lvl = 0;
    while (lvl < 6 && lvl < (int)sl.len && sl.data[lvl] == '#') lvl++;
    if (lvl == 0) return 0;
    if (lvl < (int)sl.len && sl.data[lvl] == ' ') return lvl;
    return 0;
}

static bool is_hr(Slice sl) {
    if (sl.len < 3) return false;
    char c = sl.data[0];
    if (c != '-' && c != '*' && c != '_') return false;
    usize cnt = 0;
    for (usize i = 0; i < sl.len; i++) {
        if (sl.data[i] == c) cnt++;
        else if (sl.data[i] != ' ') return false;
    }
    return cnt >= 3;
}

static bool is_ul(Slice sl) {
    if (sl.len < 2) return false;
    char c = sl.data[0];
    return (c == '-' || c == '*' || c == '+') && sl.data[1] == ' ';
}

static bool is_ol(Slice sl) {
    usize i = 0;
    while (i < sl.len && isdigit((unsigned char)sl.data[i])) i++;
    return i > 0 && i < sl.len && sl.data[i] == '.' && i + 1 < sl.len && sl.data[i + 1] == ' ';
}

static usize ol_prefix_len(Slice sl) {
    usize i = 0;
    while (i < sl.len && isdigit((unsigned char)sl.data[i])) i++;
    return i + 2; // digits + ". "
}

static bool is_blockquote(Slice sl) { return sl.len > 0 && sl.data[0] == '>'; }
static bool is_fence(Slice sl) { return sl.len >= 3 && sl.data[0] == '`' && sl.data[1] == '`' && sl.data[2] == '`'; }
static bool is_table_row(Slice sl) { return sl.len > 0 && sl.data[0] == '|'; }

static bool is_table_sep(Slice sl) {
    for (usize i = 0; i < sl.len; i++) {
        char c = sl.data[i];
        if (c != '|' && c != '-' && c != ':' && c != ' ') return false;
    }
    // Must contain at least one dash to be a real separator
    for (usize i = 0; i < sl.len; i++) {
        if (sl.data[i] == '-') return true;
    }
    return false;
}

#define TABLE_MAX_COLS 16
static usize parse_table_cells(Slice row, Slice* cells, usize max_cells) {
    usize count = 0, i = 0;
    if (i < row.len && row.data[i] == '|') i++;
    while (i < row.len && count < max_cells) {
        usize start = i;
        while (i < row.len && row.data[i] != '|') i++;
        Slice cell = { row.data + start, i - start };
        while (cell.len > 0 && cell.data[0] == ' ')           { cell.data++; cell.len--; }
        while (cell.len > 0 && cell.data[cell.len - 1] == ' ') cell.len--;
        cells[count++] = cell;
        if (i < row.len) i++;
    }
    if (count > 0 && cells[count - 1].len == 0) count--;
    return count;
}

// ---- block rendering --------------------------------------------------------

bool tpl_markdown_to_html(const char* md, usize md_len, SkaString* out) {
    if (s_md_depth == 0) s_used_slug_count = 0;
    s_md_depth++;

    usize pos = 0;

    while (pos <= md_len) {
        if (pos == md_len) break;

        Slice line = get_line(md, md_len, pos);
        usize next_pos = pos + line.raw_len + 1; // +1 for \n

        // Blank line — skip
        if (is_blank(line)) { pos = next_pos; continue; }

        // Fenced code block
        if (is_fence(line)) {
            // extract optional language
            char lang[64] = "";
            if (line.len > 3) {
                usize llen = line.len - 3;
                if (llen >= sizeof(lang)) llen = sizeof(lang) - 1;
                memcpy(lang, line.data + 3, llen);
                lang[llen] = '\0';
                // trim whitespace
                while (llen > 0 && isspace((unsigned char)lang[llen - 1])) lang[--llen] = '\0';
            }
            if (lang[0]) {
                char open[128];
                snprintf(open, sizeof(open), "<pre><code class=\"language-%s\">", lang);
                str_append_cstr(out, open);
            } else {
                str_append_cstr(out, "<pre><code>");
            }
            pos = next_pos;
            while (pos <= md_len) {
                if (pos == md_len) break;
                Slice cl = get_line(md, md_len, pos);
                pos += cl.raw_len + 1;
                if (is_fence(cl)) break;
                html_escape_append(out, cl.data, cl.len);
                str_append(out, "\n", 1);
            }
            str_append_cstr(out, "</code></pre>\n");
            continue;
        }

        // HR
        if (is_hr(line)) {
            str_append_cstr(out, "<hr>\n");
            pos = next_pos;
            continue;
        }

        // ATX heading
        int hlvl = heading_level(line);
        if (hlvl) {
            const char* htext = line.data + hlvl + 1;
            usize hlen = line.len - (usize)hlvl - 1;
            // strip trailing # markers
            while (hlen > 0 && htext[hlen - 1] == '#') hlen--;
            while (hlen > 0 && htext[hlen - 1] == ' ') hlen--;

            char base_slug[TPL_MARKDOWN_TOC_TEXT_MAX];
            tpl_markdown_slugify(htext, hlen, base_slug, sizeof(base_slug));
            char slug[TPL_MARKDOWN_TOC_TEXT_MAX];
            dedup_slug(base_slug, slug, sizeof(slug));

            char tag[192];
            snprintf(tag, sizeof(tag), "<h%d id=\"%s\">", hlvl, slug);
            str_append_cstr(out, tag);
            render_inline(htext, hlen, out);
            str_append_cstr(out, " <a class=\"heading-anchor\" href=\"#");
            str_append_cstr(out, slug);
            str_append_cstr(out, "\">#</a>");
            snprintf(tag, sizeof(tag), "</h%d>\n", hlvl);
            str_append_cstr(out, tag);
            pos = next_pos;
            continue;
        }

        // Blockquote — collect consecutive > lines
        if (is_blockquote(line)) {
            str_append_cstr(out, "<blockquote>\n");
            SkaString inner;
            ska_string_init_empty(&inner);
            while (pos <= md_len) {
                if (pos == md_len) break;
                Slice bl = get_line(md, md_len, pos);
                if (!is_blockquote(bl)) break;
                const char* bcontent = bl.data + 1;
                usize blen = bl.len > 1 ? bl.len - 1 : 0;
                if (blen > 0 && bcontent[0] == ' ') { bcontent++; blen--; }
                str_append(&inner, bcontent, blen);
                str_append(&inner, "\n", 1);
                pos += bl.raw_len + 1;
            }
            tpl_markdown_to_html(ska_string_as_slice(&inner), inner.len, out);
            ska_string_deinit(&inner);
            str_append_cstr(out, "</blockquote>\n");
            continue;
        }

        // Unordered list
        if (is_ul(line)) {
            str_append_cstr(out, "<ul>\n");
            while (pos <= md_len) {
                if (pos == md_len) break;
                Slice il = get_line(md, md_len, pos);
                if (!is_ul(il)) break;
                str_append_cstr(out, "<li>");
                render_inline(il.data + 2, il.len > 2 ? il.len - 2 : 0, out);
                str_append_cstr(out, "</li>\n");
                pos += il.raw_len + 1;
            }
            str_append_cstr(out, "</ul>\n");
            continue;
        }

        // Ordered list
        if (is_ol(line)) {
            str_append_cstr(out, "<ol>\n");
            while (pos <= md_len) {
                if (pos == md_len) break;
                Slice il = get_line(md, md_len, pos);
                if (!is_ol(il)) break;
                usize plen = ol_prefix_len(il);
                str_append_cstr(out, "<li>");
                render_inline(il.data + plen, il.len > plen ? il.len - plen : 0, out);
                str_append_cstr(out, "</li>\n");
                pos += il.raw_len + 1;
            }
            str_append_cstr(out, "</ol>\n");
            continue;
        }

        // GFM table: header row followed immediately by a separator row
        if (is_table_row(line)) {
            Slice sep = get_line(md, md_len, next_pos);
            if (is_table_row(sep) && is_table_sep(sep)) {
                Slice hdr_cells[TABLE_MAX_COLS];
                usize col_count = parse_table_cells(line, hdr_cells, TABLE_MAX_COLS);
                str_append_cstr(out, "<table>\n<thead>\n<tr>");
                for (usize c = 0; c < col_count; c++) {
                    str_append_cstr(out, "<th>");
                    render_inline(hdr_cells[c].data, hdr_cells[c].len, out);
                    str_append_cstr(out, "</th>");
                }
                str_append_cstr(out, "</tr>\n</thead>\n<tbody>\n");
                pos = next_pos + sep.raw_len + 1; // skip header + separator
                while (pos <= md_len) {
                    if (pos == md_len) break;
                    Slice row = get_line(md, md_len, pos);
                    if (!is_table_row(row)) break;
                    Slice cells[TABLE_MAX_COLS];
                    usize cell_count = parse_table_cells(row, cells, TABLE_MAX_COLS);
                    str_append_cstr(out, "<tr>");
                    for (usize c = 0; c < col_count; c++) {
                        str_append_cstr(out, "<td>");
                        if (c < cell_count) render_inline(cells[c].data, cells[c].len, out);
                        str_append_cstr(out, "</td>");
                    }
                    str_append_cstr(out, "</tr>\n");
                    pos += row.raw_len + 1;
                }
                str_append_cstr(out, "</tbody>\n</table>\n");
                continue;
            }
        }

        // Raw HTML block — line starts with '<'; pass through verbatim until blank line
        if (line.len > 0 && line.data[0] == '<') {
            while (pos <= md_len) {
                if (pos == md_len) break;
                Slice hl = get_line(md, md_len, pos);
                if (is_blank(hl)) break;
                str_append(out, hl.data, hl.len);
                str_append(out, "\n", 1);
                pos += hl.raw_len + 1;
            }
            continue;
        }

        // Paragraph — collect lines until blank or block element
        str_append_cstr(out, "<p>");
        bool first_line = true;
        while (pos <= md_len) {
            if (pos == md_len) break;
            Slice pl = get_line(md, md_len, pos);
            // A leading '|' only ends the paragraph if it wasn't already considered
            // (and rejected) as this block's own opening line above — otherwise a
            // bare '|' line with no separator row would loop forever without
            // advancing `pos`.
            if (is_blank(pl) || heading_level(pl) || is_hr(pl) || is_fence(pl) ||
                is_ul(pl) || is_ol(pl) || is_blockquote(pl) ||
                (!first_line && is_table_row(pl))) break;
            if (!first_line) str_append(out, "\n", 1);
            render_inline(pl.data, pl.len, out);
            first_line = false;
            pos += pl.raw_len + 1;
        }
        str_append_cstr(out, "</p>\n");
    }

    s_md_depth--;
    return true;
}

// ---- TOC extraction ----------------------------------------------------------

void tpl_markdown_extract_toc(const char* md, usize md_len,
                               TplMarkdownTocEntry* out, usize max,
                               usize* count) {
    // Local dedup state, independent of the render-time slug dedup in
    // tpl_markdown_to_html — this must not observe or affect other pages.
    char used[TPL_MARKDOWN_TOC_MAX][TPL_MARKDOWN_TOC_TEXT_MAX];
    usize used_count = 0;
    usize n = 0;
    usize pos = 0;
    bool in_fence = false;

    while (pos <= md_len && n < max) {
        if (pos == md_len) break;
        Slice line = get_line(md, md_len, pos);
        usize next_pos = pos + line.raw_len + 1;

        if (is_fence(line)) { in_fence = !in_fence; pos = next_pos; continue; }
        if (in_fence) { pos = next_pos; continue; }

        int hlvl = heading_level(line);
        if (hlvl == 2 || hlvl == 3) {
            const char* htext = line.data + hlvl + 1;
            usize hlen = line.len - (usize)hlvl - 1;
            while (hlen > 0 && htext[hlen - 1] == '#') hlen--;
            while (hlen > 0 && htext[hlen - 1] == ' ') hlen--;

            TplMarkdownTocEntry* entry = &out[n];
            entry->level = hlvl;
            usize tlen = hlen < TPL_MARKDOWN_TOC_TEXT_MAX - 1 ? hlen : TPL_MARKDOWN_TOC_TEXT_MAX - 1;
            memcpy(entry->text, htext, tlen);
            entry->text[tlen] = '\0';

            char base_slug[TPL_MARKDOWN_TOC_TEXT_MAX];
            tpl_markdown_slugify(htext, hlen, base_slug, sizeof(base_slug));
            snprintf(entry->slug, sizeof(entry->slug), "%s", base_slug);
            int suffix = 1;
            while (true) {
                bool collision = false;
                for (usize i = 0; i < used_count; i++) {
                    if (strcmp(used[i], entry->slug) == 0) { collision = true; break; }
                }
                if (!collision) break;
                suffix++;
                snprintf(entry->slug, sizeof(entry->slug), "%s-%d", base_slug, suffix);
            }
            if (used_count < TPL_MARKDOWN_TOC_MAX) {
                snprintf(used[used_count++], TPL_MARKDOWN_TOC_TEXT_MAX, "%s", entry->slug);
            }

            n++;
        }
        pos = next_pos;
    }

    *count = n;
}
