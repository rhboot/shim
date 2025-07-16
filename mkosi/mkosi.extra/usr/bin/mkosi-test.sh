#!/bin/bash -eux

# Ensure secure boot is enabled and not in setup mode
cmp /sys/firmware/efi/efivars/SecureBoot-8be4df61-93ca-11d2-aa0d-00e098032b8c <(printf '\6\0\0\0\1')
cmp /sys/firmware/efi/efivars/SetupMode-8be4df61-93ca-11d2-aa0d-00e098032b8c <(printf '\6\0\0\0\0')

# Check that we didn't accidentally install the microsoft-signed distro shim instead of the local one
if [ -d /boot/EFI ] && command -v sbverify >/dev/null 2>&1; then
    sbverify --list /boot/EFI/BOOT/BOOT*.EFI | grep -q mkosi
    sbverify --list /boot/EFI/BOOT/mm*.efi | grep -q mkosi
    sbverify --list /boot/EFI/BOOT/grub* | grep -v -q mkosi
fi

# Verify mok-signed UKI addon was loaded correctly
if SYSTEMD_UTF8=0 bootctl status | grep -q "+ Pick up .cmdline from addons"; then
    grep -q foobarbaz /proc/cmdline
fi
