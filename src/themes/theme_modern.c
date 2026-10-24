#include "theme.h"

/* ---- Shared nav/footer snippets (inlined per layout) --------------------- */

#define NAV \
"  <header class=\"site-nav\">\n" \
"    <div class=\"container\">\n" \
"      <a class=\"site-nav__brand\" href=\"/\">{{site.title}}</a>\n" \
"      <nav class=\"site-nav__links\">\n" \
"        <a href=\"/about/\">About</a>\n" \
"        <a href=\"/blog/\">Blog</a>\n" \
"      </nav>\n" \
"    </div>\n" \
"  </header>\n"

#define FOOTER \
"  <footer class=\"site-footer\">\n" \
"    <div class=\"container\">\n" \
"      <span class=\"site-footer__brand\">{{site.title}}</span>\n" \
"      <span class=\"site-footer__copy\">Built with templ</span>\n" \
"    </div>\n" \
"  </footer>\n"

#define HEAD(extra_title) \
"<!DOCTYPE html>\n" \
"<html lang=\"en\">\n" \
"<head>\n" \
"  <meta charset=\"UTF-8\">\n" \
"  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n" \
"  <title>" extra_title "</title>\n" \
"  <link rel=\"stylesheet\" href=\"/assets/theme.css\">\n" \
"</head>\n" \
"<body>\n"

/* ---- Layouts ------------------------------------------------------------- */

static const char layout_home[] =
HEAD("{{site.title}}")
NAV
"  <section class=\"hero\">\n"
"    <div class=\"container\">\n"
"      <span class=\"hero__label\">Static Site Generator</span>\n"
"      <h1 class=\"hero__title\">{{site.title}}</h1>\n"
"      <p class=\"hero__subtitle\">{{description}}</p>\n"
"      <div class=\"hero__actions\">\n"
"        <a href=\"/blog/\" class=\"btn btn--primary\">Read the blog &rarr;</a>\n"
"        <a href=\"/about/\" class=\"btn btn--ghost\">Learn more</a>\n"
"      </div>\n"
"    </div>\n"
"  </section>\n"
"  <main class=\"page-body\">\n"
"    <div class=\"container\">\n"
"      <div class=\"prose\">{{{body}}}</div>\n"
"    </div>\n"
"  </main>\n"
FOOTER
"</body>\n"
"</html>\n";

static const char layout_base[] =
HEAD("{{title}} - {{site.title}}")
NAV
"  <div class=\"page-header\">\n"
"    <div class=\"container container--narrow\">\n"
"      <h1 class=\"page-header__title\">{{title}}</h1>\n"
"      <p class=\"page-header__subtitle\">{{description}}</p>\n"
"    </div>\n"
"  </div>\n"
"  <main class=\"page-body\">\n"
"    <div class=\"container container--narrow\">\n"
"      <div class=\"prose\">{{{body}}}</div>\n"
"    </div>\n"
"  </main>\n"
FOOTER
"</body>\n"
"</html>\n";

static const char layout_post[] =
HEAD("{{title}} - {{site.title}}")
NAV
"  <main class=\"page-body\">\n"
"    <div class=\"container container--narrow\">\n"
"      <header class=\"post-header\">\n"
"        <div class=\"post-meta\">\n"
"          <span class=\"post-meta__tag\">Blog</span>\n"
"          <span>{{date}}</span>\n"
"        </div>\n"
"        <h1 class=\"post-title\">{{title}}</h1>\n"
"        <p class=\"post-desc\">{{description}}</p>\n"
"      </header>\n"
"      <article class=\"prose\">{{{body}}}</article>\n"
"      <footer class=\"post-footer\">\n"
"        <a href=\"/blog/\">&larr; All posts</a>\n"
"      </footer>\n"
"    </div>\n"
"  </main>\n"
FOOTER
"</body>\n"
"</html>\n";

#undef HEAD
#undef NAV
#undef FOOTER

