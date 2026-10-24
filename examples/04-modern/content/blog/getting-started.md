---
title: Getting Started with templ
date: 2025-05-10
description: A quick guide to setting up and running your first templ site.
layout: post
---

# Getting Started with templ

## Prerequisites

- CMake 3.27+
- A C11 compiler (GCC, Clang, or MSVC)

## Build the binary

From the repository root:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target templ
```

The `templ` binary lands in `build/` (or `build/Release/` on Windows with MSVC).

## Create a project

The minimal project needs only two things:

```
my-site/
  tconf.json
  content/
    index.md
```

A minimal config:

```json
{
  "site": { "title": "My Site", "base_url": "" },
  "paths": { "content": "content", "templates": "templates",
             "assets": "assets",   "output": "dist" },
  "defaults": { "layout": "base" }
}
```

## Run the build

```sh
cd my-site
templ build
```

Output lands in `dist/`.

## What's next

- Add `templates/layouts/base.html` to wrap pages in a real HTML document
- Add `templates/partials/nav.html` for shared navigation
- Create a `blog/` subdirectory in `content/` — it becomes a collection
- Drop a stylesheet into `assets/` — it's copied verbatim to `dist/assets/`
