#include "theme.h"

/* ---- Layouts ------------------------------------------------------------- */

static const char layout_base[] =
"<!DOCTYPE html>\n"
"<html lang=\"en\">\n"
"<head>\n"
"  <meta charset=\"UTF-8\">\n"
"  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
"  <title>{{title}} - {{site.title}}</title>\n"
"  <link rel=\"stylesheet\" href=\"/assets/theme.css\">\n"
"</head>\n"
"<body>\n"
"  <nav class=\"site-nav\">\n"
"    <a class=\"brand\" href=\"/\">{{site.title}}</a>\n"
"    <div class=\"nav-links\">\n"
"      <a href=\"/\">Home</a>\n"
"      <a href=\"/about/\">About</a>\n"
"      <a href=\"/blog/\">Blog</a>\n"
"    </div>\n"
"  </nav>\n"
"  <main class=\"content\">\n"
"    {{{body}}}\n"
"  </main>\n"
"  <footer class=\"site-footer\">\n"
"    <p>Built with <strong>templ</strong></p>\n"
"  </footer>\n"
"</body>\n"
"</html>\n";

static const char layout_post[] =
"<!DOCTYPE html>\n"
"<html lang=\"en\">\n"
"<head>\n"
"  <meta charset=\"UTF-8\">\n"
"  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
"  <title>{{title}} - {{site.title}}</title>\n"
"  <link rel=\"stylesheet\" href=\"/assets/theme.css\">\n"
"</head>\n"
"<body>\n"
"  <nav class=\"site-nav\">\n"
"    <a class=\"brand\" href=\"/\">{{site.title}}</a>\n"
"    <div class=\"nav-links\">\n"
"      <a href=\"/\">Home</a>\n"
"      <a href=\"/about/\">About</a>\n"
"      <a href=\"/blog/\">Blog</a>\n"
"    </div>\n"
"  </nav>\n"
"  <main class=\"content\">\n"
"    <p class=\"post-date\">{{date}}</p>\n"
"    {{{body}}}\n"
"    <p class=\"back-link\"><a href=\"/blog/\">&larr; Back to blog</a></p>\n"
"  </main>\n"
"  <footer class=\"site-footer\">\n"
"    <p>Built with <strong>templ</strong></p>\n"
"  </footer>\n"
"</body>\n"
"</html>\n";

/* ---- CSS ----------------------------------------------------------------- */

static const char css[] =
"/* default theme */\n"
"*, *::before, *::after { box-sizing: border-box; }\n"
"body, h1, h2, h3, p, ul, ol, li { margin: 0; padding: 0; }\n"
":root {\n"
"  --font:    system-ui, -apple-system, BlinkMacSystemFont, sans-serif;\n"
"  --mono:    'Cascadia Code', 'Fira Code', 'Courier New', monospace;\n"
"  --text:    #1a1a2e;\n"
"  --text-2:  #555;\n"
"  --bg:      #fff;\n"
"  --bg-code: #f4f4f8;\n"
"  --border:  #ddd;\n"
"  --accent:  #4a6fa5;\n"
"  --max-w:   720px;\n"
"}\n"
"html { font-size: 16px; }\n"
"body { font-family: var(--font); color: var(--text); background: var(--bg); line-height: 1.75; }\n"
"a { color: var(--accent); }\n"
"img { max-width: 100%; display: block; }\n"
".site-nav {\n"
"  display: flex; align-items: center; gap: 1.5rem;\n"
"  padding: 0.75rem 1.5rem;\n"
"  background: var(--accent); flex-wrap: wrap;\n"
"}\n"
".site-nav a { color: #fff; text-decoration: none; font-weight: 500; }\n"
".site-nav a:hover { text-decoration: underline; }\n"
".site-nav .brand { font-size: 1.1rem; font-weight: 700; margin-right: auto; }\n"
".nav-links { display: flex; gap: 1.25rem; }\n"
".content { max-width: var(--max-w); margin: 2.5rem auto; padding: 0 1.25rem; }\n"
"h1 { font-size: clamp(1.75rem, 4vw, 2.25rem); line-height: 1.2; margin-bottom: 1rem; }\n"
"h2 { font-size: clamp(1.25rem, 3vw, 1.5rem); margin: 2rem 0 0.75rem; }\n"
"h3 { font-size: 1.125rem; margin: 1.5rem 0 0.5rem; }\n"
"p { margin-bottom: 1rem; }\n"
"ul, ol { padding-left: 1.5rem; margin-bottom: 1rem; }\n"
"li { margin-bottom: 0.25rem; }\n"
"code {\n"
"  font-family: var(--mono); font-size: 0.875em;\n"
"  background: var(--bg-code); padding: 0.1em 0.35em; border-radius: 4px;\n"
"}\n"
"pre {\n"
"  background: var(--bg-code); border: 1px solid var(--border);\n"
"  border-radius: 6px; padding: 1rem 1.25rem;\n"
"  overflow-x: auto; margin-bottom: 1.25rem; line-height: 1.6;\n"
"}\n"
"pre code { background: none; padding: 0; }\n"
"blockquote {\n"
"  border-left: 3px solid var(--accent); margin: 1.25rem 0;\n"
"  padding: 0.5rem 1.25rem; color: var(--text-2); font-style: italic;\n"
"}\n"
"hr { border: none; border-top: 1px solid var(--border); margin: 2rem 0; }\n"
".post-date { color: var(--text-2); font-size: 0.875rem; margin-bottom: 1.5rem; }\n"
".back-link { margin-top: 2.5rem; }\n"
".site-footer {\n"
"  text-align: center; padding: 2rem 1.25rem;\n"
"  color: var(--text-2); font-size: 0.875rem;\n"
"  border-top: 1px solid var(--border); margin-top: 4rem;\n"
"}\n"
"@media (max-width: 480px) {\n"
"  .content { margin-top: 1.5rem; }\n"
"  .site-nav { gap: 1rem; padding: 0.6rem 1rem; }\n"
"  .nav-links { gap: 1rem; }\n"
"}\n";

/* ---- Theme definition ---------------------------------------------------- */

static const TplThemeLayout layouts[] = {
    { "base", layout_base },
    { "post", layout_post },
};

const TplTheme tpl_builtin_theme_default = {
    .name         = "default",
    .description  = "Clean, minimal single-column layout",
    .layouts      = layouts,
    .layout_count = 2,
    .css          = css,
};
