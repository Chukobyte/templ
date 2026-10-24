#include "engine.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "seika/file_system.h"
#include "seika/string.h"
#include "seika/logger.h"

#include "platform.h"
#include "config.h"
#include "frontmatter.h"
#include "markdown.h"
#include "template.h"
#include "theme.h"

// ---- helpers ----------------------------------------------------------------

static bool is_collection_dir(const TplConfig* cfg, const char* rel_name) {
    for (usize i = 0; i < cfg->collection_count; i++) {
        if (strcmp(cfg->collections[i].name, rel_name) == 0) return true;
    }
    return false;
}

static const TplCollectionConfig* get_collection_cfg(const TplConfig* cfg, const char* name) {
    for (usize i = 0; i < cfg->collection_count; i++) {
        if (strcmp(cfg->collections[i].name, name) == 0) return &cfg->collections[i];
    }
    return NULL;
}

// Builds a URL slug from a filename stem.  "my post" → "my-post"
static void stem_to_slug(char* out, usize out_size, const char* stem) {
    usize i = 0, o = 0;
    while (stem[i] && o + 1 < out_size) {
        char c = stem[i++];
        if (c == ' ') c = '-';
        out[o++] = (char)(c);
    }
    out[o] = '\0';
}

// ---- collection loading -------------------------------------------------------

// Keep item fields small — the colls array is static (~2 MB in BSS).
#define TPL_COLL_ITEM_MAX  256
#define TPL_COLL_TITLE_MAX 128
#define TPL_COLL_ORDER_MAX  64

typedef struct {
    char md_path[TPL_PATH_MAX * 2];
    char url[TPL_PATH_MAX];
    char html_out[TPL_PATH_MAX * 2];
    char title[TPL_COLL_TITLE_MAX];
    char order_val[TPL_COLL_ORDER_MAX];
} TplCollItem;

typedef struct {
    char name[TPL_MAX_NAME];
    TplCollItem items[TPL_COLL_ITEM_MAX];
    usize count;
} TplCollLoaded;

static int s_sort_dir = 1; // 1 = asc, -1 = desc

static int coll_item_cmp(const void* a, const void* b) {
    const TplCollItem* ia = (const TplCollItem*)a;
    const TplCollItem* ib = (const TplCollItem*)b;
    char* ea; char* eb;
    long va = strtol(ia->order_val, &ea, 10);
    long vb = strtol(ib->order_val, &eb, 10);
    int cmp = (ea != ia->order_val && *ea == '\0' && eb != ib->order_val && *eb == '\0')
              ? ((va > vb) - (va < vb))
              : strcmp(ia->order_val, ib->order_val);
    return cmp * s_sort_dir;
}

static TplCollLoaded* find_loaded_coll(TplCollLoaded* colls, usize count, const char* name) {
    for (usize i = 0; i < count; i++) {
        if (strcmp(colls[i].name, name) == 0) return &colls[i];
    }
    return NULL;
}

// Computes a depth-based root prefix for root-relative paths.
// "/" → "."   "/about/" → ".."   "/a/b/" → "../.."
static void compute_root(const char* rel_url, char* out, usize out_size) {
    usize depth = 0;
    for (const char* p = rel_url; *p; p++) {
        if (*p == '/' && *(p + 1) != '\0' && *(p + 1) != '/') depth++;
    }
    if (depth == 0) { snprintf(out, out_size, "."); return; }
    out[0] = '\0';
    usize rem = out_size;
    for (usize i = 0; i < depth && rem > 3; i++) {
        if (i > 0 && rem > 2) { strncat(out, "/", rem - 1); rem--; }
        strncat(out, "..", rem - 1); rem -= 2;
    }
}

