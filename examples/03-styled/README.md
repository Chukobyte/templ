# 03-styled

Adds a CSS stylesheet, a nav partial, and a blog collection.

## What you get

- Responsive single-column layout with a max-width
- Styled nav bar, headings, code blocks, blockquotes
- A `post` layout that adds a date line and "back" link
- A `{{> nav}}` partial shared across layouts
- Fluid typography with `clamp()`

## Files added vs 02-layout

```
assets/
  style.css              ← copied to dist/assets/style.css
templates/
  layouts/
    post.html            ← new: wraps blog posts
  partials/
    nav.html             ← new: shared nav bar
content/
  blog/
    index.md             ← new: manual blog listing page
    hello-world.md       ← new: first blog post
```

## Build

```sh
../../../build/Debug/templ.exe build   # Windows (Debug)
../../../build/templ build             # Linux/macOS
```

## Next step

→ **04-modern** upgrades to CSS Grid, custom properties, a hero section,
  and a fully responsive card-based design
