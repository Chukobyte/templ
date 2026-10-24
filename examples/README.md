# templ examples

Four progressive examples from bare-minimum to modern design.

| Dir | What it shows |
|---|---|
| `01-bare/` | One Markdown file, no templates — raw HTML fragment output |
| `02-layout/` | HTML5 layout wrapping the rendered body; no CSS |
| `03-styled/` | CSS stylesheet, nav partial, blog collection |
| `04-modern/` | CSS custom properties, fluid type, hero section, responsive layout |

## Building an example

Run from the example directory (where `tconf.json` lives):

```sh
cd examples/01-bare
../../../build/Debug/templ.exe build   # Windows (Debug build)
../../../build/templ build             # Linux/macOS
```

Output lands in the example's `dist/` directory.

## Viewing 04-modern

The modern example uses absolute paths (`/assets/style.css`), so open it via
a local server rather than the `file://` protocol:

```sh
cd examples/04-modern/dist
python -m http.server 8080
# then open http://localhost:8080
```