// Renders a single .md file and writes the HTML output.
// rel_url is the URL path (e.g. "/blog/my-post/").
static bool render_page(const char* md_path,
                        const char* rel_url,
                        const char* out_path,
                        const TplConfig* cfg,
                        const char* project_dir,
                        const char* collection_layout_hint,
                        const TplTheme* theme,
                        const TplCollLoaded* colls,
                        usize coll_count,
                        const char* prev_url,
                        const char* prev_title,
                        const char* next_url,
                        const char* next_title) {
    usize src_sz = 0;
    char* src = ska_fs_read_file_contents(md_path, &src_sz);
    if (!src) {
        ska_logger_error("templ: cannot read '%s'", md_path);
        return false;
    }

    TplFrontmatter fm;
    tpl_frontmatter_parse(src, &fm);

    // Determine layout
    const char* layout = tpl_fm_get(&fm, "layout");
    if (!layout || layout[0] == '\0') layout = collection_layout_hint;
    if (!layout || layout[0] == '\0') layout = cfg->default_layout;

    // Compute depth-based root prefix (".", "..", "../..", …)
    char root_buf[256];
    compute_root(rel_url, root_buf, sizeof(root_buf));

    // Render markdown body (with root prefix so root-relative img src works in file://)
    SkaString body;
    ska_string_init_empty(&body);
    usize body_len = src_sz - (usize)(fm.body_start - src);
    tpl_markdown_set_img_root(root_buf);
    tpl_markdown_to_html(fm.body_start, body_len, &body);
    tpl_markdown_set_img_root(NULL);

    // Page-local table of contents (## / ### headings), for templates that
    // render an in-page jump-link sidebar via {{#each toc}}.
    TplMarkdownTocEntry toc[TPL_MARKDOWN_TOC_MAX];
    usize toc_count = 0;
    tpl_markdown_extract_toc(fm.body_start, body_len, toc, TPL_MARKDOWN_TOC_MAX, &toc_count);

    // Build context
    TplContext* ctx = tpl_ctx_create();

    // Site vars
    tpl_ctx_set(ctx, "site.title",    cfg->site_title);
    tpl_ctx_set(ctx, "site.base_url", cfg->site_base_url);

    // Page vars from frontmatter
    for (usize i = 0; i < fm.field_count; i++) {
        tpl_ctx_set(ctx, fm.fields[i].key, fm.fields[i].value);
    }

    // Engine-injected vars
    tpl_ctx_set(ctx, "url",  rel_url);
    tpl_ctx_set(ctx, "root", root_buf);

    // Inject all collection data so {{#each collection}} works in every template
    {
        char key[128];
        char cnt[32];
        for (usize ci = 0; ci < coll_count; ci++) {
            const TplCollLoaded* cl = &colls[ci];
            snprintf(cnt, sizeof(cnt), "%zu", cl->count);
            snprintf(key, sizeof(key), "%s.count", cl->name);
            tpl_ctx_set(ctx, key, cnt);
            for (usize ii = 0; ii < cl->count; ii++) {
                snprintf(key, sizeof(key), "%s.%zu.title", cl->name, ii);
                tpl_ctx_set(ctx, key, cl->items[ii].title);
                snprintf(key, sizeof(key), "%s.%zu.url", cl->name, ii);
                tpl_ctx_set(ctx, key, cl->items[ii].url);
                // url_bare: no leading '/' — combined with {{root}} for file:// compatibility
                snprintf(key, sizeof(key), "%s.%zu.url_bare", cl->name, ii);
                {
                    const char* u = cl->items[ii].url;
                    tpl_ctx_set(ctx, key, u[0] == '/' ? u + 1 : u);
                }
            }
        }
    }

    // Inject page-local TOC as a pseudo-collection so {{#each toc}} works
    {
        char key[64];
        char cnt[32];
        snprintf(cnt, sizeof(cnt), "%zu", toc_count);
        tpl_ctx_set(ctx, "toc.count", cnt);
        for (usize ti = 0; ti < toc_count; ti++) {
            char lvl[8];
            snprintf(key, sizeof(key), "toc.%zu.text", ti);
            tpl_ctx_set(ctx, key, toc[ti].text);
            snprintf(key, sizeof(key), "toc.%zu.anchor", ti);
            tpl_ctx_set(ctx, key, toc[ti].slug);
            snprintf(key, sizeof(key), "toc.%zu.level", ti);
            snprintf(lvl, sizeof(lvl), "%d", toc[ti].level);
            tpl_ctx_set(ctx, key, lvl);
        }
    }

    // Inject prev/next for collection item pages
    if (prev_url) {
        tpl_ctx_set(ctx, "prev.url",      prev_url);
        tpl_ctx_set(ctx, "prev.url_bare", prev_url[0] == '/' ? prev_url + 1 : prev_url);
        tpl_ctx_set(ctx, "prev.title",    prev_title ? prev_title : "");
    }
    if (next_url) {
        tpl_ctx_set(ctx, "next.url",      next_url);
        tpl_ctx_set(ctx, "next.url_bare", next_url[0] == '/' ? next_url + 1 : next_url);
        tpl_ctx_set(ctx, "next.title",    next_title ? next_title : "");
    }

    // body SkaString may be heap-allocated (not null-terminated); copy it safely
    {
        usize blen = body.len;
        char* body_nul = malloc(blen + 1);
        memcpy(body_nul, ska_string_as_slice(&body), blen);
        body_nul[blen] = '\0';
        tpl_ctx_set(ctx, "body", body_nul);
        free(body_nul);
    }

    // Load layout template
    char layout_path[TPL_PATH_MAX * 2];
    snprintf(layout_path, sizeof(layout_path), "%s/%s/layouts/%s.html",
             project_dir, cfg->path_templates, layout);

    // Ensure output directory exists before any write attempt
    char out_dir[TPL_PATH_MAX * 2];
    tpl_path_dirname(out_dir, sizeof(out_dir), out_path);
    tpl_mkdir_p(out_dir);

    usize tmpl_sz = 0;
    char* tmpl_src = ska_fs_read_file_contents(layout_path, &tmpl_sz);
    if (!tmpl_src) {
        // Try the active theme before falling back to raw HTML
        const char* theme_html = tpl_theme_get_layout(theme, layout);
        if (theme_html) {
            tmpl_sz = strlen(theme_html);
            tmpl_src = malloc(tmpl_sz + 1);
            memcpy(tmpl_src, theme_html, tmpl_sz + 1);
        } else {
            ska_logger_warn("templ: layout not found '%s', writing raw HTML", layout_path);
            const char* body_slice = ska_string_as_slice(&body);
            usize blen = body.len;
            char* body_copy = malloc(blen + 1);
            memcpy(body_copy, body_slice, blen);
            body_copy[blen] = '\0';
            ska_fs_write_to_file(out_path, body_copy);
            free(body_copy);
            tpl_ctx_free(ctx);
            ska_string_deinit(&body);
            free(src);
            return true;
        }
    }

    char templates_dir[TPL_PATH_MAX * 2];
    snprintf(templates_dir, sizeof(templates_dir), "%s/%s", project_dir, cfg->path_templates);
    char layouts_dir[TPL_PATH_MAX * 2];
    snprintf(layouts_dir, sizeof(layouts_dir), "%s/%s/layouts", project_dir, cfg->path_templates);

    TplRenderOpts opts = {
        .templates_dir = templates_dir,
        .layouts_dir   = layouts_dir,
        .depth = 0,
    };

    SkaString html_out;
    ska_string_init_empty(&html_out);
    tpl_render(tmpl_src, tmpl_sz, ctx, &opts, &html_out);

    usize html_len = html_out.len;
    char* html_copy = malloc(html_len + 1);
    memcpy(html_copy, ska_string_as_slice(&html_out), html_len);
    html_copy[html_len] = '\0';
    ska_fs_write_to_file(out_path, html_copy);
    free(html_copy);

    free(tmpl_src);
    free(src);
    tpl_ctx_free(ctx);
    ska_string_deinit(&body);
    ska_string_deinit(&html_out);
    return true;
}

