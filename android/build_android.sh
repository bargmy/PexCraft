#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
bash ./prepare_sdl2.sh
if [ -x ./gradlew ]; then ./gradlew assembleRelease; else gradle assembleRelease; fi
