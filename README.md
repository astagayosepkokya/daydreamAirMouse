# 🌌 Daydream Air Mouse

**Transform your Google Daydream controller into a high-precision, stable Windows Air Mouse.**

This project is a high-performance C++/WinRT driver that decodes the proprietary Bluetooth LE protocol of the Daydream controller, translating its raw 14-bit Big-Endian inertial data into smooth, natural system-level mouse movement.

---

## ✨ Key Features

*   **🛡️ Safety Launch:** Prevents duplicate instances; automatically opens Settings on startup for easy status monitoring.
*   **🎮 Dual-Mode Mouse:** Toggle between high-precision **Gyroscope Air Mouse** and relative **Touchpad Mode** (Trackpad style).
*   **⚡ Independent Tuning:** Separate sensitivity and acceleration settings for both modes, persisted in `config.ini`.
*   **🖱️ Custom Button Mapping:**
    *   **Touchpad Click, Home, App:** Fully remappable to Left, Right, or Middle click.
    *   **Volume Keys:** Switchable between Volume control, Mouse Scrolling, or Page Up/Down.
*   **📐 Guided Calibration:** Simple manual trigger with guided instructions and success notifications to ensure perfect stability.
*   **🔄 Quick Mode Toggle:** Switch modes on-the-fly using the `App + Home` button combo.
*   **📦 Tray-Based Service:** Runs silently in the system tray with a right-click Exit menu.

---

## 🛠️ Technical Specifications

The driver decodes a 20-byte (160-bit) dense packet structure via BLE Service `0000fe55` and Characteristic `00000001`.

### Bit Address Mapping
| Component | Type | Start Bit | Length | Endian |
| :--- | :--- | :--- | :--- | :--- |
| **Orientation X/Y/Z** | Sensor | 16 / 29 / 42 | 13 bits | Big Endian |
| **Accel X/Y/Z** | Sensor | 55 / 68 / 81 | 13 bits | Big Endian |
| **Gyro X (Pitch)** | Sensor | 94 | 13 bits | Big Endian |
| **Gyro Z (Yaw)** | Sensor | 107 | 13 bits | Big Endian |
| **Gyro Y (Roll)** | Sensor | 120 | 13 bits | Big Endian |
| **Touch X** | Coordinate | 128 | 5 bits | Little Endian |
| **Touch Y** | Coordinate | 136 | 5 bits | Little Endian |
| **Touch Click** | Button | 144 | 1 bit | Little Endian |
| **Home Button** | Button | 145 | 1 bit | Little Endian |
| **App Button** | Button | 146 | 1 bit | Little Endian |
| **Volume Minus** | Button | 147 | 1 bit | Little Endian |
| **Volume Plus** | Button | 148 | 1 bit | Little Endian |
| **Battery Level** | Status | 152 | 7 bits | Little Endian |

---

## 🚀 Getting Started

1.  **Pair your controller:** Open Windows Bluetooth settings and pair your "Daydream controller".
2.  **Build:** Run `build.bat` (requires Visual Studio Community or Build Tools).
3.  **Run:** Launch `daydream.exe`. The Settings window will open automatically.
4.  **Configure:** Set your preferred sensitivities and button mappings.
5.  **Calibrate:** Click "Calibrate Now" and follow the on-screen instructions.
6.  **Enjoy:** Close the settings window to hide it in the tray. Use `App + Home` to toggle modes at any time.

---

## 📜 Acknowledgments

Developed with 💖 for the VR/Open-source community. Special thanks to the reverse-engineering efforts that decoded the bit-packed protocol.
