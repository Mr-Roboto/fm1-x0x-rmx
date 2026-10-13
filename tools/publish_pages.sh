#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Prepare a package-free X0X-RMX site for review. This script never pushes or deploys.
# Future publication instructions: docs/PAGES.md.
#   tools/publish_pages.sh build/pages
set -eu
cd "$(dirname "$0")/.."
[ "$#" -eq 1 ] || { echo "usage: tools/publish_pages.sh build/OUTPUT_DIR (local preparation only)" >&2; exit 1; }
[ "$(git branch --show-current)" = orb-remix ] || { echo "prepare from orb-remix" >&2; exit 1; }
sh web/emu/build.sh
python3 tools/make_pages.py --preview "$1"
echo "Prepared $1 locally. Review docs/PAGES.md; nothing was committed, pushed or deployed."
