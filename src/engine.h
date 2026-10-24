#pragma once

#include <stdbool.h>

// Options that can override values from tconf.json.
// Pass NULL to use all config/default values.
typedef struct TplEngineOpts {
    const char* theme;   // Override config "theme"; NULL = use config value
    const char* config;  // Config filename; NULL = "tconf.json"
} TplEngineOpts;

// Runs a full build of the templ project rooted at project_dir.
// project_dir must contain tconf.json (or the filename set in opts->config).
bool tpl_engine_build(const char* project_dir, const TplEngineOpts* opts);
