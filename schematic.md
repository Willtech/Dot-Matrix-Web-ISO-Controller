# Schematic Overview

This shows the logical connections between components—no wire lengths or physical layout, just clean signal flow and some optional components.
Components:
- NodeMCU (ESP8266)
- MAX7219 Dot Matrix Display (8x8 or chained)
- Optional Status Pixel (WS2812 or similar)
- Optional EEPROM (I²C or SPI)
- Power Supply (5V regulated)

Signal Flow:
```
+------------------+       +------------------+
|   NodeMCU ESP8266|       | MAX7219 Matrix   |
|------------------|       |------------------|
| D5 (GPIO14) -------> CLK |                  |
| D7 (GPIO13) -------> DIN |                  |
| D8 (GPIO15) -------> CS  |                  |
|                      +---> VCC (5V)         |
|                      +---> GND              |
| D4 (GPIO2) -------> Status Pixel (Data In)  |
|                      +---> VCC (5V)         |
|                      +---> GND              |
+------------------+       +------------------+

Optional:
| D1/D2 (GPIO5/4) --> EEPROM SDA/SCL
```

---

🧰 Connection Diagram (Computed Wires)

This is the physical wiring layout—ideal for breadboard or perfboard setup.

| NodeMCU Pin | Connected To         | Wire Color Suggestion |
|-----------------|--------------------------|----------------------------|
| D5 (GPIO14)      | MAX7219 CLK              | Green                      |
| D7 (GPIO13)      | MAX7219 DIN              | Blue                       |
| D8 (GPIO15)      | MAX7219 CS               | Yellow                     |
| 3V or VU         | MAX7219 VCC              | Red                        |
| GND              | MAX7219 GND              | Black                      |
| D4 (GPIO2)       | WS2812 Data In           | White                      |
| 3V or VU         | WS2812 VCC               | Red                        |
| GND              | WS2812 GND               | Black                      |
| D1/D2 (GPIO5/4)  | EEPROM SDA/SCL (optional)| Purple / Orange           |

---

🛠️ Notes:
- Power: If chaining multiple MAX7219 modules, use external 5V power with shared GND.
- Status Pixel: Can be used to reflect ISO status, ping response, or fallback mode.
- EEPROM: Optional, but useful for storing persistent messages or fallback logic.
- Serial Debug: Use USB for serial output during dev; consider adding TX/RX breakout if needed.

---
