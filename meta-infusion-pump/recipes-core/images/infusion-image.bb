SUMMARY = "Imagem Oficial da Bomba de Infusão"
LICENSE = "MIT"

# Baseia na core-image-minimal (mas turbinada)
require recipes-core/images/core-image-minimal.bb

FILESEXTRAPATHS:prepend := "${THISDIR}:"

# kernel-module-x-bcm2835 instead kernel-modules (full)

# --- PACOTES DO SISTEMA ---
IMAGE_INSTALL:append = " \
    linux-firmware-rpidistro-bcm43436 \
    linux-firmware-rpidistro-bcm43430 \
    kernel-module-brcmfmac \
    wpa-supplicant \
    openssl \
    openssl-bin \
    ca-certificates \
    ntp \
    tzdata \
    tzdata-americas \
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
    boost-json \
    boost-mqtt5-dev \
    rauc \
    rauc-mark-good \
    u-boot-fw-utils \
    util-linux \
    dosfstools \
    argus-control-cpp \
"
PACKAGECONFIG:append:pn-systemd = " timesyncd"
PACKAGECONFIG:append:pn-mosquitto = " tls"
IMAGE_FEATURES += "read-only-rootfs"

WKS_FILE = "infusion-ab.wks"

BAD_RECOMMENDATIONS += "rauc-conf"

DEPENDS += "u-boot-script-infusion"

IMAGE_BOOT_FILES:append = " \
    boot-rauc.scr;boot.scr \
    ${KERNEL_IMAGETYPE};zImage \
    bcm2710-rpi-zero-2-w.dtb;bcm2710-rpi-zero-2-w.dtb \
    overlays/*;overlays/ \
"

create_version_file() {
    echo "Version 1.0.1 - Update by Over-The-Air" > ${IMAGE_ROOTFS}/etc/ota_version.txt
    date >> ${IMAGE_ROOTFS}/etc/ota_version.txt
}

ROOTFS_POSTPROCESS_COMMAND += "create_version_file; "
IMAGE_FSTYPES += " ext4"
