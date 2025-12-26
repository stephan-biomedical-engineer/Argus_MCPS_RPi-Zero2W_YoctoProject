SUMMARY = "Imagem Oficial da Bomba de Infusão"
LICENSE = "MIT"

# Baseia na core-image-minimal (mas turbinada)
require recipes-core/images/core-image-minimal.bb

# kernel-module-x-bcm2835 instead kernel-modules (full)

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
    kernel-module-pwm-bcm2835 \
    kernel-module-spi-bcm2835 \
    kernel-module-spidev \
    kernel-module-i2c-dev \
"

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

MACHINE_EXTRA_RRECOMMENDS += "kernel-module-pwm-bcm2835"

IMAGE_FEATURES += "read-only-rootfs"
