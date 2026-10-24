#pragma once

#include <stdbool.h>
#include "seika/defines.h"

#define TPL_FM_MAX_FIELDS 32
#define TPL_FM_KEY_MAX 64
#define TPL_FM_VAL_MAX 512

typedef struct TplFMField {
    char key[TPL_FM_KEY_MAX];
    char value[TPL_FM_VAL_MAX];
} TplFMField;

typedef struct TplFrontmatter {
    TplFMField fields[TPL_FM_MAX_FIELDS];
    usize field_count;
    const char* body_start; // pointer into source, after the closing ---
} TplFrontmatter;

// Parses YAML-subset frontmatter from source (which begins with "---\n").
// If no frontmatter found, body_start = source and field_count = 0.
bool tpl_frontmatter_parse(const char* source, TplFrontmatter* out);

const char* tpl_fm_get(const TplFrontmatter* fm, const char* key);
