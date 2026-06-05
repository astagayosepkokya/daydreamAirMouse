# 🌌 Daydream Air Mouse

**Transform your Google Daydream controller into a high-precision, stable Windows Air Mouse.**

This project is a high-performance C++/WinRT driver that decodes the proprietary Bluetooth LE protocol of the Daydream controller, translating its raw 14-bit Big-Endian inertial data into smooth, natural system-level mouse movement.

---

## ✨ Key Features

*   **🚀 Gyroscope-Driven Precision:** Unlike traditional orientation-based drivers, this implementation uses raw rotation speed (Gyroscope) to eliminate **Gimbal Lock** and axis flipping at high angles.
*   **🎯 Touch-to-Move Trigger:** Features a "Dead-Man's Switch" logic—the cursor only moves when you are physically touching the touchpad, preventing accidental drift during idle handling.
*   **🛠️ Advanced Calibration:** Integrated non-blocking calibration with a 3-second countdown. Press `Vol+` and `Vol-` together to zero out any environmental drift.
*   **🖱️ Custom Button Mapping:**
    *   **Touchpad Click:** Left Mouse Click
    *   **Home Button:** Right Mouse Click
    *   **App Button:** Middle Mouse Click
    *   **Volume Keys:** Native Windows System Volume control.
*   **⚡ Real-time Speed Tuning:** Adjust mouse sensitivity on the fly by holding `App` and pressing `Vol+` or `Vol-`.
*   **📦 Lightweight & Native:** Built with modern C++20 and WinRT, running with minimal CPU overhead as a native Windows application.

---

## 🛠️ Technical Specifications

The driver decodes a 20-byte (160-bit) dense packet structure:
*   **Endianness:** Big-Endian (BE) for sensors, Little-Endian (LE) for buttons/touch.
*   **Resolution:** 13-bit fragments for motion sensors.
*   **Trigger Logic:** Coordinate-based capacitive detection.

| Component | Function | Offset |
| :--- | :--- | :--- |
| **Gyro X** | Pitch (Mouse Y) | Bit 94 |
| **Gyro Z** | Yaw (Mouse X) | Bit 107 |
| **Touchpad** | Activity Trigger | Bits 128/136 |

---

## 🚀 Getting Started

1.  **Pair your controller:** Open Windows Bluetooth settings and pair your "Daydream controller".
2.  **Build:** Run `build.bat` (requires Visual Studio Community or Build Tools).
3.  **Run:** Launch `daydream.exe`.
4.  **Calibrate:** Hold the controller steady and press `Vol+ + Vol-`. Wait 3 seconds for the beep.

---

## 📜 Acknowledgments

Developed with 💖 for the VR/Open-source community. Special thanks to the reverse-engineering efforts that decoded the bit-packed protocol.