/* ---- CSS ----------------------------------------------------------------- */

static const char css[] =
"/* modern theme */\n"
"*, *::before, *::after { box-sizing: border-box; }\n"
"body, h1, h2, h3, h4, p, ul, ol, li, blockquote { margin: 0; padding: 0; }\n"
"ul, ol { list-style: none; }\n"
"img { max-width: 100%; display: block; }\n"
":root {\n"
"  --c-bg:        #ffffff;\n"
"  --c-surface:   #f8f9fc;\n"
"  --c-border:    #e4e8f0;\n"
"  --c-text:      #111827;\n"
"  --c-text-2:    #4b5563;\n"
"  --c-text-3:    #9ca3af;\n"
"  --c-accent:    #4f46e5;\n"
"  --c-accent-h:  #4338ca;\n"
"  --c-accent-bg: #eef2ff;\n"
"  --c-code-bg:   #f1f5f9;\n"
"  --font: -apple-system, BlinkMacSystemFont, 'Segoe UI', system-ui, sans-serif;\n"
"  --mono: 'Cascadia Code', 'Fira Code', ui-monospace, 'Courier New', monospace;\n"
"  --t-sm:   0.875rem;\n"
"  --t-base: 1rem;\n"
"  --t-lg:   clamp(1.0625rem, 1.5vw, 1.125rem);\n"
"  --t-xl:   clamp(1.25rem,   2.5vw, 1.5rem);\n"
"  --t-2xl:  clamp(1.5rem,    3.5vw, 2rem);\n"
"  --t-3xl:  clamp(2rem,      5vw,   2.75rem);\n"
"  --t-hero: clamp(2.5rem,    7vw,   4rem);\n"
"  --s-2: 0.5rem; --s-3: 0.75rem; --s-4: 1rem;\n"
"  --s-6: 1.5rem; --s-8: 2rem;   --s-12: 3rem;\n"
"  --s-16: 4rem;  --s-24: 6rem;\n"
"  --w-content: 720px;\n"
"  --w-wide:    1100px;\n"
"  --nav-h:     64px;\n"
"  --radius:    8px;\n"
"  --radius-lg: 16px;\n"
"  --shadow-sm: 0 1px 3px rgba(0,0,0,.06), 0 1px 2px rgba(0,0,0,.04);\n"
"  --shadow:    0 4px 12px rgba(0,0,0,.08), 0 2px 6px rgba(0,0,0,.05);\n"
"}\n"
"html { font-size: 16px; scroll-behavior: smooth; -webkit-text-size-adjust: 100%; }\n"
"body { font-family: var(--font); font-size: var(--t-base); line-height: 1.75;\n"
"       color: var(--c-text); background: var(--c-bg); }\n"
"a { color: var(--c-accent); text-decoration: none; }\n"
"a:hover { text-decoration: underline; }\n"
".container { width: 100%; max-width: var(--w-wide); margin-inline: auto; padding-inline: var(--s-6); }\n"
".container--narrow { max-width: var(--w-content); }\n"
"/* Nav */\n"
".site-nav {\n"
"  position: sticky; top: 0; z-index: 100; height: var(--nav-h);\n"
"  background: rgba(255,255,255,0.9);\n"
"  backdrop-filter: blur(12px); -webkit-backdrop-filter: blur(12px);\n"
"  border-bottom: 1px solid var(--c-border);\n"
"}\n"
".site-nav .container { height: 100%; display: flex; align-items: center; gap: var(--s-6); }\n"
".site-nav__brand { font-size: var(--t-lg); font-weight: 700; color: var(--c-text);\n"
"                   text-decoration: none; letter-spacing: -0.01em; margin-right: auto; }\n"
".site-nav__brand:hover { color: var(--c-accent); text-decoration: none; }\n"
".site-nav__links { display: flex; align-items: center; gap: var(--s-6); }\n"
".site-nav__links a { font-size: var(--t-sm); font-weight: 500; color: var(--c-text-2); transition: color 0.15s; }\n"
".site-nav__links a:hover { color: var(--c-text); text-decoration: none; }\n"
"/* Hero */\n"
".hero {\n"
"  padding: var(--s-24) 0 var(--s-16);\n"
"  background: linear-gradient(135deg, var(--c-surface) 0%, var(--c-accent-bg) 100%);\n"
"  border-bottom: 1px solid var(--c-border);\n"
"}\n"
".hero__label { display: inline-block; font-size: var(--t-sm); font-weight: 600;\n"
"               letter-spacing: 0.08em; text-transform: uppercase;\n"
"               color: var(--c-accent); margin-bottom: var(--s-4); }\n"
".hero__title { font-size: var(--t-hero); font-weight: 800; line-height: 1.1;\n"
"               letter-spacing: -0.03em; color: var(--c-text); margin-bottom: var(--s-6); }\n"
".hero__subtitle { font-size: var(--t-xl); color: var(--c-text-2); max-width: 560px;\n"
"                  line-height: 1.5; margin-bottom: var(--s-8); }\n"
".hero__actions { display: flex; gap: var(--s-3); flex-wrap: wrap; }\n"
"/* Buttons */\n"
".btn { display: inline-flex; align-items: center; gap: var(--s-2);\n"
"       padding: var(--s-3) var(--s-6); border-radius: var(--radius);\n"
"       font-family: var(--font); font-size: var(--t-sm); font-weight: 600;\n"
"       cursor: pointer; transition: background 0.15s, transform 0.1s, box-shadow 0.15s;\n"
"       text-decoration: none; }\n"
".btn:hover { text-decoration: none; }\n"
".btn--primary { background: var(--c-accent); color: #fff; }\n"
".btn--primary:hover { background: var(--c-accent-h); transform: translateY(-1px); box-shadow: var(--shadow-sm); }\n"
".btn--ghost { background: transparent; color: var(--c-text-2); border: 1px solid var(--c-border); }\n"
".btn--ghost:hover { background: var(--c-surface); color: var(--c-text); }\n"
"/* Page sections */\n"
".page-body { padding: var(--s-16) 0 var(--s-24); }\n"
".page-header { padding: var(--s-16) 0 var(--s-8); border-bottom: 1px solid var(--c-border); margin-bottom: var(--s-12); }\n"
".page-header__title { font-size: var(--t-3xl); font-weight: 800; letter-spacing: -0.03em; margin-bottom: var(--s-3); }\n"
".page-header__subtitle { font-size: var(--t-lg); color: var(--c-text-2); }\n"
"/* Post layout */\n"
".post-header { padding-bottom: var(--s-8); border-bottom: 1px solid var(--c-border); margin-bottom: var(--s-8); }\n"
".post-meta { display: flex; align-items: center; gap: var(--s-4);\n"
"             font-size: var(--t-sm); color: var(--c-text-3); margin-bottom: var(--s-4); }\n"
".post-meta__tag { display: inline-block; padding: 2px var(--s-2);\n"
"                  background: var(--c-accent-bg); color: var(--c-accent);\n"
"                  border-radius: 4px; font-size: 0.75rem; font-weight: 600;\n"
"                  letter-spacing: 0.04em; text-transform: uppercase; }\n"
".post-title { font-size: var(--t-3xl); font-weight: 800; line-height: 1.2;\n"
"              letter-spacing: -0.03em; margin-bottom: var(--s-4); }\n"
".post-desc { font-size: var(--t-lg); color: var(--c-text-2); line-height: 1.6; }\n"
".post-footer { margin-top: var(--s-16); padding-top: var(--s-8); border-top: 1px solid var(--c-border); }\n"
".post-footer a { font-size: var(--t-sm); font-weight: 600; }\n"
"/* Prose */\n"
".prose { font-size: var(--t-lg); line-height: 1.8; color: var(--c-text); }\n"
".prose h1 { font-size: var(--t-3xl); font-weight: 800; letter-spacing: -0.03em; margin: var(--s-12) 0 var(--s-4); }\n"
".prose h2 { font-size: var(--t-2xl); font-weight: 700; letter-spacing: -0.02em;\n"
"            margin: var(--s-12) 0 var(--s-4); padding-top: var(--s-4); border-top: 1px solid var(--c-border); }\n"
".prose h3 { font-size: var(--t-xl); font-weight: 600; margin: var(--s-8) 0 var(--s-3); }\n"
".prose p { margin-bottom: var(--s-6); }\n"
".prose a { font-weight: 500; }\n"
".prose ul, .prose ol { list-style: revert; padding-left: var(--s-6); margin-bottom: var(--s-6); }\n"
".prose li { margin-bottom: var(--s-2); }\n"
".prose code { font-family: var(--mono); font-size: 0.875em; background: var(--c-code-bg);\n"
"              color: var(--c-text); padding: 0.1em 0.4em; border-radius: 4px; border: 1px solid var(--c-border); }\n"
".prose pre { background: var(--c-code-bg); border: 1px solid var(--c-border);\n"
"             border-radius: var(--radius); padding: var(--s-4) var(--s-6);\n"
"             overflow-x: auto; margin-bottom: var(--s-6); line-height: 1.6; }\n"
".prose pre code { background: none; border: none; padding: 0; font-size: var(--t-sm); }\n"
".prose blockquote { border-left: 3px solid var(--c-accent); margin: var(--s-6) 0;\n"
"                    padding: var(--s-3) var(--s-6); background: var(--c-accent-bg);\n"
"                    border-radius: 0 var(--radius) var(--radius) 0;\n"
"                    color: var(--c-text-2); font-style: italic; }\n"
".prose blockquote p { margin-bottom: 0; }\n"
".prose hr { border: none; border-top: 1px solid var(--c-border); margin: var(--s-12) 0; }\n"
".prose img { border-radius: var(--radius); margin: var(--s-6) 0; box-shadow: var(--shadow-sm); }\n"
".prose table { width: 100%; border-collapse: collapse; font-size: var(--t-sm); margin-bottom: var(--s-6); }\n"
".prose th, .prose td { text-align: left; padding: var(--s-2) var(--s-4); border-bottom: 1px solid var(--c-border); }\n"
".prose th { font-weight: 600; background: var(--c-surface); }\n"
".prose tr:last-child td { border-bottom: none; }\n"
"/* Footer */\n"
".site-footer { border-top: 1px solid var(--c-border); padding: var(--s-12) 0; background: var(--c-surface); }\n"
".site-footer .container { display: flex; align-items: center; justify-content: space-between;\n"
"                           flex-wrap: wrap; gap: var(--s-4); }\n"
".site-footer__brand { font-weight: 700; font-size: var(--t-sm); color: var(--c-text); }\n"
".site-footer__copy { font-size: var(--t-sm); color: var(--c-text-3); }\n"
"/* Responsive */\n"
"@media (max-width: 768px) {\n"
"  .hero { padding: var(--s-16) 0 var(--s-12); }\n"
"  .page-body { padding: var(--s-12) 0 var(--s-16); }\n"
"  .post-title { font-size: var(--t-2xl); }\n"
"}\n"
"@media (max-width: 480px) {\n"
"  .container { padding-inline: var(--s-4); }\n"
"  .site-nav__links { gap: var(--s-4); }\n"
"  .hero__subtitle { font-size: var(--t-base); }\n"
"}\n";

/* ---- Theme definition ---------------------------------------------------- */

static const TplThemeLayout layouts[] = {
    { "home", layout_home },
    { "base", layout_base },
    { "post", layout_post },
};

const TplTheme tpl_builtin_theme_modern = {
    .name         = "modern",
    .description  = "Fluid typography, sticky nav, hero section, CSS Grid",
    .layouts      = layouts,
    .layout_count = 3,
    .css          = css,
};
