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
    devmem2 \
    mosquitto \
    mosquitto-clients \
    libgpiod \
    libgpiod-tools \
    paho-mqtt-c \
    paho-mqtt-cpp \
    boost-system \
    boost-thread \
    boost-mqtt5-dev \
    infusion-control-cpp \                  
"

IMAGE_FEATURES += "read-only-rootfs"
