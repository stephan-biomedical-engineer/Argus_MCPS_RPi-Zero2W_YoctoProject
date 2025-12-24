FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://25-wlan.network \
    file://watchdog.conf \
"

do_install:append() {
    # --- Configuração de Rede (Já existia) ---
    install -d ${D}${sysconfdir}/systemd/network
    install -m 0644 ${WORKDIR}/25-wlan.network ${D}${sysconfdir}/systemd/network/

    # Cria a pasta especial de configuração do Systemd Manager
    install -d ${D}${sysconfdir}/systemd/system.conf.d
    
    # Instala o arquivo. O prefixo '00-' ou '50-' ajuda na ordem, 
    install -m 0644 ${WORKDIR}/watchdog.conf ${D}${sysconfdir}/systemd/system.conf.d/50-watchdog.conf
}

# Avisa o bitbake que instalamos arquivos nesses locais
FILES:${PN} += " \
    ${sysconfdir}/systemd/network \
    ${sysconfdir}/systemd/system.conf.d \
"
