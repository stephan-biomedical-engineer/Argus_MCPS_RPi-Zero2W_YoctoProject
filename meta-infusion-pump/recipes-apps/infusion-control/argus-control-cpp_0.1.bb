SUMMARY = "Argus Medical Pump Control System Firmware (Final Architecture)"
DESCRIPTION = "Firmware C++ with MQTT, Boost.Asio, HAL and OTA Updater"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# Dependencies
DEPENDS = "boost boost-mqtt5 systemd libgpiod openssl i2c-tools"
RDEPENDS:${PN} = "boost-system boost-thread libgpiod systemd"
TARGET_CC_ARCH += "${LDFLAGS}"

# Files source
SRC_URI = " \
    file://src \
    file://infusion-pump.service \
"

S = "${WORKDIR}/src"
EXTRA_OEMAKE += "STRIP_OPT=''"

# --- COMPILE ---
# Yocto exports environment variables CXX, LDFLAGS, etc.
# The oe_runmake exports them to Makefile.
do_compile() {
    oe_runmake
}

# --- INSTALL ---
do_install() {
    # Install daemon
    install -d ${D}${bindir}
    install -m 0755 ${S}/infusion-pump-app ${D}${bindir}/

    # Install updater
    install -m 0755 ${S}/stm32-updater ${D}${bindir}/

    # Install systemd
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/infusion-pump.service ${D}${systemd_system_unitdir}/infusion-pump.service
}

# Systemd config
inherit systemd
SYSTEMD_SERVICE:${PN} = "infusion-pump.service"
SYSTEMD_AUTO_ENABLE = "enable"
