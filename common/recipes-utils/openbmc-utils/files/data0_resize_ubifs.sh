#!/bin/bash

ubifs_mount() {
    ubi_vol="$1"
    mnt_point="$2"

    echo "ubifs_mount $ubi_vol to $mnt_point.."
    mount -t ubifs "$ubi_vol" "$mnt_point" -o sync,compr=zstd
}

ubifs_resize() {
    ubi_dev="$1"
    vol_id="$2"
    ubi_vol="${ubi_dev}_${vol_id}"
    mnt_point="$3"
    backup_dir="/tmp/mnt_data_ubifs_backup"
    ubi_dev_avali_leb=$(ubinfo "$ubi_dev" | grep "available logical eraseblocks" | awk -F':' '{print $2}' | awk '{print $1}')
    ubi_vol_leb=$(ubinfo "$ubi_vol" | grep "Size" | awk -F':' '{print $2}' | awk '{print $1}')
    new_vol_leb=$((ubi_dev_avali_leb+ubi_vol_leb))

    echo "available leb:    $ubi_dev_avali_leb"
    echo "volume leb:       $ubi_vol_leb"

    if [ "$ubi_dev_avali_leb" -eq 0 ]; then
        # no more avaliable LEBs
        echo "no more avaliable LEBs"
        return 0
    fi

    if ! /bin/mountpoint "$mnt_point"; then
        echo "$mnt_point is not a correct mount point"
        return 1
    fi

    # prepare backup data folder
    if [ -e "$backup_dir" ]; then
        echo "backup folder already exist, please move it to safe place"
        echo "backup folder: $backup_dir"
        return 1
    fi
    cp -ar "$mnt_point" "$backup_dir"

    # umount mount point
    if ! umount "$mnt_point"; then
        rm -rf "$backup_dir"
        echo "umount $mnt_point failed"
        return 1
    fi

    # do resize
    if ! ubirsvol "$ubi_dev" -n "$vol_id" -S "$new_vol_leb"; then
        echo "ubifs volume resize failed (ubirsvol), err_code: $?"
    elif ! mkfs.ubifs -y -r "$backup_dir" "$ubi_vol"; then
        echo "ubifs volume resize failed (mkfs.ubifs), err_code: $?"
    elif ! ubifs_mount "$ubi_vol" "$mnt_point"; then
        echo "ubifs volume resize failed (ubifs_mount), err_code: $?"
    else
        # resize done
        echo "ubifs volume resize done, vol: $ubi_vol, origin LEBs: $ubi_vol_leb, new LEBs: $new_vol_leb"
        rm -rf "$backup_dir"
        return 0
    fi

    # undo resize
    if ! ubirsvol "$ubi_dev" -n "$vol_id" -S "$ubi_vol_leb"; then
        echo "ubifs volume undo resize failed (ubirsvol), err_code: $?"
    elif ! mkfs.ubifs -y -r "$backup_dir" "$ubi_vol"; then
        echo "ubifs volume undo resize failed (mkfs.ubifs), err_code: $?"
    elif ! ubifs_mount "$ubi_vol" "$mnt_point"; then
        echo "ubifs volume undo resize failed (ubifs_mount), err_code: $?"
    else
        # undo resize done
        echo "ubifs volume undo resize done, vol: $ubi_vol, LEBs: $ubi_vol_leb"
        rm -rf "$backup_dir"
        return 0
    fi

    # undo reise failed
    echo "undo resize failed, backup folder: $backup_dir"
    return 1
}

/usr/bin/killall rsyslogd

ubifs_resize /dev/ubi0 0 /mnt/data
ret_val=$?

/usr/sbin/rsyslogd
exit $ret_val
