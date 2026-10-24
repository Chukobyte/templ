---
title: About
description: How templ works and why it exists.
layout: base
---

# About

**templ** is a static site generator written in C11, built on top of the
[seika](/) utility library.  It converts Markdown files with YAML frontmatter
into complete HTML sites using a lightweight template engine.

## How it works

1. Write Markdown in `content/` — subdirectories become collections
2. Add YAML frontmatter for metadata: `title`, `date`, `layout`, `description`
3. Create layouts in `templates/layouts/` and partials in `templates/partials/`
4. Run `templ build` — output lands in `dist/`

## Template syntax

| Syntax | Purpose |
|---|---|
| `{{title}}` | Escaped variable |
| `{{{body}}}` | Unescaped HTML (use for rendered Markdown) |
| `{{site.title}}` | Config value |
| `{{> nav}}` | Include a partial |

## Design goals

- **No runtime dependencies** — single statically-linked binary
- **Fast** — rebuilds a 200-page site in under a second
- **Portable** — runs on Windows, Linux, and macOS
