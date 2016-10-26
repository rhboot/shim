#!/bin/sh
#Krzysztof.M.Sywula@intel.com

dir=/media

auto_mount() {
  # exit if directory cannot be made
  [ -d "$dir/$1" ] || mkdir -p "$dir/$1" || exit 1

  # exit if already mounted
  cat /proc/mounts | grep $dir/$1 >/dev/null && exit 1

  # exit if mount failed
  if ! mount -t auto -o sync "/dev/$1" "$dir/$1"; then
    rmdir "$dir/$1"
    exit 1
  fi
}

auto_umount() {
  cat /proc/mounts | grep -q /dev/$1 && umount /dev/$1
  [ -d "$dir/$1" ] && rmdir "$dir/$1"
}

main() {
  case "$ACTION" in
  add|"")
    auto_umount "$MDEV"
    auto_mount "$MDEV"
    ;;
  remove)
    auto_umount "$MDEV"
    ;;
  esac
}

main "$@"
