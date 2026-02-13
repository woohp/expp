#!/bin/bash

OUTPUT="expp.hpp"
SRC_DIR="src"

# Define the order to ensure dependencies are met
# 1. Forward declarations and basic types
# 2. Type casting system
# 3. STL and extension types
# 4. Coroutines/Yielding
# 5. Core NIF wrapper
FILES=(
    "generator.hpp"
    "type_cast_fwd.hpp"
    "atom.hpp"
    "binary.hpp"
    "resource.hpp"
    "casts.hpp"
    "ext_types.hpp"
    "stl.hpp"
    "yielding.hpp"
    "expp.hpp"
)

echo "Bundling headers into $OUTPUT..."

# Start with a clean file
echo "#pragma once" >"$OUTPUT"
echo "" >>"$OUTPUT"
echo "// Combined expp bundle" >>"$OUTPUT"
echo "" >>"$OUTPUT"

# Extract all system includes, sort them, and deduplicate
tmp_includes=$(mktemp)
for file in "${FILES[@]}"; do
    if [[ -f "$SRC_DIR/$file" ]]; then
        grep "^#include <" "$SRC_DIR/$file" >>"$tmp_includes"
    fi
done

sort -u "$tmp_includes" >>"$OUTPUT"
rm "$tmp_includes"

echo "" >>"$OUTPUT"

# Append file contents, removing #pragma once and local includes
for file in "${FILES[@]}"; do
    if [[ -f "$SRC_DIR/$file" ]]; then
        echo "// --- $file ---" >>"$OUTPUT"
        # Filter out:
        # 1. #pragma once
        # 2. #include <...> (already handled)
        # 3. #include "..." (local includes)
        grep -v "^#pragma once" "$SRC_DIR/$file" |
            grep -v "^#include <" |
            grep -v '^#include "' >>"$OUTPUT"
        echo "" >>"$OUTPUT"
    fi
done

echo "Done. Created $OUTPUT"
chmod +x "$OUTPUT" # Optional: make it readable/executable if needed
