#!/usr/bin/env bash
# Generate Emacs TAGS for the base layer, project code, and vendored libraries.
# Usage: ./scripts/tags.sh

set -euo pipefail

cd "$(dirname "$0")/.."

# macOS ships a BSD ctags at /usr/bin/ctags with no -e and no -R.
if ! ctags --version 2>/dev/null | grep -qiE "exuberant|universal"; then
    echo "error: need Exuberant or Universal ctags, found BSD ctags or none." >&2
    echo "  macOS:  brew install universal-ctags" >&2
    echo "  Debian: apt install universal-ctags" >&2
    exit 1
fi

echo "Generating tags for src/ and third_party/..."
ctags -e -R src/ third_party/

echo "Tags generated: $(pwd)/TAGS"
echo ""
echo "Add this to your Emacs init:"
echo "  (setq tags-file-name \"$(pwd)/TAGS\")"
echo "  (define-key global-map \"\\C-]\" 'find-tag)"
echo "  (define-key global-map \"\\C-\\M-]\" 'pop-tag-mark)"
