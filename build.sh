#!/usr/bin/env bash
# =============================================================================
# Argus MCPS — Script de Build via kas-container
# =============================================================================
# Uso:
#   ./build.sh image      Gera imagem do SD Card (infusion-image)
#   ./build.sh bundle     Gera pacote OTA (infusion-bundle)
#   ./build.sh all        Gera imagem + bundle
#   ./build.sh shell      Abre shell interativo no container Kas
#   ./build.sh clean      Remove artefatos de build
#   ./build.sh --help     Mostra ajuda completa
# =============================================================================
set -euo pipefail

# ---------------------------------------------------------------------------
# Constantes
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KAS_DIR="${SCRIPT_DIR}/kas"
LAYER_DIR="${SCRIPT_DIR}/meta-infusion-pump"

# Determinar o executável do Kas (usar local se existir)
KAS_BIN="kas-container"
if [ -f "${SCRIPT_DIR}/kas-container" ]; then
    KAS_BIN="${SCRIPT_DIR}/kas-container"
fi

# Cache persistente (pode ser sobrescrito via variáveis de ambiente)
export DL_DIR="${DL_DIR:-${SCRIPT_DIR}/build/downloads}"
export SSTATE_DIR="${SSTATE_DIR:-${SCRIPT_DIR}/build/sstate-cache}"

# Cores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# ---------------------------------------------------------------------------
# Funções de utilidade
# ---------------------------------------------------------------------------
log_ok()   { echo -e "${GREEN}[OK]${NC}    $*"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC}  $*"; }
log_err()  { echo -e "${RED}[ERRO]${NC}  $*"; }
log_info() { echo -e "${CYAN}[INFO]${NC}  $*"; }

banner() {
    echo -e "${CYAN}"
    echo "╔═══════════════════════════════════════════════════════════╗"
    echo "║  Argus MCPS — Bomba de Infusão IoT                      ║"
    echo "║  Build System (Kas/Yocto Scarthgap)                     ║"
    echo "╚═══════════════════════════════════════════════════════════╝"
    echo -e "${NC}"
}

usage() {
    echo -e "${BOLD}Uso:${NC} $0 <comando> [opções]"
    echo ""
    echo -e "${BOLD}Comandos:${NC}"
    echo "  image       Compila a imagem do SD Card (infusion-image)"
    echo "  bundle      Compila o pacote de atualização OTA (infusion-bundle)"
    echo "  all         Compila imagem + bundle sequencialmente"
    echo "  shell       Abre um shell interativo dentro do container Kas"
    echo "  clean       Remove o diretório build/"
    echo ""
    echo -e "${BOLD}Opções:${NC}"
    echo "  --no-certs  Pula a verificação/geração de certificados"
    echo "  --help, -h  Mostra esta mensagem"
    echo ""
    echo -e "${BOLD}Variáveis de Ambiente:${NC}"
    echo "  DL_DIR      Diretório de downloads (default: build/downloads)"
    echo "  SSTATE_DIR  Diretório de sstate-cache (default: build/sstate-cache)"
    echo ""
    echo -e "${BOLD}Exemplos:${NC}"
    echo "  $0 image                       # Imagem SD Card"
    echo "  $0 bundle                      # Pacote OTA"
    echo "  $0 all                         # Tudo"
    echo "  $0 shell                       # Shell interativo"
    echo "  DL_DIR=/mnt/cache/dl $0 image  # Cache externo"
}

