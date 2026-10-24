# templ

A static site generator written in C11. Reads Markdown content with YAML frontmatter and produces static HTML sites using a lightweight template engine.

## Building

templ depends on [seika](https://github.com/Chukobyte/seika), which CMake fetches automatically (pinned in `CMakeLists.txt`):

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target templ
```

The `templ` binary lands in `build/` (or `build\Debug\` / `build\Release\` with a multi-config generator such as Visual Studio).

To also build and run the unit tests:

```sh
cmake --build build
ctest --test-dir build --output-on-failure
```

Or with zig: `zig build`.

### Developing against a local seika

```sh
cmake -B build -DFETCHCONTENT_SOURCE_DIR_SEIKA=../seika   # CMake
zig build --fork=../seika                                 # zig
```

Inside the `new_project` superproject this happens automatically for CMake — the root adds `seika/` before `templ/`, so the fetch is skipped.

## Installation

Copy the binary to a permanent location and add it to your `PATH` so you can run `templ` from any directory.

### Windows

```powershell
# 1. Create an install directory
New-Item -ItemType Directory -Force "$env:LOCALAPPDATA\Programs\templ"

# 2. Copy the binary (adjust Debug/Release to match your build type)
Copy-Item build\Debug\templ.exe "$env:LOCALAPPDATA\Programs\templ\"

# 3. Add to PATH for the current user (persists across sessions)
$installDir = "$env:LOCALAPPDATA\Programs\templ"
$current = [System.Environment]::GetEnvironmentVariable("PATH", "User")
if ($current -notlike "*$installDir*") {
    [System.Environment]::SetEnvironmentVariable("PATH", "$current;$installDir", "User")
}

# 4. Reload PATH in the current shell
$env:PATH = [System.Environment]::GetEnvironmentVariable("PATH", "User")
```

Open a new terminal and verify:

```powershell
templ --help
```

### Linux / macOS

```sh
# Copy to a directory already on PATH
sudo cp build/templ /usr/local/bin/

# Or install to a user-local bin (no sudo required)
mkdir -p ~/.local/bin
cp build/templ ~/.local/bin/

# If ~/.local/bin is not already on your PATH, add this to ~/.bashrc or ~/.zshrc:
export PATH="$HOME/.local/bin:$PATH"
```

Reload your shell and verify:

```sh
source ~/.bashrc   # or ~/.zshrc
templ --help
```

## Quick start

The only required files are `tconf.json` and your Markdown content. No templates needed — pick a built-in theme and go:

```sh
mkdir mysite && cd mysite
templ new           # writes tconf.json
# edit config: set site.title and theme
templ build         # generates the output directory
templ serve         # build + serve on http://localhost:8080
```

All commands must be run from the directory containing `tconf.json`.

## CLI

| Command | Description |
|---|---|
| `templ build [--theme <name>]` | Build the site. `--theme` overrides the config value. |
| `templ serve [--port <n>] [--theme <name>]` | Build then serve locally. Default port: 8080. |
| `templ deploy` | Build then force-push the output directory to the `gh-pages` branch. |
| `templ new [<name>]` | Write a starter `tconf.json` in `<name>/` or the current directory. |

## Themes

templ ships with built-in themes so you can get a working site from just Markdown files and a config — no templates directory required.

### Built-in themes

| Theme | Description |
|---|---|
| `default` | Clean single-column layout, styled nav, responsive CSS |
| `modern` | Sticky nav, hero section, fluid typography, CSS custom properties |
| `bare` | Raw HTML fragment only — no `<html>` wrapper, no CSS |

Select a theme in `tconf.json`:

```json
{
  "theme": "modern"
}
```

Or override it per-build on the CLI:

```sh
templ build --theme modern
templ serve --theme default
```

### Theme resolution order

1. **Disk templates** — if `templates/layouts/{layout}.html` exists, it is always used first
2. **Active theme** — if the layout is not on disk, templ looks it up in the configured theme
3. **Raw HTML fallback** — if neither provides a layout, the rendered Markdown body is written as a plain fragment

This means you can override individual layouts by creating them on disk while letting the theme handle the rest.

### Theme CSS

When a theme includes CSS, it is written to `dist/assets/theme.css` before user assets are copied.  Your own `assets/` directory is copied afterwards, so placing a `theme.css` there overrides the built-in one.

Built-in theme layouts reference `/assets/theme.css`.  If you write your own layouts you can reference any stylesheet you like.

### Registering a custom theme (C API)

```c
#include "theme.h"

static const TplThemeLayout my_layouts[] = {
    { "base", "<!DOCTYPE html><html>...<body>{{{body}}}</body></html>" },
    { "post", "<!DOCTYPE html><html>...<p>{{date}}</p>{{{body}}}...</html>" },
};

static const TplTheme my_theme = {
    .name         = "mytheme",
    .description  = "My custom theme",
    .layouts      = my_layouts,
    .layout_count = 2,
    .css          = "body { font-family: sans-serif; }",
};

// Call before tpl_engine_build()
tpl_theme_register(&my_theme);
```

Layout strings use the same template syntax as on-disk files (`{{var}}`, `{{{body}}}`, `{{> partial}}`, etc.).

## Project layout

The only required file is `tconf.json`. All directory names are configurable.

```
mysite/
  tconf.json       ← required; run templ commands here
  content/                ← Markdown source
    index.md
    blog/
      my-post.md
  templates/              ← optional; overrides theme layouts per-file
    layouts/
      base.html
    partials/
      nav.html
  assets/                 ← static files copied verbatim to dist/assets/
    style.css
  dist/                   ← generated output (gitignore this)
```

`templates/` is optional when using a built-in theme.

## tconf.json

```json
{
  "site": {
    "title": "My Site",
    "base_url": "https://example.com"
  },
  "theme": "modern",
  "paths": {
    "content":   "content",
    "templates": "templates",
    "assets":    "assets",
    "output":    "dist"
  },
  "defaults": {
    "layout": "base"
  },
  "collections": {
    "blog": {
      "order_by": "date",
      "order":    "desc",
      "layout":   "post"
    }
  }
}
```

Top-level directories under `content/` that match a collection name are treated as collections. Everything else is a standalone page.

A `content/blog/index.md` file is rendered to `dist/blog/index.html` (URL `/blog/`), not to the root.

## Content files

Markdown files with optional YAML frontmatter:

```markdown
---
title: My Post
date: 2025-02-15
layout: post
description: A short summary.
---

## Body

Normal Markdown here. **Bold**, *italic*, `code`.
```

Supported frontmatter fields: `title`, `layout`, `date`, `order`, `description`, `featured`.

## URL scheme

| Source | URL | Output |
|---|---|---|
| `content/index.md` | `/` | `dist/index.html` |
| `content/about.md` | `/about/` | `dist/about/index.html` |
| `content/blog/index.md` | `/blog/` | `dist/blog/index.html` |
| `content/blog/my-post.md` | `/blog/my-post/` | `dist/blog/my-post/index.html` |

## Template syntax

| Tag | Meaning |
|---|---|
| `{{var}}` | HTML-escaped variable |
| `{{{var}}}` | Unescaped HTML (use for `body`) |
| `{{site.title}}` | Dot-path into config site block |
| `{{> nav}}` | Include `templates/partials/nav.html` |
| `{{#if var}}...{{/if}}` | Render block only if `var` is set and non-empty |
| `{{#unless var}}...{{/unless}}` | Render block only if `var` is absent or empty |

### Variable scope (later overrides earlier)

1. `site.*` from config
2. Collection config defaults
3. Page frontmatter fields
4. Engine-injected: `url`, `body`

## Deploy to GitHub Pages

```sh
templ deploy
```

Builds the site then force-pushes the output directory to the `gh-pages` branch of your `origin` remote. The branch contains only the built HTML — no source files. A `.nojekyll` file is included automatically so GitHub Pages serves the files without Jekyll processing.

Requires `git` on `$PATH` and a remote named `origin`.

## HTTP server

`templ serve` uses the built-in seika TCP server (`SKA_HTTP_SERVER`). It is single-threaded and intended for local development only.

The server is compiled in when `SEIKA_HTTP_SERVER=ON` (the default). To disable:

```sh
cmake -B build -DSEIKA_HTTP_SERVER=OFF
```

## Examples

Progressive examples live in `examples/`:

| Directory | What it shows |
|---|---|
| `examples/01-bare/` | One Markdown file, no config theme — raw fragment output |
| `examples/02-layout/` | Custom `templates/layouts/base.html`, no CSS |
| `examples/03-styled/` | Custom CSS, nav partial, blog collection |
| `examples/04-modern/` | Custom modern CSS with Grid, fluid type, hero section |

Build any example from its directory:

```sh
cd examples/04-modern
../../../build/Debug/templ.exe build      # Windows
../../../build/templ build                # Linux/macOS
```

## Dependencies

| Library | Purpose |
|---|---|
| [seika](../seika/) | Memory, strings, file I/O, HTTP server |
| [cJSON](../thirdparty/cjson/) | Config JSON parsing |
| [Unity](https://github.com/ThrowTheSwitch/Unity) | Unit testing (fetched by CMake) |
