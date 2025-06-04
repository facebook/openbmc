#!/bin/env bash
set -exuo pipefail

if [[ $# != 1 ]]; then
    echo "Usage: $0 <git tag/branch/commit>" >&2
    exit 1
fi
tag="$1"

cd "$(dirname "$0")"

git fetch --tags

# Clean repo and submodules
git clean -xfd
git submodule foreach --recursive git clean -xfd
git reset --hard
git submodule foreach --recursive git reset --hard
git submodule update --recursive

# Pull latest helium
git checkout helium
git reset --hard origin/helium
git pull origin helium --rebase
git submodule update --recursive .
git clean -xdf

# Checkout desired tag and update submodules
git checkout "$tag"
git submodule update --recursive .
git clean -xdf
git submodule foreach --recursive git clean -xfd
git reset --hard
git submodule foreach --recursive git reset --hard
git submodule update --recursive .