// Copies a directory tree from src to dst.
// Binary-safe file copy — skips nothing, handles null bytes and binary data.
static bool copy_file_binary(const char* src, const char* dst) {
    FILE* in  = fopen(src, "rb");
    if (!in) return false;
    FILE* out = fopen(dst, "wb");
    if (!out) { fclose(in); return false; }
    char buf[65536];
    usize n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        fwrite(buf, 1, n, out);
    }
    fclose(in);
    fclose(out);
    return true;
}

static void copy_dir(const char* src_dir, const char* dst_dir) {
    tpl_mkdir_p(dst_dir);

    TplFileList files = tpl_file_list_create();
    tpl_dir_list_recursive(src_dir, &files);

    for (usize i = 0; i < files.count; i++) {
        TplFileEntry* e = &files.entries[i];
        const char* rel = e->path + strlen(src_dir);
        if (rel[0] == '/') rel++;

        char dst_path[TPL_PATH_MAX * 2];
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst_dir, rel);

        if (e->is_dir) {
            tpl_mkdir_p(dst_path);
        } else {
            char dst_dir_path[TPL_PATH_MAX * 2];
            tpl_path_dirname(dst_dir_path, sizeof(dst_dir_path), dst_path);
            tpl_mkdir_p(dst_dir_path);
            copy_file_binary(e->path, dst_path);
        }
    }

    tpl_file_list_free(&files);
}

