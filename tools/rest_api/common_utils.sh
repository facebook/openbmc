#!/bin/bash
# shellcheck disable=SC2044

pardir=$(git rev-parse --show-toplevel)
common_src_dir="$pardir/common/recipes-rest/rest-api/files"

function validate_environment(){
    if [[ ! "$BBPATH" ]]; then echo "No BBPATH in environment. Have you entered a bitbake build-env?" >&2; return 1; fi
    if [[ ! -x  /bin/inotifywait ]]; then echo "inotifywait is not available.  Install inotify-tools" >&2; return 1; fi
}

function  get_build_platform(){
    BUILD_PLATFORM=$(grep BBFILE_COLLECTIONS "$TEMPLATECONF/layer.conf"| cut -d'"' -f 2)
    echo "$BUILD_PLATFORM"
}


function get_platform_recipe_path(){
    if [[ $# != 1 ]]; then echo "Usage: get_platform_recipe_path <build_name>" >&2; return 1; fi
    plat="$1"
    if [[ -d "$pardir/meta-facebook/meta-$plat/recipes-utils/rest-api/files" ]];
    then
        # this is a network device.
        echo "$pardir/meta-facebook/meta-$plat/recipes-utils/rest-api/files"
        return 0
    fi
    if [[ -d "$pardir/meta-facebook/meta-$plat/recipes-$plat/rest-api/files" ]];
    then
        # this is a compute device.
        echo "$pardir/meta-facebook/meta-$plat/recipes-$plat/rest-api/files"
        return 0
    fi
    echo "ERROR: Could not find rest-api/files path for platform '$plat'" >&2
    return 1
}

function prepare_working_directory(){
    (
        if [[ $# != 1 ]]; then echo "Usage: prepare_working_directory <plat_specific_src_dir>">&2; return 1; fi
        cd "$pardir" || exit 1
        plat_specific_src_dir=$1
        outdir="$BBPATH/tmp/work/x86_64-linux/rest-api-native/"
        # Remove old symlinks
        if [[ -e "$outdir" ]]; then
            for file in $(find "$outdir" -type l -iname '*.py'); do
                org=$(readlink -f "$file" || true)
                rm "$file"
                if [[ -e "$org" ]]; then
                    cp -f "$org" "$file"
                fi
            done
        fi

        # Build test once
        bitbake rest-api-native --force  >/dev/null 2>/dev/null || true

        # Link source files to the output directory so that source changes are reflected immediately
        set +x
        for src_file in $(find "$common_src_dir" -type f | grep -Po '(?<=/rest-api/files/).+$'); do
            src_file_abs="$common_src_dir/$src_file"
            if diff -q "$src_file_abs" "$outdir/"*"/$src_file" >/dev/null 2>/dev/null; then
                ln -svf "$src_file_abs" "$outdir/"*"/$src_file" >/dev/null
            fi
        done
        for src_file in $(find "$plat_specific_src_dir" -type f | grep -Po '(?<=/rest-api/files/).+$'); do
            src_file_abs="$plat_specific_src_dir/$src_file"
            if diff -q "$src_file_abs" "$outdir/"*"/$src_file" >/dev/null 2>/dev/null; then
                ln -svf "$src_file_abs" "$outdir/"*"/$src_file" >/dev/null
            fi
        done

    )
}
