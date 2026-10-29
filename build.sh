#!/usr/bin/env bash
# amalgamate.sh - build a single-header library from include/**/*.h
#
# Usage: ./amalgamate.sh        (no arguments)
#
# Place this script in the project root, next to include/. Edit the two
# settings below if your layout differs.
#
# It walks every header under INCLUDE_DIR and:
#   - inlines each local  #include "x.h"  exactly once, at its first use
#     (so dependency order is correct automatically)
#   - hoists top-level  #include <sys>  lines to the top, de-duplicated
#     (includes inside #if/#ifdef blocks stay where they are)
#   - drops '#pragma once' and wraps the result in one generated guard

set -euo pipefail

# ---- settings ---------------------------------------------------------
INCLUDE_DIR="include"
OUT="single/cpp-featured.hpp"
# -----------------------------------------------------------------------

cd "$(dirname "$(realpath "$0")")"

[ -d "$INCLUDE_DIR" ] || { echo "Include dir not found: $INCLUDE_DIR" >&2; exit 1; }

incdir="$(realpath "$INCLUDE_DIR")"
out="$OUT"

declare -A seen sys
sys_order=()
body="$(mktemp)"
trap 'rm -f "$body"' EXIT

re_local='^[[:space:]]*#[[:space:]]*include[[:space:]]*"([^"]+)"'
re_sys='^[[:space:]]*#[[:space:]]*include[[:space:]]*<([^>]+)>'
re_once='^[[:space:]]*#[[:space:]]*pragma[[:space:]]+once'
re_if='^[[:space:]]*#[[:space:]]*(if|ifdef|ifndef)([^[:alnum:]_]|$)'
re_endif='^[[:space:]]*#[[:space:]]*endif([^[:alnum:]_]|$)'

resolve() { # $1 = name, $2 = dir of including file
    local d
    for d in "$2" "$incdir"; do
        if [ -f "$d/$1" ]; then realpath "$d/$1"; return 0; fi
    done
    return 1
}

process() {
    local file="$1" depth=0 line path
    [ -z "${seen[$file]:-}" ] || return 0
    seen[$file]=1

    printf '/* ---- %s ---- */\n' "${file#"$incdir"/}" >> "$body"
    while IFS= read -r line || [ -n "$line" ]; do
        if [[ $line =~ $re_once ]]; then
            continue
        elif [[ $line =~ $re_local ]]; then
            if path="$(resolve "${BASH_REMATCH[1]}" "$(dirname "$file")")"; then
                process "$path"
            else
                printf '%s\n' "$line" >> "$body"   # not ours, keep as is
            fi
        elif [[ $line =~ $re_sys ]] && [ "$depth" -eq 0 ]; then
            local h="${BASH_REMATCH[1]}"
            if [ -z "${sys[$h]:-}" ]; then sys[$h]=1; sys_order+=("$h"); fi
        else
            [[ $line =~ $re_if ]] && depth=$((depth + 1))
            [[ $line =~ $re_endif ]] && depth=$((depth - 1))
            printf '%s\n' "$line" >> "$body"
        fi
    done < "$file"
    printf '\n' >> "$body"
}

# Process every header; dependencies get pulled in first via the includes.
while IFS= read -r f; do
    process "$f"
done < <(find "$incdir" -type f \( -name '*.h' -o -name '*.hpp' \) | LC_ALL=C sort)

guard="$(basename "$out" | tr '[:lower:].-' '[:upper:]__')"
mkdir -p "$(dirname "$out")"
{
    echo "#ifndef ${guard}"
    echo "#define ${guard}"
    echo
    for h in "${sys_order[@]}"; do echo "#include <${h}>"; done
    echo
    cat "$body"
    echo "#endif /* ${guard} */"
} > "$out"

echo "Amalgamated ${#seen[@]} header(s) into $out"