SUMMARY = "Bundle de Atualizacao OTA para Bomba de Infusao"
LICENSE = "MIT"

inherit bundle

# Compatibilidade (Deve bater com o 'compatible' do 'rauc status')
RAUC_BUNDLE_COMPATIBLE = "infusion-pump-rpi"

# Versão do Pacote
RAUC_BUNDLE_VERSION = "v1.0.1"
RAUC_BUNDLE_DESCRIPTION = "Atualizacao OTA Teste v1.0.1"

# O que vai dentro do pacote?
RAUC_BUNDLE_SLOTS = "rootfs"

# Qual imagem usar para preencher o slot rootfs?
RAUC_SLOT_rootfs = "infusion-image"
RAUC_SLOT_rootfs[fstype] = "ext4"

# Certificados de Teste (Padrao do meta-rauc)
# Na producao voce usaria os seus proprios
RAUC_KEY_FILE = "${THISDIR}/files/development-1.key.pem"
RAUC_CERT_FILE = "${THISDIR}/files/development-1.cert.pem"
RAUC_BUNDLE_FORMAT = "verity"
#RAUC_BUNDLE_FORMAT = "plain"
