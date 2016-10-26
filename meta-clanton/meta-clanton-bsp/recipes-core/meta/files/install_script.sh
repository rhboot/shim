#!/bin/bash
toolchain_name=unknown
INST_ARCH=$(uname -m | sed -e "s/i[3-6]86/ix86/" -e "s/x86[-_]64/x86_64/")
SDK_ARCH=$(echo i586 | sed -e "s/i[3-6]86/ix86/" -e "s/x86[-_]64/x86_64/")

if [ "$INST_ARCH" != "$SDK_ARCH" ]; then
	# Allow for installation of ix86 SDK on x86_64 host
	if [ "$INST_ARCH" != x86_64 -o "$SDK_ARCH" != ix86 ]; then
		echo "Error: Installation machine not supported!"
		exit 1
	fi
fi

DEFAULT_INSTALL_DIR="/opt/clanton-tiny/1.4.2"
SUDO_EXEC=""
target_sdk_dir=""
answer=""
relocate=1
savescripts=0
verbose=0
while getopts ":yd:DRS" OPT; do
	case $OPT in
	y)
		answer="Y"
		[ "$target_sdk_dir" = "" ] && target_sdk_dir=$DEFAULT_INSTALL_DIR
		;;
	d)
		target_sdk_dir=$OPTARG
		;;
	D)
		verbose=1
		;;
	R)
		relocate=0
		savescripts=1
		;;
	S)
		savescripts=1
		;;
	*)
		echo "Usage: $(basename $0) [-y] [-d <dir>]"
		echo "  -y         Automatic yes to all prompts"
		echo "  -d <dir>   Install the SDK to <dir>"
		echo "======== Advanced DEBUGGING ONLY OPTIONS ========"
		echo "  -S         Save relocation scripts"
		echo "  -R         Do not relocate executables"
		echo "  -D         use set -x to see what is going on"
		exit 1
		;;
	esac
done

if [ $verbose = 1 ] ; then
	set -x
fi

target_sdk_dir=$(pwd)
# Try to create the directory (this will not succeed if user doesn't have rights)
mkdir -p $target_sdk_dir >/dev/null 2>&1

# if don't have the right to access dir, gain by sudo
if [ ! -x $target_sdk_dir -o ! -w $target_sdk_dir -o ! -r $target_sdk_dir ]; then
	SUDO_EXEC=$(which "sudo")
	if [ -z $SUDO_EXEC ]; then
		echo "No command 'sudo' found, please install sudo first. Abort!"
		exit 1
	fi

	# test sudo could gain root right
	$SUDO_EXEC pwd >/dev/null 2>&1
	[ $? -ne 0 ] && echo "Sorry, you are not allowed to execute as root." && exit 1

	# now that we have sudo rights, create the directory
	$SUDO_EXEC mkdir -p $target_sdk_dir >/dev/null 2>&1
fi

if [ -e "$target_sdk_dir/prev_install" ]; then
	DEFAULT_INSTALL_DIR=$($SUDO_EXEC cat "$target_sdk_dir/prev_install")
	echo This is $DEFAULT_INSTALL_DIR
fi

if [ ! -e "$target_sdk_dir/environment-setup-i586-poky-linux-uclibc" ]; then
printf "Extracting SDK..."
cat $toolchain_name | $SUDO_EXEC tar xj -C $target_sdk_dir
echo "done"
fi
printf "Setting it up..."
# fix environment paths
for env_setup_script in `ls $target_sdk_dir/environment-setup-*`; do
	$SUDO_EXEC sed -e "s:$DEFAULT_INSTALL_DIR:$target_sdk_dir:g" -i $env_setup_script
done

# fix dynamic loader paths in all ELF SDK binaries
native_sysroot=$($SUDO_EXEC cat $env_setup_script |grep OECORE_NATIVE_SYSROOT|cut -d'=' -f2|tr -d '"')
dl_path=$($SUDO_EXEC find $native_sysroot/lib -name "ld-linux*")
if [ "$dl_path" = "" ] ; then
	echo "SDK could not be set up. Relocate script unable to find ld-linux.so. Abort!"
	exit 1
fi
executable_files=$($SUDO_EXEC find $native_sysroot -type f -perm +111)

tdir=`mktemp -d`
if [ x$tdir = x ] ; then
   echo "SDK relocate failed, could not create a temporary directory"
   exit 1
fi
echo "#!/bin/bash" > $tdir/relocate_sdk.sh
echo exec ${env_setup_script%/*}/relocate_sdk.py $target_sdk_dir $dl_path $executable_files >> $tdir/relocate_sdk.sh
$SUDO_EXEC mv $tdir/relocate_sdk.sh ${env_setup_script%/*}/relocate_sdk.sh
$SUDO_EXEC chmod 755 ${env_setup_script%/*}/relocate_sdk.sh
rm -rf $tdir
if [ $relocate = 1 ] ; then
	$SUDO_EXEC ${env_setup_script%/*}/relocate_sdk.sh
	if [ $? -ne 0 ]; then
		echo "SDK could not be set up. Relocate script failed. Abort!"
		exit 1
	fi
fi

# replace /opt/clanton-tiny/1.4.2 with the new prefix in all text files: configs/scripts/etc
$SUDO_EXEC find $native_sysroot -type f -exec file '{}' \;|grep ":.*\(ASCII\|script\|source\).*text"|cut -d':' -f1|$SUDO_EXEC xargs sed -i -e "s:$DEFAULT_INSTALL_DIR:$target_sdk_dir:g"

# change all symlinks pointing to /opt/clanton-tiny/1.4.2
for l in $($SUDO_EXEC find $native_sysroot -type l); do
	$SUDO_EXEC ln -sfn $(readlink $l|$SUDO_EXEC sed -e "s:$DEFAULT_INSTALL_DIR:$target_sdk_dir:") $l
done
# find out all perl scripts in $native_sysroot and modify them replacing the
# host perl with SDK perl.
for perl_script in $($SUDO_EXEC grep "^#!.*perl" -rl $native_sysroot); do
	$SUDO_EXEC sed -i -e "s:^#! */usr/bin/perl.*:#! /usr/bin/env perl:g" -e \
		"s: /usr/bin/perl: /usr/bin/env perl:g" $perl_script
done

echo done

echo "SDK has been successfully set up and is ready to be used."

echo $target_sdk_dir > prev_install
exit 0
