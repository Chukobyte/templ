#pragma once

#include "seika/defines.h"

// A single named layout within a theme.
typedef struct TplThemeLayout {
    const char* name;  // "base", "post", "home", ...
    const char* html;  // Full HTML template string (same syntax as on-disk templates)
} TplThemeLayout;

// A built-in or third-party theme.
//
// To register a custom theme from external code:
//
//   static const TplThemeLayout my_layouts[] = {
//       { "base", "<!DOCTYPE html>..." },
//       { "post", "<!DOCTYPE html>..." },
//   };
//   static const TplTheme my_theme = {
//       .name        = "mytheme",
//       .description = "My custom theme",
//       .layouts     = my_layouts,
//       .layout_count = 2,
//       .css         = "body { ... }",  // NULL if no CSS
//   };
//   tpl_theme_register(&my_theme);
//
typedef struct TplTheme {
    const char* name;
    const char* description;
    const TplThemeLayout* layouts;
    usize layout_count;
    const char* css;  // Written to dist/assets/theme.css; NULL = no CSS injected
} TplTheme;

// Register a theme.  Pointer must remain valid for the lifetime of the process.
void tpl_theme_register(const TplTheme* theme);

// Find a registered theme by name.  Returns NULL if not found.
const TplTheme* tpl_theme_find(const char* name);

// Find a layout within a theme.  Falls back to "base" if the specific layout
// is not defined.  Returns NULL if neither is found.
const char* tpl_theme_get_layout(const TplTheme* theme, const char* layout_name);

// Register all built-in themes.  Called once at engine startup.
void tpl_theme_register_builtins(void);
