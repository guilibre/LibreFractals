#!/usr/bin/env bash
set -euo pipefail

FRONTEND_DIR="$(git rev-parse --show-toplevel)/frontend"
cd "$FRONTEND_DIR"

npx --no lint-staged
