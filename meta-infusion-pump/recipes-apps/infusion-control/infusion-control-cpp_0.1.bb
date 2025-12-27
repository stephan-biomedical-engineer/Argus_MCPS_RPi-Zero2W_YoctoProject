SUMMARY = "Infusion Pump Firmware (C++)"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# Dependências de Compilação (Build-time)
# Mantemos o paho-mqtt-cpp no DEPENDS apenas para o sysroot tê-lo disponível
DEPENDS = "glibc libgpiod paho-mqtt-cpp systemd boost boost-mqtt5"

# Dependências de Execução (Runtime)
# Mantemos paho-mqtt-cpp aqui para que as .so estejam na imagem caso você precise
# rodar um binário antigo manualmente para testes.
RDEPENDS:${PN} = "libgpiod paho-mqtt-cpp systemd boost-system boost-thread"

SRC_URI = " \
    file://listener.cpp \
    file://infusion-pump.service \
"

S = "${WORKDIR}"

# Comando de compilação atualizado
do_compile() {
    ${CXX} ${CXXFLAGS} ${LDFLAGS} ${S}/listener.cpp -o infusion-pump-bin \
    -lgpiod -lsystemd -lboost_system -lboost_thread -lpthread
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