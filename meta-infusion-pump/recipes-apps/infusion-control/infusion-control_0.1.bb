SUMMARY = "Software de Controle da Bomba de Infusão"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# Dependências de Runtime
RDEPENDS:${PN} = "python3 python3-paho-mqtt rpi-gpio"

# Usar classe Systemd ---
inherit systemd

# Apontar para o arquivo .service ---
SRC_URI = " \
    file://listener.py \
    file://infusion-pump.service \
"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${S}/listener.py ${D}${bindir}/infusion-pump-app
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${S}/infusion-pump.service ${D}${systemd_system_unitdir}/infusion-pump.service
}

SYSTEMD_SERVICE:${PN} = "infusion-pump.service"

# Habilita o serviço automaticamente no boot
SYSTEMD_AUTO_ENABLE = "enable"
