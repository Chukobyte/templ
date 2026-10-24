#pragma once

#include <stdbool.h>
#include "seika/defines.h"
#include "seika/string.h"
#include "seika/data_structures/hash_map_string.h"

// Flat key→value context. Dot-path keys (e.g. "site.title") are stored literally.
typedef SkaStringHashMap TplContext;

TplContext* tpl_ctx_create(void);
void tpl_ctx_set(TplContext* ctx, const char* key, const char* value);
const char* tpl_ctx_get(const TplContext* ctx, const char* key);
void tpl_ctx_free(TplContext* ctx);

typedef struct TplRenderOpts {
    const char* templates_dir; // base dir for partials (templates/partials/)
    const char* layouts_dir;   // base dir for layouts  (templates/layouts/)
    int depth;                 // internal recursion guard, start at 0
} TplRenderOpts;

// Renders src into out using ctx for variable substitution.
// Handles: {{var}}, {{{var}}}, {{site.var}}, {{> partial}}, {{#if var}}...{{/if}}
// Handles layout inheritance: {{extends name}} + {{block slot}}...{{/block}}
bool tpl_render(const char* src, usize src_len,
                  const TplContext* ctx,
                  const TplRenderOpts* opts,
                  SkaString* out);
