# 02-layout

Adds an HTML5 layout template. No CSS — just structure.

## What you get

- A proper `<!DOCTYPE html>` wrapper around every page
- `{{title}}` and `{{site.title}}` interpolated into the `<title>` tag
- A minimal nav with links between pages
- `{{{body}}}` (triple braces = unescaped) renders the Markdown HTML

## Files added vs 01-bare

```
templates/
  layouts/
    base.html   ← new
content/
  about.md      ← new second page
```

## Build

```sh
../../../build/Debug/templ.exe build   # Windows (Debug)
../../../build/templ build             # Linux/macOS
```

## Next step

→ **03-styled** adds a CSS stylesheet and a blog collection
