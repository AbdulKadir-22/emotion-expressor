#!/usr/bin/env bash
set -euo pipefail

# Emotion Expressor Uninstaller
# Usage:
#   bash uninstall.sh
#   EMOTION_EXPRESSOR_PREFIX="~/.local" bash uninstall.sh

PREFIX="${EMOTION_EXPRESSOR_PREFIX:-${HOME}/.local}"

GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}[INFO]${NC} Uninstalling Emotion Expressor from ${PREFIX}..."

# Stop any running process
if pgrep -f "emotion_expressor" &>/dev/null; then
    echo -e "${BLUE}[INFO]${NC} Stopping running emotion_expressor process..."
    pkill -f "emotion_expressor" || true
fi

# Remove application files
rm -f "${PREFIX}/bin/emotion_expressor"
rm -f "${PREFIX}/bin/emoji_cli"
rm -f "${PREFIX}/share/applications/in.abdulkadir.clipmoji.desktop"
rm -f "${PREFIX}/share/metainfo/in.abdulkadir.clipmoji.metainfo.xml"
rm -f "${PREFIX}/share/icons/hicolor/scalable/apps/in.abdulkadir.clipmoji.svg"
rm -rf "${PREFIX}/share/emotion-expressor"

# Remove autostart entry
rm -f "${HOME}/.config/autostart/emotion-expressor.desktop"

echo -e "${GREEN}[SUCCESS]${NC} Emotion Expressor files have been safely removed."
echo -e "Note: User configuration at ~/.config/emoji-picker/ was preserved. Remove manually if desired:"
echo -e "      rm -rf ~/.config/emoji-picker"
