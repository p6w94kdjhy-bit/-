// SVG를 발표 자료용 PNG(2배 해상도)로 변환한다.
// 사용법: node tools/svg_to_png.js a.svg [b.svg ...]   (Playwright 필요)
const path = require("path");
const fs = require("fs");
const { chromium } = require("playwright");

(async () => {
  const browser = await chromium.launch();
  const page = await browser.newPage({ deviceScaleFactor: 2 });
  for (const file of process.argv.slice(2)) {
    const svg = fs.readFileSync(file, "utf8");
    const width = Number(svg.match(/width="(\d+)"/)[1]);
    const height = Number(svg.match(/height="(\d+)"/)[1]);
    await page.setViewportSize({ width, height });
    await page.goto("file://" + path.resolve(file));
    const out = file.replace(/\.svg$/, ".png");
    await page.screenshot({ path: out, clip: { x: 0, y: 0, width, height } });
    console.log(out);
  }
  await browser.close();
})();
