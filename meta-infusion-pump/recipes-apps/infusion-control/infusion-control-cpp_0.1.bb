SUMMARY = "Infusion Pump Firmware (C++)"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# Dependências de Compilação (Build-time)
DEPENDS = "glibc libgpiod paho-mqtt-cpp systemd"
LDFLAGS += "-lsystemd"

# Dependências de Execução (Runtime)
RDEPENDS:${PN} = "libgpiod paho-mqtt-cpp systemd"

SRC_URI = " \
    file://listener.cpp \
    file://infusion-pump.service \
"

S = "${WORKDIR}"

# Comando de compilação dentro do Yocto (usando as variáveis do Bitbake)
do_compile() {
    ${CXX} ${CXXFLAGS} ${LDFLAGS} ${S}/listener.cpp -o infusion-pump-bin -lgpiod -lpaho-mqttpp3 -lpaho-mqtt3as -lsystemd
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 infusion-pump-bin ${D}${bindir}/infusion-pump-app
    
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${S}/infusion-pump.service ${D}${systemd_system_unitdir}/infusion-pump.service
}

inherit systemd
SYSTEMD_SERVICE:${PN} = "infusion-pump.service"
SYSTEMD_AUTO_ENABLE = "enable"