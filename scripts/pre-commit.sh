#!/usr/bin/env bash
set -euo pipefail

ROOT="$(git rev-parse --show-toplevel)"

# Frontend: lint-staged
cd "$ROOT/frontend"
npx --no lint-staged

# Backend + Compiler: clang-format nos arquivos staged
cd "$ROOT"
STAGED_CPP=$(git diff --cached --name-only --diff-filter=ACM | grep -E '^(backend|compiler)/.*\.(cpp|hpp|h|c)$' || true)

if [ -n "$STAGED_CPP" ]; then
  while IFS= read -r file; do
    before=$(git show :"$file" 2>/dev/null | md5sum)
    clang-format -i --style=file "$ROOT/$file"
    after=$(md5sum < "$ROOT/$file")
    if [ "$before" != "$after" ]; then
      echo "  clang-format: $file"
    fi
    git add "$ROOT/$file"
  done <<< "$STAGED_CPP"
fi
