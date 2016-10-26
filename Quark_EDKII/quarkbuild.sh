#!/bin/bash
# Copyright (c) 2013 Intel Corporation.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in
# the documentation and/or other materials provided with the
# distribution.
# * Neither the name of Intel Corporation nor the names of its
# contributors may be used to endorse or promote products derived
# from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# This build script is:
# 1. An implementation of the Tianocore instructions for GCC, plus
# 2. Clanton-specific build steps

# This script automates compiling the default configs. For anything
# more custom edit the Conf/target.txt file and use the original
# Tianocore 'build' command
# http://sourceforge.net/apps/mediawiki/tianocore/index.php?title=Using_EDK_II_with_Native_GCC_4.4

set -e

build_all()
{
    
    test -d ./BaseTools ||
    die "Missing required directories, have you run 'svn update'?\n"

    test -e ./QuarkSocPkg/QuarkNorthCluster/Binary/QuarkMicrocode/RMU.bin || {
	echo "Trying to fetch Chipset Microcode"
	./fetchCMCBinary.py || die "Missing Chipset Microcode\n"
    }

    mkdir -p Conf


    ##############################################################################
    ########################       PreBuild-processing       #####################
    ##############################################################################

    # Provide default config files when the user has not
    for cfgf in  tools_def  build_rule; do

        [ -e ./Conf/"${cfgf}".txt ] ||
            cp -f ./QuarkPlatformPkg/Override/BaseTools/Conf/"${cfgf}".template ./Conf/"${cfgf}".txt

    done

    make -C BaseTools
     # This defines EDK_TOOLS_PATH=..../BaseTools and others
    # Note: this script will also provide all the other default Conf/
    # files (from BaseTools/), i.e., all config files besides the ones
    # we provided just above
    . ./edksetup.sh


    local thread_number_opt=''
    if test -n "${THREAD_NUMBER}"; then
	thread_number_opt="-n ${THREAD_NUMBER}"
    fi

    ####################################################################################################################
    ######                                Perform the actual build                                         #############
    ######   Warning: parameters here are supposed to override any corresponding value in Conf/target.txt  #############
    ####################################################################################################################
    build -p ${platform}Pkg/${platform}Pkg.dsc -b ${target} -a IA32 ${thread_number_opt} -t $tool_opt -y Report.log $_args ${debug_print_error_level} ${debug_property_mask}


    ###############################################################################
    ########################       PostBuild-processing       #####################
    ###############################################################################
    ./QuarkPlatformPkg/Tools/QuarkSpiFixup/QuarkSpiFixup.py $platform $target $tool_opt


    ####################################################################################################################
    ######         Perform EDKII build again after QuarkSpiFixup so that output .fd file usable.           #############
    ######   Warning: parameters here are supposed to override any corresponding value in Conf/target.txt  #############
    ####################################################################################################################
    build -p ${platform}Pkg/${platform}Pkg.dsc -b ${target} -a IA32 ${thread_number_opt} -t $tool_opt -y Report.log $_args ${debug_print_error_level} ${debug_property_mask}

    OutputModulesDir=$WORKSPACE/Build/${platform}/${target}_$tool_opt/FV

    # Copy build output .fd file int0 FlashModules directory.
    # Use similar name to full Quark Spi Flash tools but emphasise only EDKII assets in bin file.
    cp -f $OutputModulesDir/QUARK.fd $OutputModulesDir/FlashModules/Flash-EDKII-missingPDAT.bin

    ###############################################################################
    ########################     Image signing stage           ####################
    ########################       (dummy signing)             ####################
    ###############################################################################
    if [ ! -e $WORKSPACE/QuarkPlatformPkg/Tools/SignTool/DummySignTool ]
    then
      make -C $WORKSPACE/QuarkPlatformPkg/Tools/SignTool
    fi
          
    $WORKSPACE/QuarkPlatformPkg/Tools/SignTool/DummySignTool ${OutputModulesDir}/FlashModules/EDKII_BOOT_STAGE1_IMAGE1.Fv $OutputModulesDir/EDKII_BOOT_STAGE1_IMAGE1.Fv.signed
    $WORKSPACE/QuarkPlatformPkg/Tools/SignTool/DummySignTool ${OutputModulesDir}/FlashModules/EDKII_BOOT_STAGE1_IMAGE2.Fv $OutputModulesDir/EDKII_BOOT_STAGE1_IMAGE2.Fv.signed
    $WORKSPACE/QuarkPlatformPkg/Tools/SignTool/DummySignTool ${OutputModulesDir}/FlashModules/EDKII_BOOT_STAGE2_RECOVERY.Fv $OutputModulesDir/EDKII_BOOT_STAGE2_RECOVERY.Fv.signed
    $WORKSPACE/QuarkPlatformPkg/Tools/SignTool/DummySignTool ${OutputModulesDir}/FlashModules/EDKII_BOOT_STAGE2_COMPACT.Fv $OutputModulesDir/EDKII_BOOT_STAGE2_COMPACT.Fv.signed
    $WORKSPACE/QuarkPlatformPkg/Tools/SignTool/DummySignTool ${OutputModulesDir}/FlashModules/EDKII_BOOT_STAGE2.Fv $OutputModulesDir/EDKII_BOOT_STAGE2.Fv.signed
    $WORKSPACE/QuarkPlatformPkg/Tools/SignTool/DummySignTool ${OutputModulesDir}/FlashModules/EDKII_RECOVERY_IMAGE1.Fv $OutputModulesDir/EDKII_RECOVERY_IMAGE1.Fv.signed

    ###############################################################################
    ####################         Capsule creation stage            ################
    ####################     (Recovery and Update capsules)        ################
    ###############################################################################
    CapsuleConfigFile=$WORKSPACE/${platform}Pkg/Tools/CapsuleCreate/${platform}PkgCapsuleComponents.inf
    CapsuleOutputFileReset=$OutputModulesDir/${platform}PkgReset.Cap
    # CAPSULE_FLAGS_PERSIST_ACROSS_RESET          0x00010000
    # CAPSULE_FLAGS_INITIATE_RESET                0x00040000
    CapsuleFlagsNoReset=0x00000000
    CapsuleFlagsReset=0x00050000

	  if [ ! -e $WORKSPACE/QuarkPlatformPkg/Tools/CapsuleCreate/CapsuleCreate ]
    then
		  make -C $WORKSPACE/QuarkPlatformPkg/Tools/CapsuleCreate
    fi

	  if [ -e $WORKSPACE/${platform}Pkg/Tools/CapsuleCreate/${platform}PkgCapsuleComponents.inf ]
    then
		  $WORKSPACE/QuarkPlatformPkg/Tools/CapsuleCreate/CapsuleCreate $CapsuleConfigFile $OutputModulesDir $CapsuleOutputFileReset $CapsuleFlagsReset
    fi

    $WORKSPACE/QuarkPlatformPkg/Tools/SignTool/DummySignTool $CapsuleOutputFileReset ${CapsuleOutputFileReset}.signed
		cat $OutputModulesDir/EDKII_BOOT_STAGE2_RECOVERY.Fv.signed $CapsuleOutputFileReset.signed > $OutputModulesDir/FVMAIN.fv

    ###############################################################################
    ################       Create useful output directories        ################
    ###############################################################################
    # Create Remediation directory
	  if [ ! -e $OutputModulesDir/RemediationModules ]
    then
		  mkdir $OutputModulesDir/RemediationModules
    fi

		# Copy the 'Recovery Capsule' and 'Update Capsules' to the RemediationModules directory
		# Use same/similar names in stand-alone builds compared to full Quark Spi Flash tools.
		cp -f $OutputModulesDir/FVMAIN.fv $OutputModulesDir/RemediationModules/.
		cp -f ${CapsuleOutputFileReset}.signed $OutputModulesDir/RemediationModules/Flash-EDKII.cap
		cp -f $WORKSPACE/QuarkPlatformPkg/Applications/CapsuleApp.efi $OutputModulesDir/RemediationModules/.

    # Create Tools directory
	  if [ ! -e $OutputModulesDir/Tools ]
    then
		  mkdir $OutputModulesDir/Tools
    fi
    		  
		# Copy the 'CapsuleCreate' tool to the Tools directory
		cp -f $WORKSPACE/QuarkPlatformPkg/Tools/CapsuleCreate/CapsuleCreate $OutputModulesDir/Tools/.
		    	
    # Create Applications directory
	  if [ ! -e $OutputModulesDir/Applications ]
    then
		  mkdir $OutputModulesDir/Applications
    fi
    		   
		# Copy the 'CapsuleApp.efi' application to the Application directory
		cp -f $WORKSPACE/QuarkPlatformPkg/Applications/CapsuleApp.efi $OutputModulesDir/Applications/.
    		    		  
    ########################################################################################
    ########    Remove build files created (now that we are finished using them)    ########
    ########################################################################################		    		  
    rm -f $WORKSPACE/QuarkPlatformPkg/Tools/SignTool/DummySignTool
    rm -f $WORKSPACE/QuarkPlatformPkg/Tools/SignTool/DummySignTool.o
    rm -f $WORKSPACE/QuarkPlatformPkg/Tools/SignTool/DummySignTool.d
		rm -f $WORKSPACE/QuarkPlatformPkg/Tools/CapsuleCreate/CapsuleCreate
		rm -f $WORKSPACE/QuarkPlatformPkg/Tools/CapsuleCreate/CapsuleCreate.o
		rm -f $WORKSPACE/QuarkPlatformPkg/Tools/CapsuleCreate/CapsuleCreate.d
		      		  
    cp -f ./Report.log ./Build/${platform}/${target}_$tool_opt
}

