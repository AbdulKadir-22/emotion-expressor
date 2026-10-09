#!/usr/bin/env bash
set -euo pipefail

# Emotion Expressor Installer for Linux
# Usage:
#   curl -fsSL https://raw.githubusercontent.com/AbdulKadir-22/emotion-expressor/main/install.sh | bash
#   EMOTION_EXPRESSOR_VERSION="v0.1.0" bash install.sh

REPO_OWNER="AbdulKadir-22"
REPO_NAME="emotion-expressor"
DEFAULT_INSTALL_PREFIX="${HOME}/.local"

# Colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

info() {
    echo -e "${BLUE}[INFO]${NC} $*"
}

success() {
    echo -e "${GREEN}[SUCCESS]${NC} $*"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $*"
}

error() {
    echo -e "${RED}[ERROR]${NC} $*" >&2
    exit 1
}

detect_arch() {
    local raw_arch
    raw_arch="$(uname -m)"
    case "${raw_arch}" in
        x86_64|amd64)
            echo "x86_64"
            ;;
        aarch64|arm64)
            echo "aarch64"
            ;;
        *)
            error "Unsupported architecture: ${raw_arch}. Emotion Expressor currently provides official release binaries for x86_64."
            ;;
    esac
}

detect_os() {
    local os
    os="$(uname -s)"
    if [ "${os}" != "Linux" ]; then
        error "Emotion Expressor only supports Linux desktop environments."
    fi
}

