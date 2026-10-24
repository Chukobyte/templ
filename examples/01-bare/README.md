# 01-bare

The absolute minimum: one Markdown file, one config, no templates, no assets.

## What you get

- `templ` converts the Markdown body to HTML
- No `templates/layouts/` dir means no HTML wrapper — the output is a raw fragment
- Useful as a sanity-check or for piping HTML into another tool

## Build

```sh
../../../build/Debug/templ.exe build   # Windows (Debug)
../../../build/templ build             # Linux/macOS
```

Run from **this directory** (where `tconf.json` lives).

## Output

`dist/index.html` — a raw HTML fragment, not a complete page.

## Next step

→ **02-layout** adds a proper HTML5 wrapper
