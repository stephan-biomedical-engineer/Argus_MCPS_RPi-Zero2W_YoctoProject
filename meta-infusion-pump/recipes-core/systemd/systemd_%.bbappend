FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://25-wlan.network \
    file://watchdog.conf \
    file://timesyncd.conf \
"

do_install:append() {

    # --- tmpfiles.d ---
    install -d ${D}${sysconfdir}/tmpfiles.d
    install -m 0644 ${WORKDIR}/timesyncd.conf \
        ${D}${sysconfdir}/tmpfiles.d/timesyncd.conf

    # --- Configuração de Rede ---
    install -d ${D}${sysconfdir}/systemd/network
    install -m 0644 ${WORKDIR}/25-wlan.network \
        ${D}${sysconfdir}/systemd/network/

    # --- Watchdog ---
    install -d ${D}${sysconfdir}/systemd/system.conf.d
    install -m 0644 ${WORKDIR}/watchdog.conf \
        ${D}${sysconfdir}/systemd/system.conf.d/50-watchdog.conf

    # --- Fix timesyncd persistente ---
    rm -rf ${D}${localstatedir}/lib/systemd/timesync
    ln -s /data/timesync ${D}${localstatedir}/lib/systemd/timesync
}

FILES:${PN} += " \
    ${sysconfdir}/systemd/network \
    ${sysconfdir}/systemd/system.conf.d \
    ${sysconfdir}/tmpfiles.d \
"
