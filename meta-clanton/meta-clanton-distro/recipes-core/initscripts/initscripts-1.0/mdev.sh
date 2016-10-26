#!/bin/sh

# This script starts mdev which populates /dev
echo /sbin/mdev > /proc/sys/kernel/hotplug
mdev -s

