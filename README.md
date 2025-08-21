# SO-ARM-chan

**"Fusing the Soul of a Character Robot with the Body of an AI Robot Arm."** SO-ARM-chan is an open-source robotics platform designed to bridge two vibrant maker communities.

---

## Overview

SO-ARM-chan is a unique robot that gives the "character" of Stack-chan to the `SO-101` research robot arm, all powered by M5Stack's `AtomS3R` as its core.

This project aims to be a **bridge** between two distinct worlds: the expressive and "kawaii" **Stack-chan community** and the **LeRobot community**, which is gaining attention for its integration with imitation learning and foundational robot models.

Hobbyists can equip their characters with cutting-edge AI, while researchers can explore new human-robot interactions with a more personable robot. SO-ARM-chan provides a tangible and accessible platform for both.

### ✨ Features

* **Expressive Face:** The built-in IMU (Inertial Measurement Unit) detects movement, changing the robot's facial expression in real-time.
* **Unique Motions:** Comes with multiple motion patterns (like bowing and shaking its head) that can be easily customized via a configuration file.
* **High Extensibility:** Infinitely expandable by adding M5Stack's rich ecosystem of modules (e.g., for voice and camera).
* **Self-Contained Calibration:** Reads the servo's operational range directly from each servo's EEPROM, eliminating the need for external calibration files and ensuring robust, reproducible movements.
* **AI-Ready:** Designed with the LeRobot ecosystem in mind, serving as an entry point for experimenting with advanced AI like imitation learning and foundational models such as NVIDIA Isaac GR00T.

---

## 🛠️ Hardware Components

| Component | Role | Notes |
| :--- | :--- | :--- |
| **AtomS3R** | Main Controller, Left Eye, IMU | The core ESP32-S3 brain for expression and motion detection. |
| **AtomS3R-M12** | Camera Unit, Right Eye | (Future Expansion) For computer vision tasks with LeRobot. |
| **Feetech STS3215 x6** | Servo Motors | Constitutes the SO-101 robot arm. |
| **Seeed Studio Servo Adaptor**| Servo Controller Board | Manages the servos from the AtomS3R. |
| **Atomic Echo Base** | Audio I/O | (Future Expansion) For conversational AI features with LLMs. |
| **3D Printed Parts** | Face & Mounts | Custom parts designed in FreeCAD. |
| **External Power Supply** | 7.4V~12V (2A+) | Provides stable power to the servo motors and controller. |

---

## ⚙️ Software Stack

* **Development Environment:** PlatformIO (Arduino Framework)
* **Key Libraries:**
  * `M5Unified`: Integrated library for M5Stack devices (LCD, IMU).
    * `SCServo`: For controlling the Feetech servos.
    * `ArduinoJson`: For parsing the `config.json` settings file.
    * `ArduinoBLE`: Will be used for the teleoperation feature.

---

## 🚀 Getting Started

### 1. Prerequisites

* All parts listed in the **Hardware Components** table.
* A development environment with PlatformIO installed (VSCode is recommended).
* A PC with [LeRobot](https.github.com/huggingface/lerobot) set up (required for initial servo calibration).

### 2. Assembly

(Link to an assembly manual or provide brief steps here)

### 3. Servo Calibration

This project stores the operational range directly on each servo's EEPROM. Please follow these steps to calibrate:

1. Connect the SO-101 arm to your PC.
2. Follow the official LeRobot instructions to perform servo calibration.
3. Once completed, the `min_position_limit`, `max_position_limit`, and `homing_offset` values are written to each servo's EEPROM.

### 4. Firmware Installation

1. Clone this repository:

    ```bash
    git clone [Your Repository URL]
    ```

2. Open the project in PlatformIO.
3. Place your image files (e.g., `normal.png`) and the settings file `config.json` into the `/data` directory.
    * Use `config.json.sample` as a reference to create your own `config.json`.
4. Run the **"Upload Filesystem Image"** task to write the files from the `/data` directory to the AtomS3R.
5. Run the **"Upload"** task to flash the main firmware.

---

## 🕹️ Usage

* Upon startup, the servos will move to their initial positions and enter standby mode.
* Pressing **Button A** on the unit will cycle through the servo motion patterns defined in `config.json` (e.g., Normal -> Bow -> Shake Head).
* Gently shaking the unit will trigger the IMU, causing the facial expression on the LCD to change in real-time.

---

## 🗺️ Future Roadmap

SO-ARM-chan is a platform designed to evolve with the community. Here are the planned expansions:

* **BLE Teleoperation Integration:** Low-latency wireless control using Bluetooth Low Energy. **A prototype is already working in a separate codebase** and will be integrated into the main firmware in a future update. This will enable real-time control from a PC, essential for imitation learning data collection.
* **Conversational AI:** Integrate the Atomic Echo Base with LLMs (like ChatGPT) and voice synthesis APIs (like VOICEVOX) to enable natural voice commands and conversation.
* **Computer Vision:** Utilize the AtomS3R-M12 camera for features like object recognition and face tracking.
* **Full LeRobot Integration:** Move beyond basic control to run imitation learning models collected on a PC directly on the SO-ARM-chan.

---

## 🤝 Contributing

Contributions are what make the open-source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

---

## 🙏 Acknowledgements

* The **Stack-chan** community for creating such a wonderful character and culture.
* The **LeRobot** developers and community for pioneering accessible AI robotics.
* **M5Stack** for providing the excellent hardware that forms the foundation of this project.

---

## 📜 License

Distributed under the **Apache License 2.0**. See `LICENSE` for more information.
