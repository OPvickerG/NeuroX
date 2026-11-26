// Realistic renders of the chair drive (three.js in headless Chromium).
// Run: python3 render3d/export_scene.py, then  THREE_DIR=<node_modules/three> node render3d/render.js  -> ../img/real_*.png
const http = require("http"), fs = require("fs"), path = require("path");
const { chromium } = require("playwright");
const OUT = path.join(__dirname, "out"), IMG = path.join(__dirname, "..", "img");
const THREE_DIR = process.env.THREE_DIR || path.join(__dirname, "node_modules", "three");

const PAGE = `<!doctype html><html><head><meta charset="utf-8"><style>
html,body{margin:0;background:#eef0f3;overflow:hidden;font-family:"DejaVu Sans",Arial,sans-serif}
#lab{position:absolute;inset:0;pointer-events:none}
.t{position:absolute;font-size:17px;color:#16181c;background:rgba(255,255,255,.88);padding:4px 8px;border-radius:6px;white-space:pre;line-height:1.3;
   box-shadow:0 1px 4px rgba(0,0,0,.18)}
#ttl{position:absolute;left:22px;top:16px;font-size:24px;font-weight:600;color:#16181c}
</style>
<script type="importmap">{"imports":{"three":"/three/build/three.module.js","three/addons/":"/three/examples/jsm/"}}</script></head>
<body><div id="ttl"></div><svg id="lab" width="100%" height="100%"></svg>
<script type="module">
import * as THREE from "three";
import { STLLoader } from "three/addons/loaders/STLLoader.js";
import { RoomEnvironment } from "three/addons/environments/RoomEnvironment.js";
const V = JSON.parse(decodeURIComponent(location.hash.slice(1)));
const S = await (await fetch("/out/scenes.json")).json();
const W = innerWidth, H = innerHeight;
const r = new THREE.WebGLRenderer({ antialias: true, preserveDrawingBuffer: true });
r.setPixelRatio(devicePixelRatio); r.setSize(W, H); r.shadowMap.enabled = true; r.shadowMap.type = THREE.PCFSoftShadowMap;
r.toneMapping = THREE.ACESFilmicToneMapping; r.toneMappingExposure = 1.15; r.outputColorSpace = THREE.SRGBColorSpace;
document.body.prepend(r.domElement);
const scene = new THREE.Scene(); scene.background = new THREE.Color("#eef0f3");
scene.environment = new THREE.PMREMGenerator(r).fromScene(new RoomEnvironment(), 0.04).texture;
const root = new THREE.Group(); root.rotation.x = -Math.PI / 2; scene.add(root);        // chair Z up -> three Y up
const sun = new THREE.DirectionalLight(0xffffff, 2.2); sun.position.set(500, 1400, 900); sun.castShadow = true;
sun.shadow.mapSize.set(4096, 4096); Object.assign(sun.shadow.camera, { left: -700, right: 700, top: 700, bottom: -700, near: 10, far: 4000 });
sun.shadow.bias = -0.0004; sun.shadow.normalBias = 0.6; scene.add(sun); scene.add(new THREE.HemisphereLight(0xffffff, 0x9aa0aa, 0.9));
const toThree = (p) => new THREE.Vector3(p[0], p[2], -p[1]);
const loader = new STLLoader();
for (const p of S.parts) {
  if (V.hide.includes(p.group) || V.hide.includes(p.name)) continue;
  const g = await new Promise((res) => loader.load("/out/" + p.name + ".stl", res));
  g.computeVertexNormals();
  const [col, met, rough] = S.materials[p.mat];
  const m = new THREE.Mesh(g, new THREE.MeshStandardMaterial({ color: col, metalness: met, roughness: rough, envMapIntensity: 1.0 }));
  m.castShadow = m.receiveShadow = true;
  const off = V.exploded ? (S.exploded[p.group] || [0, 0, 0]) : [0, 0, 0];
  m.position.set(off[0], off[1], off[2]);
  root.add(m);
}
if (V.floor) {
  const f = new THREE.Mesh(new THREE.PlaneGeometry(6000, 6000), new THREE.ShadowMaterial({ opacity: 0.22 }));
  f.rotation.x = -Math.PI / 2; f.position.y = -305; f.receiveShadow = true; scene.add(f);
}
const cam = new THREE.PerspectiveCamera(V.fov || 30, W / H, 5, 20000);
cam.position.copy(toThree(V.cam)); cam.lookAt(toThree(V.tgt));
r.render(scene, cam);
document.getElementById("ttl").textContent = V.title || "";
// labels: anchor (chair coords, + exploded offset of its group), text, pixel offset
const svg = document.getElementById("lab");
for (const [key, txt, dx, dy, grp] of (V.labels || [])) {
  const a = S.labels[key].slice(); const off = V.exploded && grp ? (S.exploded[grp] || [0, 0, 0]) : [0, 0, 0];
  const p = toThree([a[0] + off[0], a[1] + off[1], a[2] + off[2]]).project(cam);
  const x = (p.x + 1) / 2 * W, y = (1 - p.y) / 2 * H, tx = x + dx, ty = y + dy;
  svg.insertAdjacentHTML("beforeend", '<line x1="' + x + '" y1="' + y + '" x2="' + tx + '" y2="' + ty + '" stroke="#16181c" stroke-width="1.6"/><circle cx="' + x + '" cy="' + y + '" r="3.5" fill="#16181c"/>');
  const d = document.createElement("div"); d.className = "t"; d.textContent = txt; document.body.appendChild(d);
  const bw = d.offsetWidth, bh = d.offsetHeight;
  d.style.left = (dx < 0 ? tx - bw : tx) + "px"; d.style.top = (ty - bh / 2) + "px";
}
window.DONE = true;
</script></body></html>`;

const VIEWS = JSON.parse(fs.readFileSync(path.join(__dirname, "views.json"), "utf8"));
const srv = http.createServer((q, s) => {
  const u = decodeURIComponent(q.url.split("#")[0].split("?")[0]);
  if (u === "/") { s.writeHead(200, { "content-type": "text/html" }); return s.end(PAGE); }
  const f = u.startsWith("/three/") ? path.join(THREE_DIR, u.slice(7)) : path.join(__dirname, u);
  fs.readFile(f, (e, d) => { if (e) { s.writeHead(404); return s.end(); }
    s.writeHead(200, { "content-type": f.endsWith(".js") ? "text/javascript" : f.endsWith(".json") ? "application/json" : "application/octet-stream" }); s.end(d); });
}).listen(0, async () => {
  const port = srv.address().port;
  const b = await chromium.launch({ args: ["--use-gl=angle", "--use-angle=swiftshader", "--enable-unsafe-swiftshader"] });
  const only = process.argv[2];
  for (const [name, v] of Object.entries(VIEWS)) {
    if (only && name !== only) continue;
    const pg = await b.newPage({ viewport: { width: 1500, height: 1000 }, deviceScaleFactor: 2 });
    pg.on("pageerror", (e) => console.log("ERR", e.message));
    await pg.goto("http://localhost:" + port + "/#" + encodeURIComponent(JSON.stringify(v)));
    await pg.waitForFunction("window.DONE === true", null, { timeout: 300000 });
    await pg.screenshot({ path: path.join(IMG, "real_" + name + ".png") }); console.log("saved", name); await pg.close();
  }
  await b.close(); srv.close();
});
