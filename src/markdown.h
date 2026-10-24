#pragma once

#include <stdbool.h>
#include "seika/defines.h"
#include "seika/string.h"

// Set a root-relative prefix (e.g. "../..") that is prepended to root-relative
// image src attributes so they resolve correctly in file:// contexts.
// Pass NULL to disable rewriting (default).
void tpl_markdown_set_img_root(const char* root);

// Converts a Markdown string to HTML, appending into out.
// Supports: ATX headings, paragraphs, fenced code blocks, blockquotes,
//           unordered/ordered lists, HR, bold, italic, inline code,
//           links, images, and hard line breaks.
// Headings are rendered with a stable `id="{slug}"` (see tpl_markdown_slugify)
// and a hover-revealed `.heading-anchor` permalink.
bool tpl_markdown_to_html(const char* md, usize md_len, SkaString* out);

// Slugifies heading text into a URL-safe anchor id: lowercase, non-alnum runs
// collapse to a single '-', leading/trailing '-' trimmed. Truncates to out_size - 1.
void tpl_markdown_slugify(const char* text, usize len, char* out, usize out_size);

#define TPL_MARKDOWN_TOC_MAX   64
#define TPL_MARKDOWN_TOC_TEXT_MAX 128

typedef struct {
    int level;                          // 2 or 3 (## / ###)
    char text[TPL_MARKDOWN_TOC_TEXT_MAX];
    char slug[TPL_MARKDOWN_TOC_TEXT_MAX];
} TplMarkdownTocEntry;

// Scans markdown source for ## and ### ATX headings (skips # and deeper than
// ### to keep the TOC to page-section granularity) and fills `out` with their
// text and slug, using the same slugify as tpl_markdown_to_html so anchors match.
void tpl_markdown_extract_toc(const char* md, usize md_len,
                               TplMarkdownTocEntry* out, usize max,
                               usize* count);
