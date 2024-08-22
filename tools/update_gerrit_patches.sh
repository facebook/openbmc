#!/bin/bash


download_change() {
  cid=$1
  outf=$2
  curl -ks "https://gerrit.openbmc.org/changes/$cid/revisions/current/patch?zip" -o tmp.zip
  unzip -p tmp.zip > "$outf"
  rm tmp.zip
}

test_change() {
  cid=$1
  if curl -sk "https://gerrit.openbmc.org/changes/$cid" | grep "Not found: " > /dev/null; then
    return 1
  fi
  return 0
}

patch_status() {
  cid=$1
  curl -sk "https://gerrit.openbmc.org/changes/$cid" | jq -R 'fromjson?' | jq -r .status
}

update_patch() {
  file=$1
  if grep "Change-Id: " "$file" > /dev/null; then
    # Some kernel patches have change ID as well.
    if ! grep "Lore link: " "$file" > /dev/null; then
      change=$(grep "Change-Id: " "$file" | awk '{print $2}')
      if test_change "$change"; then
        patch_status=$(patch_status "$change")
        if [ "$patch_status" == "MERGED" ]; then
          echo "STALE PATCH: $file. Merged upstream please remove/delete"
        else
          echo "Updating $file with $change from gerrit $patch_status"
          download_change "$change" "$file"
        fi
      fi
    fi
  fi
}

update_all() {
  locations=( "common" "meta-facebook" )
  for location in "${locations[@]}"; do
    files=$(find "$location" -name "*.patch")
    for file in $files; do
      update_patch "$file"
    done
  done
}

if [ "$1" == "" ]; then
  update_all
elif [ "$1" == "-h" ]; then
  echo "USAGE: $0 [PATCH]"
  echo "If no options provided, find all and update"
else
  update_patch "$1"
fi
