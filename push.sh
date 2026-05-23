#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

REMOTE="${1:-origin}"
BRANCH="$(git branch --show-current)"

echo "=== 1/4 — Prerequisites check ==="
for path in import/glad/src/glad.c shader/phong.vert assets/objects/cube.obj; do
  if [[ ! -f "$path" ]]; then
    echo "Missing file: $path" >&2
    exit 1
  fi
done

echo "=== 2/4 — Project build ==="
"$ROOT/build.sh"

echo "=== 3/4 — Git check ==="
STAGED_LABS="$(git diff --cached --name-only | grep -E '^LAB0[1-4]/' || true)"
if [[ -n "$STAGED_LABS" ]]; then
  echo "Error: LAB folders are staged in Git:" >&2
  echo "$STAGED_LABS" >&2
  echo "Unstage them with: git reset HEAD LAB0X/" >&2
  exit 1
fi

if git diff --quiet && git diff --cached --quiet && [[ -z "$(git ls-files --others --exclude-standard)" ]]; then
  echo "No local changes to push."
else
  echo "Uncommitted changes detected:"
  git status --short
  echo ""
  echo "Commit first, for example:"
  echo "  git add -A"
  echo "  git commit -m \"Describe your change\""
  echo "  ./push.sh"
  exit 1
fi

echo "=== 4/4 — Push to $REMOTE/$BRANCH ==="
git push "$REMOTE" "$BRANCH"
echo ""
echo "Push complete."
