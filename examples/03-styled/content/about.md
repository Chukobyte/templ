---
title: About
description: About this site.
layout: base
---

# About

This example shows how to add a stylesheet and a blog collection to a templ site.

## Project layout

```
03-styled/
  tconf.json
  content/
    index.md
    about.md
    blog/
      index.md
      hello-world.md
  templates/
    layouts/
      base.html   ← wraps every page
      post.html   ← wraps blog posts
    partials/
      nav.html    ← shared navigation
  assets/
    style.css     ← copied verbatim to dist/assets/
```

The `assets/` directory is copied to `dist/assets/` unchanged.  Link it with
`<link rel="stylesheet" href="/assets/style.css">` in your layout.
