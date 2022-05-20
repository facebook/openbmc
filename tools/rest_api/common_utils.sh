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


function get_platform_parent_name(){
    # returns either platform parent name or just platrom name if not derived
    platform_name=$1

    case "$platform_name" in
      "fby2-nd")
        echo "fby2"
        ;;
      *)
        echo "$platform_name"
        ;;
    esac
}

function get_platform_meta_path(){
    if [[ $# != 1 ]]; then echo "Usage: get_platform_meta_path <build_name> [<subplatform_name>]" >&2; return 1; fi

    subplat=""
    plat="$1"

    parent_platform_name=$(get_platform_parent_name $platform_name)
    if [[ "$parent_platform_name" ]] && [[ "$parent_platform_name" != "$plat" ]]; then
      subplat="$plat"
      plat=$parent_platform_name
    fi

    if [[ "$subplat" ]] && [[ -d "$pardir/meta-facebook/meta-$plat/meta-$subplat" ]]; then
        echo "$pardir/meta-facebook/meta-$plat/meta-$subplat"
        return 0
    fi
    if [[ -d "$pardir/meta-facebook/meta-$plat" ]]; then
        echo "$pardir/meta-facebook/meta-$plat"
        return 0
    fi
    echo "ERROR: Could not find meta-files path for platform '$plat'" >&2
    return 1
}

function get_platform_recipe_path(){
    plat=$1
    meta_path="$(get_platform_meta_path $plat)"
    retval=$?

    if [[ $retval -gt 0 ]]; then
        return $retval
    fi

    if [[ -d "$meta_path/recipes-utils" ]]; then
        # network device
        echo "$meta_path/recipes-utils"
        return 0
    fi
    if [[ -d "$meta_path/recipes-$plat" ]]; then
        # compute device
        echo "$meta_path/recipes-$plat"
        return 0
    fi

    return 1
}

function get_machine_name(){
    platform_name=$1
    meta_path="$(get_platform_meta_path $1)"
    retval=$?

    if [[ $retval -ne 0 ]]; then
        return $retval
    fi

    machine_name=$(grep ^MACHINE "$meta_path/conf/local.conf.sample" | awk '{print $NF}' | tr -d '"')
    retval=$?

    if [[ $retval -ne 0 ]]; then
        return 1
    fi

    echo "$machine_name"
}

function prepare_working_directory(){
    (
        if [[ $# != 1 ]]; then echo "Usage: prepare_working_directory <plat_specific_src_dir>">&2; return 1; fi
        cd "$pardir" || exit 1
        plat_specific_src_dir=$1
        outdir="$BBPATH/tmp/workarmv*-fb-linux-gnueabi/rest-api/0.1-r1/rest-api-0.1"
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
        bitbake rest-api --force  >/dev/null 2>/dev/null || true

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