check_dependencies() {
    info "Checking runtime dependencies..."
    local missing=()

    if ! command -v curl &>/dev/null && ! command -v wget &>/dev/null; then
        missing+=("curl or wget")
    fi
    if ! command -v tar &>/dev/null; then
        missing+=("tar")
    fi
    if ! command -v sha256sum &>/dev/null && ! command -v shasum &>/dev/null; then
        missing+=("sha256sum")
    fi

    if [ ${#missing[@]} -ne 0 ]; then
        error "Missing required utilities to run installer: ${missing[*]}"
    fi

    # Check for GTK4 runtime shared libraries
    if command -v ldconfig &>/dev/null; then
        if ! ldconfig -p 2>/dev/null | grep -q "libgtk-4.so"; then
            warn "libgtk-4.so was not detected by ldconfig. Make sure GTK4 runtime is installed:"
            warn "  Ubuntu/Debian: sudo apt install libgtk-4-1"
            warn "  Fedora:        sudo dnf install gtk4"
            warn "  Arch Linux:    sudo pacman -S gtk4"
        fi
    fi
}

download_file() {
    local url="$1"
    local dest="$2"

    if command -v curl &>/dev/null; then
        curl -fsSL "${url}" -o "${dest}"
    elif command -v wget &>/dev/null; then
        wget -qO "${dest}" "${url}"
    else
        error "Neither curl nor wget is available."
    fi
}

main() {
    detect_os
    ARCH="$(detect_arch)"
    check_dependencies

    PREFIX="${EMOTION_EXPRESSOR_PREFIX:-${DEFAULT_INSTALL_PREFIX}}"
    VERSION="${EMOTION_EXPRESSOR_VERSION:-latest}"

    info "Installing Emotion Expressor (${VERSION}) for ${ARCH} into ${PREFIX}..."

    if [ "${VERSION}" = "latest" ]; then
        RELEASE_API_URL="https://api.github.com/repos/${REPO_OWNER}/${REPO_NAME}/releases/latest"
        info "Fetching latest release metadata..."
        TMP_JSON="$(mktemp)"
        if ! download_file "${RELEASE_API_URL}" "${TMP_JSON}"; then
            error "Failed to fetch release metadata from GitHub API."
        fi
        TAG_NAME="$(grep -o '"tag_name": "[^"]*' "${TMP_JSON}" | head -n1 | cut -d'"' -f4)"
        rm -f "${TMP_JSON}"
        if [ -z "${TAG_NAME}" ]; then
            error "Could not determine latest release tag."
        fi
        VERSION="${TAG_NAME}"
    fi

    ASSET_NAME="emotion-expressor-${VERSION}-linux-${ARCH}.tar.gz"
    CHECKSUM_NAME="${ASSET_NAME}.sha256"

    DOWNLOAD_BASE_URL="https://github.com/${REPO_OWNER}/${REPO_NAME}/releases/download/${VERSION}"
    ASSET_URL="${DOWNLOAD_BASE_URL}/${ASSET_NAME}"
    CHECKSUM_URL="${DOWNLOAD_BASE_URL}/${CHECKSUM_NAME}"

    TMP_DIR="$(mktemp -d)"
    trap 'rm -rf "${TMP_DIR}"' EXIT

    info "Downloading ${ASSET_NAME}..."
    download_file "${ASSET_URL}" "${TMP_DIR}/${ASSET_NAME}"
    info "Downloading checksum ${CHECKSUM_NAME}..."
    download_file "${CHECKSUM_URL}" "${TMP_DIR}/${CHECKSUM_NAME}"

    info "Verifying SHA-256 checksum..."
    (
        cd "${TMP_DIR}"
        if command -v sha256sum &>/dev/null; then
            sha256sum -c "${CHECKSUM_NAME}"
        else
            shasum -a 256 -c "${CHECKSUM_NAME}"
        fi
    ) || error "Checksum verification failed! The downloaded archive may be corrupted or tampered with."

    info "Extracting release archive..."
    mkdir -p "${TMP_DIR}/extracted"
    tar -xzf "${TMP_DIR}/${ASSET_NAME}" -C "${TMP_DIR}/extracted"

    # Structure check inside archive
    EXTRACTED_ROOT="${TMP_DIR}/extracted"
    if [ -d "${EXTRACTED_ROOT}/emotion-expressor-${VERSION}" ]; then
        EXTRACTED_ROOT="${EXTRACTED_ROOT}/emotion-expressor-${VERSION}"
    fi

    info "Installing binaries and desktop assets..."
    mkdir -p "${PREFIX}/bin"
    mkdir -p "${PREFIX}/share/applications"
    mkdir -p "${PREFIX}/share/metainfo"
    mkdir -p "${PREFIX}/share/icons/hicolor/scalable/apps"
    mkdir -p "${PREFIX}/share/emotion-expressor"

    if [ -f "${EXTRACTED_ROOT}/bin/emotion_expressor" ]; then
        cp -f "${EXTRACTED_ROOT}/bin/emotion_expressor" "${PREFIX}/bin/emotion_expressor"
        chmod +x "${PREFIX}/bin/emotion_expressor"
    fi
    if [ -f "${EXTRACTED_ROOT}/bin/emoji_cli" ]; then
        cp -f "${EXTRACTED_ROOT}/bin/emoji_cli" "${PREFIX}/bin/emoji_cli"
        chmod +x "${PREFIX}/bin/emoji_cli"
    fi

    if [ -d "${EXTRACTED_ROOT}/share/emotion-expressor/data" ]; then
        cp -rf "${EXTRACTED_ROOT}/share/emotion-expressor/data" "${PREFIX}/share/emotion-expressor/"
    fi
    if [ -d "${EXTRACTED_ROOT}/share/emotion-expressor/assets" ]; then
        cp -rf "${EXTRACTED_ROOT}/share/emotion-expressor/assets" "${PREFIX}/share/emotion-expressor/"
    fi

    if [ -f "${EXTRACTED_ROOT}/share/applications/in.abdulkadir.clipmoji.desktop" ]; then
        cp -f "${EXTRACTED_ROOT}/share/applications/in.abdulkadir.clipmoji.desktop" "${PREFIX}/share/applications/in.abdulkadir.clipmoji.desktop"
        sed -i "s|^Exec=.*|Exec=${PREFIX}/bin/emotion_expressor|" "${PREFIX}/share/applications/in.abdulkadir.clipmoji.desktop"
    fi

    if [ -f "${EXTRACTED_ROOT}/share/metainfo/in.abdulkadir.clipmoji.metainfo.xml" ]; then
        cp -f "${EXTRACTED_ROOT}/share/metainfo/in.abdulkadir.clipmoji.metainfo.xml" "${PREFIX}/share/metainfo/in.abdulkadir.clipmoji.metainfo.xml"
    fi

    if [ -f "${EXTRACTED_ROOT}/share/icons/hicolor/scalable/apps/in.abdulkadir.clipmoji.svg" ]; then
        cp -f "${EXTRACTED_ROOT}/share/icons/hicolor/scalable/apps/in.abdulkadir.clipmoji.svg" "${PREFIX}/share/icons/hicolor/scalable/apps/in.abdulkadir.clipmoji.svg"
    fi

    # Set up autostart desktop entry if available
    mkdir -p "${HOME}/.config/autostart"
    if [ -f "${PREFIX}/share/applications/in.abdulkadir.clipmoji.desktop" ]; then
        cp -f "${PREFIX}/share/applications/in.abdulkadir.clipmoji.desktop" "${HOME}/.config/autostart/emotion-expressor.desktop"
    fi

    # Ensure ~/.local/bin is in PATH output notice
    success "Emotion Expressor ${VERSION} successfully installed to ${PREFIX}!"
    echo -e ""
    echo -e "  🚀 Run application daemon / toggle picker: ${GREEN}${PREFIX}/bin/emotion_expressor --toggle${NC}"
    echo -e "  💻 Run CLI emoji search:                  ${GREEN}${PREFIX}/bin/emoji_cli${NC}"
    echo -e "  ⌨️  Global Shortcut:                       ${YELLOW}Ctrl+.${NC} (Active when daemon is running)"
    echo -e ""
    if [[ ":$PATH:" != *":${PREFIX}/bin:"* ]]; then
        warn "Note: ${PREFIX}/bin is not in your current PATH. Add it to your ~/.bashrc or ~/.zshrc:"
        echo -e "      export PATH=\"${PREFIX}/bin:\$PATH\""
    fi
}

main "$@"
