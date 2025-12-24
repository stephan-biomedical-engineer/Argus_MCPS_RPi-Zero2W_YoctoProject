SUMMARY = "Imagem Oficial da Bomba de Infusão"
LICENSE = "MIT"

# Baseia na core-image-minimal (mas turbinada)
require recipes-core/images/core-image-minimal.bb

# --- PACOTES DO SISTEMA ---
IMAGE_INSTALL:append = " \
    linux-firmware-rpidistro-bcm43436 \
    linux-firmware-rpidistro-bcm43430 \
    kernel-module-brcmfmac \
    wpa-supplicant \
    iw \
    wireless-regdb \
    htop \
    nano \
    i2c-tools \
    kernel-modules \
"

# --- PACOTES PYTHON E APLICAÇÃO ---
# python3-modules traz a biblioteca padrão completa (json, threading, etc)
IMAGE_INSTALL:append = " \
    python3 \
    python3-pip \
    python3-modules \
    python3-smbus \
    python3-flask \
    python3-paho-mqtt \
    rpi-gpio \
    devmem2 \
    mosquitto \
    mosquitto-clients \
    libgpiod \
    libgpiod-tools \
    libgpiod-dev \
    paho-mqtt-c \
    paho-mqtt-cpp \
    paho-mqtt-c-dev \
    paho-mqtt-cpp-dev \
    infusion-control-cpp \                  
"

IMAGE_FEATURES += "read-only-rootfs"