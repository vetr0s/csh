#!/bin/sh
# Usage: scripts/docs.sh
#
# Builds docs/spec.pdf from docs/spec.typ. The PDF is deliberately not in git: a
# rebuild is ~200 KB and PDFs do not delta-compress, so every edit would add a
# fresh copy and the artifact would dwarf the source it came from.
#
# docs/spec.typ is plain text. Read it directly if you have no typst.
# Override the compiler with e.g. TYPST=/opt/homebrew/bin/typst.

set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root_dir=$(dirname -- "$script_dir")

TYPST="${TYPST:-typst}"

if ! command -v "$TYPST" >/dev/null 2>&1
then
    echo "docs.sh: $TYPST not found. See https://github.com/typst/typst" >&2
    exit 1
fi

set -x
$TYPST compile "$root_dir/docs/spec.typ" "$root_dir/docs/spec.pdf"
