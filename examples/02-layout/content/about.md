---
title: About
description: About this site.
layout: base
---

# About

This is the about page.  It uses the same `base` layout as the home page.

## How layouts work

1. `templ` renders the Markdown body to HTML
2. It reads `templates/layouts/{layout}.html`
3. It substitutes `{{{body}}}` with the rendered HTML
4. Variables from frontmatter (`{{title}}`, `{{description}}`) are available
5. Site-level vars from the config are available as `{{site.title}}`, etc.
