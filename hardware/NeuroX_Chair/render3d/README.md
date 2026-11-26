Realistic renders (three.js in headless Chromium):
    python3 render3d/export_scene.py
    npm i three@0.160.0   # once, anywhere; point THREE_DIR at its node_modules/three
    THREE_DIR=/path/to/node_modules/three node render3d/render.js   # -> img/real_*.png
Cameras and labels: render3d/views.json. Meshes in render3d/out/ are generated (not committed).
