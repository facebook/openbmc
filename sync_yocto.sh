#!/bin/bash
set -e

obmc_dir=$(realpath "$(dirname "$0")")
. $obmc_dir/yocto_repos.sh

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
git remote add yocto-lf-openbmc \
    https://github.com/openbmc/openbmc.git || true

for branch in ${branches[@]}
do
  # Make the branch if it does not exist.
  if [ ! -d ./yocto/$branch ]; then
    mkdir ./yocto/$branch
  fi

  if [[ ${branch} == lf-* ]]; then
      real_branch="${branch/lf-/}"
  else
      real_branch="${branch}"
  fi

  repos=${branch/-/_}_repos[@]
  for repo in ${!repos}
  do
    repo_name=${repo%%:*}
    commit_id=${repo##*:}

    if [ "lf-openbmc" = "$repo_name" ]; then
        repo_name="lf-openbmc"
        repo_path="./yocto/${branch}"
        repo_patch_var="${real_branch}_openbmc_patch"
    else
        repo_path="./yocto/${branch}/${repo_name}"
        repo_patch_var="${branch}_${repo_name/-/_}_patch"
    fi

    # Remove the repo in the branch
    if [ -d ${repo_path} ]; then
      rm -rf ${repo_path}
    fi

    # Fetch the repo branch
    git fetch yocto-${repo_name} ${real_branch}

    # Add specific commit of repo into yocto/branch/repo/ worktree
    if [ $FETCH_ONLY -eq 0 ]; then
      git worktree add -f ${repo_path} ${commit_id}
      if [ -n "${!repo_patch_var}" ]; then
          for patch in ${!repo_patch_var}; do
              git -C ${repo_path} am \
                $(realpath ./github/yocto/patches/${patch})
          done
      fi
    fi
  done
done

