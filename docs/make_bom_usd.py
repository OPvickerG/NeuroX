"""NeuroX BOM for the Hack Club Fallen 'Upload a CSV' step: USD, one buy link per row.
INR = market price (incl. GST) from bom.csv; USD at 94.24 INR per USD (13 Nov 2026)."""
import csv

RATE = 94.24
RC = "https://robocraze.com/products/"
# item, description, qty, unit_inr, link
R = [
 ("ESP32-S3 DevKitC-1 (N8R8)", "Box: main controller, screen + buttons + radio", 1, 1124, RC + "7semi-esp32-s3-dev-boardc-1-n8r8-wifi-bluetooth-dual-usb-c-rgb-led"),
 ("2.8in SPI TFT touch screen 240x320", "Box: display + XPT2046 resistive touch", 1, 1031, RC + "2-8-inch-spi-tft-touchscreen-module-240-320-resolution"),
 ("Tactile switch 12x12 mm 4-pin (pack of 5)", "Box: OK, LEFT, NEW, STOP buttons (4 used)", 1, 37, "https://www.electronicscomp.com/12x12x7mm-tactile-4-pin-push-button-switch-5-pieces-pack"),
 ("KCD1 mini rocker switch (pack of 5)", "Box: power switch (1 used)", 1, 46, RC + "rocker-switch-mini-on-off-spst-2-pin-kcd11"),
 ("TP4056 USB-C charger with protection", "Box + HALO: LiPo charging (1 each)", 2, 36, RC + "tp4056-battery-charger-c-type-module-with-protection-1"),
 ("LiPo 3.7V 2000mAh", "Box: battery", 1, 369, RC + "witty-fox-2000mah-rechargeable-3-7v-lipo-battery"),
 ("5V active buzzer 12mm (bare)", "Box: beeps for lock, unlock, STOP; soldered on my perfboard", 1, 15, "https://www.flyrobo.in/5v-active-electromagnetic-buzzer"),
 ("MPU-6050 (GY-521) accel + gyro", "Box: tilt / fall check", 1, 199, RC + "mpu-6050-triple-axis-accelerometer-gyroscope-module"),
 ("Perfboard / prototyping board", "Box + HALO: cut into the two boards", 1, 29, "https://robocraze.com/collections/prototyping-boards"),
 ("1N5819 Schottky diode (pack of 5)", "Box + HALO: reverse-current diode (2 used)", 1, 12, RC + "1n5819-diode-pack-of-5"),
 ("100k resistor 1/4W (pack of 10)", "Box + HALO: battery / USB sense dividers (5 used)", 1, 13, RC + "100k-ohm-resistor"),
 ("150k resistor 1/4W", "Box: USB sense divider (1 used)", 1, 13, "https://robocraze.com/collections/resistors"),
 ("0.1uF 50V ceramic capacitor (pack of 10)", "Box + HALO: decoupling (4 used)", 1, 17, RC + "0-1-uf-50v-ceramic-capacitor"),
 ("100uF 25V electrolytic (pack of 5)", "Box: across battery input (1 used)", 1, 23, RC + "100uf-25v-electrolytic-capacitor-pack-of-5"),
 ("10uF 25V electrolytic capacitor", "HALO: BioAmp pill decoupling", 2, 6, "https://robocraze.com/collections/capacitors"),
 ("Female header 1x40 2.54mm", "Box: socket for the ESP32-S3 (2 x 22 pins)", 2, 25, RC + "40x1-female-berg-strip"),
 ("Male header 1x40 2.54mm", "Box: screen, power, buttons, buzzer, MPU headers", 1, 7, RC + "40x1-pin-2-54mm-pitch-male-berg-strip"),
 ("Female-female jumper wires 20cm (40 pcs)", "Box: screen, buttons, MPU wiring", 1, 78, RC + "f2f-jumper-wires-20cm-40pcs"),
 ("ESP32-C3 SuperMini", "HALO: MCU on my HALO perfboard; reads both pills at 256 Hz, sends over ESP-NOW", 1, 287, RC + "esp32-c3-mini-development-board-unsoldered"),
 ("BioAmp EXG Pill kit (with cable + gel electrodes)", "HALO: 2 channels, eye + jaw signals", 2, 2649, "https://store.upsidedownlabs.tech/product/bioamp-exg-pill/"),
 ("LiPo 3.7V 1000mAh", "HALO: battery", 1, 222, RC + "3-7v-1000mah-li-ion-battery"),
 ("Slide switch SPDT 3-pin", "HALO: power switch", 1, 10, RC + "slide-switch-3-pin-2-way-spdt"),
 ("M3x8 socket head bolt + nut set (12 pcs)", "HALO: 6 stem lock knobs", 1, 36, "https://robu.in/product/easymech-set-m3-x-8mm-socket-head-cap-allen-bolt-nut-12-pcs/"),
 ("M3 self-tapping screws (pack of 10)", "Box + HALO: lid, covers, guides (33 used)", 4, 40, "https://thinkrobotics.com/collections/screws-and-nuts-online/products/self-tapping-screws"),
 ("M2 self-tapping screws (pack of 10)", "Box + HALO: small parts (16 used)", 2, 40, "https://thinkrobotics.com/collections/screws-and-nuts-online/products/self-tapping-screws"),
 ("M4x16 bolt + nut", "HALO: back dial", 1, 8, "https://robu.in/product-category/mechanical-parts-and-tools/nuts-bolts-and-spacers/socket-head-cap-screw-allen-bolt/"),
 ("Heat shrink tube kit (530 pcs)", "Box + HALO: insulate solder joints", 1, 270, RC + "530-pcs-color-shrink-tube-wire-insulation-sleeve"),
 ("Fevikwik gel (2 x 3 g)", "Box: glue button guides; HALO: knobs, hub", 1, 20, "https://www.bigbasket.com/pd/1213331/pidilite-fevikwik-gel-2-x-3-g"),
 ("Nylon cable ties 100mm (100 pcs)", "HALO: electrode cable strain relief", 1, 50, "https://robu.in/product/nylon-cable-zip-ties-100mm-100pcs-bag-1-bag"),
 ("EVA foam sheets 2mm A4", "HALO: comfort padding inside the band", 1, 40, "https://www.meesho.com/qatalitic-a4-eva-foam-sheet-10-different-colours-2mm-thickness-for-diy-art-craft-projects-decorations/p/1chtyh"),
 ("Bambu Lab PLA Basic 1kg", "Box + HALO: ~650 g of prints", 1, 1229, "https://www.moglix.com/bambu-lab-1kg-175mm-pla-black-3d-printer-filament-with-spool/mp/msn75dql2ojv92"),
]
tot = 0.0
with open("NeuroX_BOM_USD.csv", "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["item", "description", "qty", "unit_price_usd", "total_usd", "link"])
    for item, desc, q, inr, link in R:
        u = round(inr / RATE, 2); t = round(u * q, 2); tot += t
        w.writerow([item, desc, q, f"{u:.2f}", f"{t:.2f}", link])
print(len(R), "rows, total USD", round(tot, 2))