// ---- build ------------------------------------------------------------------

bool tpl_engine_build(const char* project_dir, const TplEngineOpts* opts) {
    tpl_theme_register_builtins();

    // 1. Load config
    char config_path[TPL_PATH_MAX * 2];
    const char* cfg_name = (opts && opts->config && opts->config[0]) ? opts->config : "tconf.json";
    snprintf(config_path, sizeof(config_path), "%s/%s", project_dir, cfg_name);

    TplConfig cfg;
    if (!tpl_config_load(config_path, &cfg)) return false;

    // Resolve active theme: CLI override → config → "default"
    const char* theme_name = (opts && opts->theme && opts->theme[0]) ? opts->theme
                           : (cfg.theme[0] ? cfg.theme : "default");
    const TplTheme* theme = tpl_theme_find(theme_name);
    if (!theme) {
        ska_logger_warn("templ: unknown theme '%s', falling back to 'default'", theme_name);
        theme = tpl_theme_find("default");
    }

    ska_logger_message("templ: building project at '%s' (theme: %s)", project_dir,
                       theme ? theme->name : "none");

    // 2. Set up output dir (clear and recreate)
    char out_dir[TPL_PATH_MAX * 2];
    snprintf(out_dir, sizeof(out_dir), "%s/%s", project_dir, cfg.path_output);

    if (ska_fs_does_dir_exist(out_dir)) {
        tpl_rmdir_recursive(out_dir);
    }
    tpl_mkdir_p(out_dir);

    // 3. Scan content directory
    char content_dir[TPL_PATH_MAX * 2];
    snprintf(content_dir, sizeof(content_dir), "%s/%s", project_dir, cfg.path_content);

    if (!ska_fs_does_dir_exist(content_dir)) {
        ska_logger_warn("templ: content dir not found: '%s'", content_dir);
        return true;
    }

    // 4. Pre-pass: load and sort every configured collection so templates can
    //    use {{#each collection}} and collection items get prev/next links.
    // Static to avoid a ~2 MB stack allocation; safe because tpl_engine_build
    // is single-threaded and we memset before use.
    static TplCollLoaded colls[TPL_MAX_COLLECTIONS];
    memset(colls, 0, sizeof(colls));
    usize coll_count = 0;

    for (usize ci = 0; ci < cfg.collection_count && coll_count < TPL_MAX_COLLECTIONS; ci++) {
        TplCollLoaded* cl = &colls[coll_count];
        snprintf(cl->name, sizeof(cl->name), "%s", cfg.collections[ci].name);

        char coll_dir[TPL_PATH_MAX * 2];
        snprintf(coll_dir, sizeof(coll_dir), "%s/%s", content_dir, cl->name);
        if (!ska_fs_does_dir_exist(coll_dir)) { coll_count++; continue; }

        TplFileList cfl = tpl_file_list_create();
        tpl_dir_list_recursive(coll_dir, &cfl);

        for (usize fi = 0; fi < cfl.count && cl->count < TPL_COLL_ITEM_MAX; fi++) {
            TplFileEntry* fe = &cfl.entries[fi];
            if (fe->is_dir) continue;
            if (strcmp(tpl_path_ext(fe->path), ".md") != 0) continue;

            const char* coll_rel = fe->path + strlen(coll_dir);
            if (coll_rel[0] == '/') coll_rel++;
            char item_stem[TPL_PATH_MAX];
            tpl_path_strip_ext(item_stem, sizeof(item_stem), coll_rel);
            char item_slug[TPL_PATH_MAX];
            stem_to_slug(item_slug, sizeof(item_slug), item_stem);
            if (strcmp(item_slug, "index") == 0) continue; // skip collection index pages

            usize src_sz = 0;
            char* item_src = ska_fs_read_file_contents(fe->path, &src_sz);
            if (!item_src) continue;
            TplFrontmatter fm;
            tpl_frontmatter_parse(item_src, &fm);

            TplCollItem* item = &cl->items[cl->count];
            snprintf(item->md_path,  sizeof(item->md_path),  "%s", fe->path);
            snprintf(item->url,      sizeof(item->url),      "/%s/%s/", cl->name, item_slug);
            snprintf(item->html_out, sizeof(item->html_out), "%s/%s/%s/index.html",
                     out_dir, cl->name, item_slug);
            const char* t = tpl_fm_get(&fm, "title");
            snprintf(item->title, TPL_COLL_TITLE_MAX, "%s", t ? t : item_slug);
            const char* ov = tpl_fm_get(&fm, cfg.collections[ci].order_by);
            snprintf(item->order_val, TPL_COLL_ORDER_MAX, "%s", ov ? ov : "");
            free(item_src);
            cl->count++;
        }
        tpl_file_list_free(&cfl);

        // Sort the collection
        s_sort_dir = (strcmp(cfg.collections[ci].order, "desc") == 0) ? -1 : 1;
        qsort(cl->items, cl->count, sizeof(TplCollItem), coll_item_cmp);
        coll_count++;
    }

    // 5. Render all .md files
    TplFileList top = tpl_file_list_create();
    tpl_dir_list_recursive(content_dir, &top);

    for (usize i = 0; i < top.count; i++) {
        TplFileEntry* e = &top.entries[i];
        if (e->is_dir) continue;

        const char* ext = tpl_path_ext(e->path);
        if (strcmp(ext, ".md") != 0) continue;

        // Relative path from content_dir
        const char* rel = e->path + strlen(content_dir);
        if (rel[0] == '/') rel++;

        // Extract slug from the path
        char stem[TPL_PATH_MAX];
        tpl_path_strip_ext(stem, sizeof(stem), rel);

        // Determine if it belongs to a collection (top-level dir name)
        char first_dir[TPL_PATH_MAX] = "";
        const char* slash = strchr(stem, '/');
        if (slash) {
            usize d = (usize)(slash - stem);
            if (d >= sizeof(first_dir)) d = sizeof(first_dir) - 1;
            memcpy(first_dir, stem, d);
            first_dir[d] = '\0';
        }

        bool in_collection = first_dir[0] && is_collection_dir(&cfg, first_dir);
        const TplCollectionConfig* col_cfg = in_collection ? get_collection_cfg(&cfg, first_dir) : NULL;

        // Compute slug
        char basename_stem[TPL_PATH_MAX];
        tpl_path_basename(basename_stem, sizeof(basename_stem), stem);
        char slug[TPL_PATH_MAX];
        stem_to_slug(slug, sizeof(slug), basename_stem);

        // Determine output path and URL
        char url[TPL_PATH_MAX];
        char html_out_path[TPL_PATH_MAX * 2];

        if (!in_collection && (strcmp(stem, "index") == 0 || strcmp(slug, "index") == 0)) {
            snprintf(url, sizeof(url), "/");
            snprintf(html_out_path, sizeof(html_out_path), "%s/index.html", out_dir);
        } else if (in_collection) {
            if (strcmp(slug, "index") == 0) {
                snprintf(url, sizeof(url), "/%s/", first_dir);
                snprintf(html_out_path, sizeof(html_out_path), "%s/%s/index.html",
                         out_dir, first_dir);
            } else {
                snprintf(url, sizeof(url), "/%s/%s/", first_dir, slug);
                snprintf(html_out_path, sizeof(html_out_path), "%s/%s/%s/index.html",
                         out_dir, first_dir, slug);
            }
        } else {
            snprintf(url, sizeof(url), "/%s/", slug);
            snprintf(html_out_path, sizeof(html_out_path), "%s/%s/index.html", out_dir, slug);
        }

        // Resolve prev/next for collection items
        const char *prev_url = NULL, *prev_title = NULL, *next_url = NULL, *next_title = NULL;
        if (in_collection && strcmp(slug, "index") != 0) {
            TplCollLoaded* cl = find_loaded_coll(colls, coll_count, first_dir);
            if (cl) {
                for (usize ii = 0; ii < cl->count; ii++) {
                    if (strcmp(cl->items[ii].url, url) == 0) {
                        if (ii > 0) {
                            prev_url   = cl->items[ii - 1].url;
                            prev_title = cl->items[ii - 1].title;
                        }
                        if (ii + 1 < cl->count) {
                            next_url   = cl->items[ii + 1].url;
                            next_title = cl->items[ii + 1].title;
                        }
                        break;
                    }
                }
            }
        }

        const char* col_layout_hint = col_cfg ? col_cfg->layout : NULL;
        if (!render_page(e->path, url, html_out_path, &cfg, project_dir, col_layout_hint, theme,
                         colls, coll_count, prev_url, prev_title, next_url, next_title)) {
            ska_logger_warn("templ: failed to render '%s'", e->path);
        } else {
            ska_logger_info("templ:   %s → %s", e->path, url);
        }
    }

    tpl_file_list_free(&top);

    // 5. Write theme CSS to dist/assets/theme.css (before user assets so they can override)
    if (theme && theme->css) {
        char theme_assets[TPL_PATH_MAX * 2];
        char theme_css_path[TPL_PATH_MAX * 2];
        snprintf(theme_assets,   sizeof(theme_assets),   "%s/assets", out_dir);
        snprintf(theme_css_path, sizeof(theme_css_path), "%s/assets/theme.css", out_dir);
        tpl_mkdir_p(theme_assets);
        ska_fs_write_to_file(theme_css_path, theme->css);
    }

    // 6. Copy user assets (may overwrite theme.css if the user provides one)
    char assets_src[TPL_PATH_MAX * 2];
    char assets_dst[TPL_PATH_MAX * 2];
    snprintf(assets_src, sizeof(assets_src), "%s/%s", project_dir, cfg.path_assets);
    snprintf(assets_dst, sizeof(assets_dst), "%s/assets", out_dir);

    if (ska_fs_does_dir_exist(assets_src)) {
        copy_dir(assets_src, assets_dst);
        ska_logger_info("templ: copied assets → %s", assets_dst);
    }

    ska_logger_message("templ: build complete → %s", out_dir);
    return true;
}