die()
{
    printf "ERROR: "
    printf "$@"; exit 1
}

usage()
{
    die  "$0  [-r32|-d32|-clean] [GCC43|GCC44|GCC45|GCC46|GCC47] [PlatformName] [-DSECURE_LD(optional)] [-DSECURE_BOOT(optional)]\n"
}

clean()
{
    printf "Warning: this option is different from every EDK2's 'build cleanXXX' feature\n"

    ( set -x
	rm -rf Conf Build
    )
}

main()
{
        case "$1" in
        -clean)
            clean; exit 0
            ;;
        -r32)
            target=RELEASE
            debug_print_error_level=-DDEBUG_PRINT_ERROR_LEVEL=0x80000000
            debug_property_mask=-DDEBUG_PROPERTY_MASK=0x23
            ;;
        -d32)
            target=DEBUG
            debug_print_error_level=-DDEBUG_PRINT_ERROR_LEVEL=0x80000042
            debug_property_mask=-DDEBUG_PROPERTY_MASK=0x27
            ;;
        *)
            usage
        esac
        case "$2" in
        GCC43) ;;
        GCC44) ;;
        GCC45) ;;
        GCC46) ;;
        GCC47) ;;
        *)
            usage
        esac

        # 2nd argument is the build tools
        tool_opt=$2
        
        # 3rd argument is the platform name
        platform=$3
                
        # Add all arguments after the 3rd to the _args array
        array=( $@ )
        _args=${array[@]:3:$#}

        build_all
}


main "$@"
