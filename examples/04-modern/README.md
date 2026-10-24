# 04-modern

A complete, modern-looking static site with responsive CSS Grid layout,
fluid typography, and a hero section.

## What you get

- **Sticky nav** with `backdrop-filter` blur
- **Hero section** on the home page with gradient background and CTA buttons
- **Fluid typography** using `clamp()` across a full type scale
- **CSS custom properties** for easy theming
- **Responsive** — works from 320px phones to wide desktops
- **Blog post layout** with date badge, description, and styled article prose
- **Footer partial** shared across all layouts
- **Styled prose** — code blocks, blockquotes, tables, lists, images

## Files

```
04-modern/
  tconf.json
  content/
    index.md               ← home page (layout: home)
    about.md               ← about page (layout: base)
    blog/
      index.md             ← manual blog listing (layout: base)
      hello-world.md       ← post (layout: post)
      getting-started.md   ← post (layout: post)
  templates/
    layouts/
      home.html            ← hero + body
      base.html            ← page header + prose
      post.html            ← article with date badge and back link
    partials/
      nav.html             ← sticky nav bar
      footer.html          ← site footer
  assets/
    style.css              ← all styles (~250 lines)
```

## Build

```sh
../../../build/Debug/templ.exe build   # Windows (Debug)
../../../build/templ build             # Linux/macOS
```

Open `dist/index.html` in a browser.  Because paths like `/assets/style.css`
are absolute, serve it with a local server for the CSS to load:

```sh
# Python 3
cd dist && python -m http.server 8080
# Then open http://localhost:8080
```
