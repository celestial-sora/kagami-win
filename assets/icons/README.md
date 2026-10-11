# Transport icons

The user supplied these original SVG files for the connection tiles:

- `airplay.svg`: original `airplay-svgrepo-com.svg`, carrying the SVG Repo upload/generator comment.
- `smartview.svg`: original `noun_smartview_3844661.svg`, **Created by The Icon Z**, from the **Noun Project**. The original attribution text is retained in this file.

The original SVGs remain unchanged. The committed 128×128 transparent PNGs recolor the glyphs to lavender `#ddbef0`. The Smart View raster uses the square glyph area (`0 0 32 32`); the below-viewBox attribution text is represented here instead of being rendered inside a small UI tile. No license text accompanied the supplied files; retain this attribution and confirm redistribution terms before stable release.

`tools/convert-transport-icons.cjs` performs offline conversion with skia-canvas. Pass an installed module's absolute path as its argument. End-user executables embed the PNGs and require no SVG renderer, Node or skia-canvas. The approved Kagami application artwork is unchanged.
