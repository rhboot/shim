1. Prepare a build machine (Part 2 of Build and Software User Guide)
    http://downloadmirror.intel.com/23962/eng/Quark_BSP_BuildandSWUserGuide_329687_007.pdf

    Note: Be sure to install IASL 5.0A or higher to build EDKII


2. Download the latest Galileo Runtime (Part 3 of the User Guide)
    git clone https://github.com/01org/Galileo-Runtime.git
    cd Galileo-Runtime


3. Build Linux kernel (Part 6 of the User Guide)
    cd meta-clanton
    ./setup.sh
    source poky/oe-init-build-env yocto_build
    bitbake image-spi-galileo


4. Build EDKII (Part 4 of the User Guide)

4.1. Prepare the build environment (Part 4.2.1 of the User Guide)
    cd Quark_EDKII
    ./svn_setup.py
    svn update

4.2. Build EDKII (Part 4.4 of the User Guide)
    export GCCVERSION=$(gcc -dumpversion | cut -c 3)
    ./quarkbuild.sh -r32 GCC4${GCCVERSION} QuarkPlatform

4.3. Create a symlink for SPI Flash Tools
    export GCCVERSION=$(gcc -dumpversion | cut -c 3)
    ln -s RELEASE_GCC4${GCCVERSION} Build/QuarkPlatform/RELEASE_GCC


5. Create a flash Image (SPI) (Part 8 of the User Guide)
    cd sysimage/sysimage.CP-8M-release
    ../../spi-flash-tools/Makefile


6. Congratulations!
    Flash-missingPDAT.cap is the firmware that you can flash onto Galileo.
