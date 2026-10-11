// Offline development conversion. End-user builds embed the committed PNGs.
// Usage: node tools/convert-transport-icons.cjs <absolute skia-canvas module path>
const fs = require('node:fs');
const path = require('node:path');
const { Canvas, loadImage } = require(process.argv[2] || 'skia-canvas');
const root = path.resolve(__dirname, '..');
const directory = path.join(root, 'assets', 'icons');
const color = '#ddbef0';
(async () => {
  for (const name of ['airplay', 'smartview']) {
    let svg = fs.readFileSync(path.join(directory, `${name}.svg`), 'utf8');
    // The supplied noun attribution lies below the viewBox. Keep the original
    // verbatim and credit in README/ATTRIBUTION; rasterize its square glyph area.
    if (name === 'smartview') {
      svg = svg.replace(/viewBox="0 0 32 40"/, 'viewBox="0 0 32 32"')
        .replace(/<text\b[^>]*>[\s\S]*?<\/text>/g, '')
        .replace('<g data-name="Smart View">', `<g data-name="Smart View" fill="${color}">`);
    } else svg = svg.replace(/stroke="#333333"/g, `stroke="${color}"`);
    const image = await loadImage(Buffer.from(svg));
    const canvas = new Canvas(128, 128);
    const context = canvas.getContext('2d');
    context.clearRect(0, 0, 128, 128);
    context.drawImage(image, 0, 0, 128, 128);
    await canvas.toFile(path.join(directory, `${name}.png`));
    console.log(`${name}: transparent 128×128 PNG`);
  }
})().catch(error => { console.error(error); process.exitCode = 1; });
