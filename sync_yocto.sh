#!/bin/bash
set -e

repos="rocko warrior"

if [ ! -d ./yocto ]; then
  mkdir ./yocto
fi

git remote add yocto-poky \
    https://git.yoctoproject.org/git/poky || true
git remote add yocto-meta-oe \
    https://github.com/openembedded/meta-openembedded.git || true
git remote add yocto-meta-security \
    https://git.yoctoproject.org/git/meta-security || true

for branch in $repos
do
  # Make the branch if it does not exist.
  if [ ! -d ./yocto/$branch ]; then
    mkdir ./yocto/$branch
  fi
  # Remove the branches.
  if [ -d ./yocto/$branch/poky ]; then
    rm -rf ./yocto/$branch/poky
  fi
  if [ -d ./yocto/$branch/meta-openembedded ]; then
    rm -rf ./yocto/$branch/meta-openembedded
  fi
  if [ -d ./yocto/$branch/meta-security ]; then
    rm -rf ./yocto/$branch/meta-security
  fi

  git fetch yocto-poky $branch
  git fetch yocto-meta-oe $branch
  git fetch yocto-meta-security $branch

  git worktree add yocto/$branch/poky yocto-poky/$branch
  git worktree add yocto/$branch/meta-openembedded yocto-meta-oe/$branch
  git worktree add yocto/$branch/meta-security yocto-meta-security/$branch

done
