FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://mosquitto.conf \
    file://mosquitto.service.d/override.conf \
"
do_install:append() {
    install -d ${D}${sysconfdir}/mosquitto
    install -m 0644 ${WORKDIR}/mosquitto.conf ${D}${sysconfdir}/mosquitto/mosquitto.conf
    install -d ${D}/var/lib/mosquitto
    install -d ${D}${systemd_system_unitdir}/mosquitto.service.d
    install -m 0644 ${WORKDIR}/mosquitto.service.d/override.conf ${D}${systemd_system_unitdir}/mosquitto.service.d/override.conf
}

# Adiciona a pasta criada à lista de arquivos do pacote principal
FILES:${PN} += " \
    /var/lib/mosquitto \
    ${systemd_system_unitdir}/mosquitto.service.d \
"
