#!/bin/bash
set -e

branches=(
  rocko
  warrior
  zeus
)

rocko_repos=(
    poky:5f660914cd
    meta-openembedded:eae996301d
    meta-security:74860b2b61
)

warrior_repos=(
    poky:4c773c7b03
    meta-openembedded:a24acf94d4
    meta-security:4f7be0d252
)

zeus_repos=(
  poky:5531ffc566
  meta-openembedded:9e60d3066
  meta-security:ecd8c30
)

FETCH_ONLY=0
while [[ $# -gt 0 ]]
do
  case $1 in
    -f|--fetch-only)
      FETCH_ONLY=1
      shift
      ;;

    *)
      shift
      ;;
  esac
done

if [ ! -d ./yocto ]; then
  mkdir ./yocto
fi

git remote add yocto-poky \
    https://git.yoctoproject.org/git/poky || true
git remote add yocto-meta-openembedded \
    https://github.com/openembedded/meta-openembedded.git || true
git remote add yocto-meta-security \
    https://git.yoctoproject.org/git/meta-security || true

for branch in ${branches[@]}
do
  # Make the branch if it does not exist.
  if [ ! -d ./yocto/$branch ]; then
    mkdir ./yocto/$branch
  fi

  repos=${branch}_repos[@]
  for repo in ${!repos}
  do
    repo_name=${repo%%:*}
    commit_id=${repo##*:}

    # Remove the repo in the branch
    if [ -d ./yocto/${branch}/${repo_name} ]; then
      rm -rf ./yocto/${branch}/${repo_name}
    fi

    # Fetch the repo branch
    git fetch yocto-${repo_name} ${branch}

    # Add specific commit of repo into yocto/branch/repo/ worktree 
    if [ $FETCH_ONLY -eq 0 ]; then
      git worktree add yocto/${branch}/${repo_name} ${commit_id}
    fi
  done
done

