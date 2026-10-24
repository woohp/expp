#!/bin/bash
set -euo pipefail

OUTPUT="expp.hpp"
SRC_DIR="include/expp"

# Ordered so each header's dependencies come before it.
FILES=(
    "generator.hpp"
    "type_cast_fwd.hpp"
    "atom.hpp"
    "binary.hpp"
    "resource.hpp"
    "casts.hpp"
    "ext_types.hpp"
    "process.hpp"
    "stl.hpp"
    "gleam.hpp"
    "yielding.hpp"
    "expp.hpp"
)

echo "Bundling headers into $OUTPUT..."

{
    echo "#pragma once"
    echo ""
    echo "// Combined expp bundle"
    echo ""

    # Hoist all system includes, sorted and deduplicated
    (cd "$SRC_DIR" && grep -h "^#include <" "${FILES[@]}") | sort -u
    echo ""

    # Append file contents, removing #pragma once and all includes
    for file in "${FILES[@]}"; do
        echo "// --- $file ---"
        grep -v -e "^#pragma once" -e "^#include <" -e '^#include "' "$SRC_DIR/$file"
        echo ""
    done
} >"$OUTPUT"

echo "Done. Created $OUTPUT"
