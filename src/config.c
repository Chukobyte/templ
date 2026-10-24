#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cjson/cJSON.h"
#include "seika/file_system.h"
#include "seika/logger.h"

static void copy_str(char* dst, usize dst_size, const cJSON* item) {
    if (cJSON_IsString(item) && item->valuestring) {
        snprintf(dst, dst_size, "%s", item->valuestring);
    }
}

bool tpl_config_load(const char* path, TplConfig* out) {
    memset(out, 0, sizeof(*out));
    // Sensible defaults
    snprintf(out->path_content,   sizeof(out->path_content),   "content");
    snprintf(out->path_templates, sizeof(out->path_templates), "templates");
    snprintf(out->path_assets,    sizeof(out->path_assets),    "assets");
    snprintf(out->path_output,    sizeof(out->path_output),    "dist");
    snprintf(out->default_layout, sizeof(out->default_layout), "base");

    usize sz = 0;
    char* src = ska_fs_read_file_contents(path, &sz);
    if (!src) {
        ska_logger_error("templ: cannot read config '%s'", path);
        return false;
    }

    cJSON* root = cJSON_ParseWithLength(src, sz);
    free(src);
    if (!root) {
        ska_logger_error("templ: failed to parse config JSON: %s", cJSON_GetErrorPtr());
        return false;
    }

    cJSON* site = cJSON_GetObjectItemCaseSensitive(root, "site");
    if (site) {
        copy_str(out->site_title,    sizeof(out->site_title),    cJSON_GetObjectItemCaseSensitive(site, "title"));
        copy_str(out->site_base_url, sizeof(out->site_base_url), cJSON_GetObjectItemCaseSensitive(site, "base_url"));
    }

    cJSON* paths = cJSON_GetObjectItemCaseSensitive(root, "paths");
    if (paths) {
        copy_str(out->path_content,   sizeof(out->path_content),   cJSON_GetObjectItemCaseSensitive(paths, "content"));
        copy_str(out->path_templates, sizeof(out->path_templates), cJSON_GetObjectItemCaseSensitive(paths, "templates"));
        copy_str(out->path_assets,    sizeof(out->path_assets),    cJSON_GetObjectItemCaseSensitive(paths, "assets"));
        copy_str(out->path_output,    sizeof(out->path_output),    cJSON_GetObjectItemCaseSensitive(paths, "output"));
    }

    cJSON* defaults = cJSON_GetObjectItemCaseSensitive(root, "defaults");
    if (defaults) {
        copy_str(out->default_layout, sizeof(out->default_layout), cJSON_GetObjectItemCaseSensitive(defaults, "layout"));
    }

    copy_str(out->theme, sizeof(out->theme), cJSON_GetObjectItemCaseSensitive(root, "theme"));

    cJSON* collections = cJSON_GetObjectItemCaseSensitive(root, "collections");
    if (collections) {
        cJSON* col = NULL;
        cJSON_ArrayForEach(col, collections) {
            if (out->collection_count >= TPL_MAX_COLLECTIONS) break;
            TplCollectionConfig* c = &out->collections[out->collection_count++];
            memset(c, 0, sizeof(*c));
            snprintf(c->name, sizeof(c->name), "%s", col->string ? col->string : "");
            copy_str(c->order_by, sizeof(c->order_by), cJSON_GetObjectItemCaseSensitive(col, "order_by"));
            copy_str(c->order,    sizeof(c->order),    cJSON_GetObjectItemCaseSensitive(col, "order"));
            copy_str(c->layout,   sizeof(c->layout),   cJSON_GetObjectItemCaseSensitive(col, "layout"));
        }
    }

    cJSON_Delete(root);
    out->valid = true;
    return true;
}
