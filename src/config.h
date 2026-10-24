#pragma once

#include <stdbool.h>
#include "seika/defines.h"

#define TPL_MAX_COLLECTIONS 16
#define TPL_MAX_NAME 128
#define TPL_MAX_PATH 512

typedef struct TplCollectionConfig {
    char name[TPL_MAX_NAME];
    char order_by[32];
    char order[8];    // "asc" or "desc"
    char layout[TPL_MAX_NAME];
} TplCollectionConfig;

typedef struct TplConfig {
    char site_title[TPL_MAX_NAME];
    char site_base_url[TPL_MAX_PATH];
    char path_content[TPL_MAX_PATH];
    char path_templates[TPL_MAX_PATH];
    char path_assets[TPL_MAX_PATH];
    char path_output[TPL_MAX_PATH];
    char default_layout[TPL_MAX_NAME];
    char theme[TPL_MAX_NAME];  // Built-in or registered theme name; empty = "default"
    TplCollectionConfig collections[TPL_MAX_COLLECTIONS];
    usize collection_count;
    bool valid;
} TplConfig;

bool tpl_config_load(const char* path, TplConfig* out);