# ---------------------------------------------------------------------------
# Verificar submódulos antigos
# ---------------------------------------------------------------------------
check_legacy_submodules() {
    local legacy_dirs=("poky" "meta-raspberrypi" "meta-openembedded" "meta-rauc")
    local found_legacy=()
    for d in "${legacy_dirs[@]}"; do
        if [ -d "${SCRIPT_DIR}/$d" ]; then
            found_legacy+=("$d")
        fi
    done

    if [ ${#found_legacy[@]} -gt 0 ]; then
        log_warn "Diretórios de submódulos antigos detectados: ${found_legacy[*]}"
        log_warn "Isso causará o erro 'Repo is dirty - no checkout' no Kas."
        log_warn "Para corrigir, execute o comando abaixo e rode ./build.sh novamente:"
        echo -e "    ${BOLD}rm -rf ${found_legacy[*]}${NC}"
        echo ""
    fi
}

# ---------------------------------------------------------------------------
# Pré-requisitos
# ---------------------------------------------------------------------------
check_prerequisites() {
    local ok=true

    check_legacy_submodules

    if [ -f "$KAS_BIN" ]; then
        log_ok "Kas: usando executável local ($KAS_BIN)"
    elif command -v kas-container &>/dev/null; then
        log_ok "Kas: encontrado no PATH (sistema)"
    else
        log_err "kas-container não encontrado no PATH nem localmente."
        echo ""
        echo "    Instale com:"
        echo "      pip3 install kas"
        echo ""
        echo "    Ou baixe localmente:"
        echo "      curl -fsSL https://raw.githubusercontent.com/siemens/kas/master/kas-container -o kas-container"
        echo "      chmod +x kas-container"
        ok=false
    fi

    # Verificar container engine (docker ou podman)
    if command -v docker &>/dev/null; then
        log_ok "Container engine: docker"
    elif command -v podman &>/dev/null; then
        log_ok "Container engine: podman"
    else
        log_err "Docker ou Podman não encontrado. kas-container precisa de um deles."
        ok=false
    fi

    if [ "$ok" = false ]; then
        exit 1
    fi
}

# ---------------------------------------------------------------------------
# Certificados
# ---------------------------------------------------------------------------
check_certificates() {
    local needs_gen=false

    # Arquivos críticos que devem existir e ter conteúdo real (>10 bytes)
    local cert_files=(
        "${LAYER_DIR}/recipes-connectivity/mosquitto/files/certs/ca.crt"
        "${LAYER_DIR}/recipes-connectivity/mosquitto/files/certs/server.crt"
        "${LAYER_DIR}/recipes-connectivity/mosquitto/files/certs/server.key"
        "${LAYER_DIR}/recipes-core/bundles/files/development-1.key.pem"
        "${LAYER_DIR}/recipes-core/bundles/files/development-1.cert.pem"
        "${LAYER_DIR}/recipes-support/rauc/files/keyring.pem"
    )

    for cert in "${cert_files[@]}"; do
        if [ ! -f "$cert" ] || [ "$(wc -c < "$cert" 2>/dev/null)" -le 10 ]; then
            needs_gen=true
            break
        fi
    done

    if [ "$needs_gen" = true ]; then
        log_warn "Certificados de desenvolvimento ausentes ou são placeholders."
        log_info "Gerando certificados de desenvolvimento..."
        echo ""
        bash "${LAYER_DIR}/generate_dev_certs.sh"
        echo ""
        log_ok "Certificados gerados com sucesso."
    else
        log_ok "Certificados de desenvolvimento encontrados."
    fi
}

# ---------------------------------------------------------------------------
# Preparação de cache
# ---------------------------------------------------------------------------
prepare_cache() {
    mkdir -p "${DL_DIR}" "${SSTATE_DIR}"
    log_ok "DL_DIR     = ${DL_DIR}"
    log_ok "SSTATE_DIR = ${SSTATE_DIR}"
}

# ---------------------------------------------------------------------------
# Comandos de build
# ---------------------------------------------------------------------------
run_build() {
    local config="$1"
    local label="$2"

    echo ""
    log_info "Iniciando build: ${BOLD}${label}${NC}"
    log_info "Config: ${config}"
    echo ""

    "$KAS_BIN" build "${config}"

    echo ""
    log_ok "Build concluído: ${label}"
}

run_shell() {
    echo ""
    log_info "Abrindo shell interativo no container Kas..."
    log_info "Use 'bitbake <target>' para builds manuais."
    echo ""

    "$KAS_BIN" shell "${KAS_DIR}/image.yml"
}

run_clean() {
    local build_dir="${SCRIPT_DIR}/build"
    if [ -d "$build_dir" ]; then
        log_warn "Removendo ${build_dir}/ ..."
        rm -rf "$build_dir"
        log_ok "Limpeza concluída."
    else
        log_ok "Nada para limpar (build/ não existe)."
    fi
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
main() {
    local command=""
    local machine_target="raspberrypi0-2w" # default
    local skip_certs=false

    # Parse argumentos
    while [[ $# -gt 0 ]]; do
        case "$1" in
            image|bundle|all|shell|clean)
                command="$1"
                shift
                ;;
            raspberrypi3|raspberrypi0-2w)
                machine_target="$1"
                shift
                ;;
            --no-certs)
                skip_certs=true
                shift
                ;;
            --help|-h)
                banner
                usage
                exit 0
                ;;
            *)
                log_err "Argumento desconhecido: $1"
                echo ""
                usage
                exit 1
                ;;
        esac
    done

    banner

    if [ -z "$command" ]; then
        usage
        exit 1
    fi

    # Clean não precisa de pré-requisitos
    if [ "$command" = "clean" ]; then
        run_clean
        exit 0
    fi

    # Verificações
    check_prerequisites

    if [ "$skip_certs" = false ]; then
        check_certificates
    fi

    prepare_cache

    local machine_yml="${KAS_DIR}/machines/${machine_target}.yml"
    if [ ! -f "$machine_yml" ]; then
        log_err "Target de maquina invalido: $machine_target. Arquivo $machine_yml nao encontrado."
        exit 1
    fi

    # Execução
    case "$command" in
        image)
            run_build "${machine_yml}:${KAS_DIR}/image.yml" "Imagem SD Card (infusion-image) - ${machine_target}"
            ;;
        bundle)
            run_build "${machine_yml}:${KAS_DIR}/bundle.yml" "Bundle OTA (infusion-bundle) - ${machine_target}"
            ;;
        all)
            run_build "${machine_yml}:${KAS_DIR}/image.yml" "Imagem SD Card (infusion-image) - ${machine_target}"
            run_build "${machine_yml}:${KAS_DIR}/bundle.yml" "Bundle OTA (infusion-bundle) - ${machine_target}"
            ;;
        shell)
            "$KAS_BIN" shell "${machine_yml}:${KAS_DIR}/image.yml"
            ;;
    esac

    echo ""
    echo -e "${GREEN}${BOLD}✔ Concluído com sucesso!${NC}"
}

main "$@"
