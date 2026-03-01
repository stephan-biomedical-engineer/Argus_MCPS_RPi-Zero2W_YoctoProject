---

# Argus Medical Control Infusion Pump Gateway Project

This is the Yocto Project meta-layer developed for the **Argus Infusion Pump IoT Gateway**.
The system is based on a custom Embedded Linux environment for the **Raspberry Pi Zero 2 W** board, strictly designed with a focus on **Medical Cybersecurity**, **High Availability**, and **Hardware Integration**.

Developed for the Biomedical Instrumentation II course - Federal University of Uberlândia (UFU).

## ✨ Architecture & Key Features

This project goes beyond a simple academic prototype, implementing medical industry engineering standards:

* **Embedded DevOps (OTA & A/B Partitioning):** Full integration with **RAUC**. The system features two root partitions (A/B) with automatic fallback support. Firmware updates are performed via encrypted packages (Bundles) ensuring *Zero-Downtime*.
* **Network Security (mTLS):** MQTT Broker (Mosquitto) configured exclusively for TLS traffic (port 8883) with **Mutual Authentication (mTLS)**. Only clients with authorized certificates can send or receive telemetry.
* **Immutable File System:** Image generated with a `read-only-rootfs`, protecting the gateway against SD card corruption from power outages and mitigating malware injection attacks.
* **Service Discovery (mDNS):** Utilization of the **Avahi** daemon (`infusion-gateway.local`) for dynamic provisioning on networks without a static IP (DHCP).
* **C++ Control Layer (`argus-control-cpp`):** Custom daemon that manages MQTT communication and acts as a hardware bridge via SPI, I2C, PWM, and GPIO to the STM32 microcontroller.

## 📂 Layer Structure

The layer is organized according to OpenEmbedded best practices:

* `recipes-apps/`: Contains the source code and compilation recipe for the main C++ daemon (`argus-control-cpp`).
* `recipes-bsp/`: Modified U-Boot scripts to support RAUC partition switching.
* `recipes-connectivity/`: Network configurations, including WPA Supplicant, Mosquitto rules (ACLs/Certificates), and Avahi.
* `recipes-core/`: Base system recipes. Includes final image generation (`infusion-image`), OTA bundle creation (`infusion-bundle`), and advanced `systemd` configurations.
* `recipes-kernel/`: Linux Kernel configuration fragments (`.cfg`) enabling hardware PWM support, SPI/I2C drivers, and RAUC dependencies (SquashFS, Loopback).
* `recipes-support/`: Cryptographic keys and system configurations for the RAUC client.

## 📦 Dependencies

To build this project, your Yocto environment (`scarthgap` branch) must contain the following submodules:

* `poky`
* `meta-openembedded`
* `meta-raspberrypi`
* `meta-rauc`

## 🔐 Key and Certificate Management (IMPORTANT)

For security reasons, **private keys and certificates are not versioned in this repository**.
Before building the image or the bundle, you **must** generate your own certificates and place them in the following directories:

**1. Mosquitto Certificates (MQTT mTLS):**
Place in `recipes-connectivity/mosquitto/files/certs/`:

* `ca.crt` (Certificate Authority)
* `server.crt` and `server.key` (Gateway Keys)

**2. OTA Update Keys (RAUC):**
Place the public keyring in `recipes-support/rauc/files/`:

* `keyring.pem`

Place the Bundle signing keys in `recipes-core/bundles/files/`:

* `development-1.cert.pem`
* `development-1.key.pem`

*(The build will fail if these files are not found).*

## 🚀 How to Build

1. Initialize the Yocto environment:
```bash
source poky/oe-init-build-env

```


2. Add this layer (and its dependencies) to your `bblayers.conf`.
3. To build the base image (Flash via SD Card):
```bash
bitbake infusion-image

```


4. To build the remote update package (OTA):
```bash
bitbake infusion-bundle

```



## 🛠️ Authorship

Developed by **Stephan Costa Barros** - Federal University of Uberlandia, Brazil, Electrical Engineering Department.

Oriented by **Marcelo Barros de Almeida** & **Alcimar Barbosa Soares** - Federal University of Uberlandia, Brazil, Electrical Engineering Department.

---
