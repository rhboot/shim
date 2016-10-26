#!/bin/sh
### BEGIN INIT INFO
# Provides:          mountdevtmpfs
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Mount devtmpfs file systems.
# Description:       Mount the devtmpfs file system..
### END INIT INFO

if [ -e /dev ] && grep -q devtmpfs /proc/filesystems; then
	mount -t devtmpfs devtmpfs /dev
fi
