#!/bin/sh

load_drivers()
{
    while IFS= read -r line; do
      modprobe $line
    done < "/etc/modules-load.quark/$1.conf"
}

do_board()
{
    type dmidecode > /dev/null 2>&1 || die "dmidecode not installed"
    board=$(dmidecode -s baseboard-product-name)
    case "$board" in
        *"GalileoGen2" )
            load_drivers "galileo_gen2" ;;
        *"Galileo" )
            load_drivers "galileo" ;;
    esac
}

die()
{
    exit 1
}

do_board
exit 0

