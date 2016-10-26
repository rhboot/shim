#!/bin/sh

# This script creates the mqueue mount point and mounts is

mkdir -m 1777 /dev/mqueue
mount none /dev/mqueue -t mqueue
