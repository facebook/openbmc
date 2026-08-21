#!/bin/bash
KV_STORE=/mnt/data/kv_store

addc_init="${KV_STORE}/addc_init"

# Read Boot Magic
sig=$(devmem 0x10015c08 2>/dev/null)

# Read the Watch Dog Flag
val=$(devmem 0x1e6e2074 2>/dev/null)
if [[ "$sig" != "0xFB420054" && "$((val & 0x1))" == "1" ]]; then
    printf "0" > ${addc_init}
fi

echo "Done"
