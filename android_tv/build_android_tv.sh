#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
./prepare_sdl2.sh
./gradlew assembleRelease 2>/dev/null || gradle assembleRelease
