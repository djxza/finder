#!/bin/bash

OUT=build_dump.txt
echo "The tree is excluding libs due to a large amount of files inside"
echo "$OUT ..."
echo "" > "$OUT"

log() {
    echo -e "\n===== $1 =====\n" | tee -a "$OUT"
}

log "TREE"
tree -a -I '.git|bin|.cache|lib' | tee -a "$OUT"

log "MAKEFILE"
sed -n '1,300p' makefile | tee -a "$OUT"

log "SOURCE FILES"
find src -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \) \
-exec sh -c 'echo "\n--- {} ---"; sed -n "1,300p" {}' \; | tee -a "$OUT"

cat $OUT | xclip -selection clipboard

echo
echo "Done. Upload or paste build_dump.txt"
