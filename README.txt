1. Prepare a build machine (Part 2 of Build and Software User Guide)
    http://downloadmirror.intel.com/23962/eng/Quark_BSP_BuildandSWUserGuide_329687_007.pdf

    Note: Be sure to install IASL 5.0A or higher to build EDKII


2. Download Galileo Runtime 1.0.4 and unarchive (Part 3 of the User Guide)
    https://github.com/01org/Galileo-Runtime/archive/1.0.4.tar.gz
    tar -xf Galileo-Runtime-1.0.4.tar.gz
    cd Galileo-Runtime-1.0.4

3. Unarchive the patches
    tar -xf patches_v1.0.5.tar.gz


4. Build Linux kernel (Part 6 of the User Guide)

4.1. Unarchive meta-clanton package
    tar -xf meta-clanton_v*.tar.gz

4.2. Patch the meta-clanton package (Part 6.1 of the User Guide)
    ./patches_v*/patch.meta-clanton.sh

4.2. Build a small Linux for SPI Flash
    cd meta-clanton_v1.0.5
    source poky/oe-init-build-env yocto_build
    bitbake image-spi-galileo


5. Build EDKII (Part 4 of the User Guide)

5.1. Unarchive Quark_EDKII package (Part 4.2 of the User Guide)
    tar -xf Quark_EDKII_v1.0.2.tar.gz

5.2. Patch the Quark_EDKII package
    ./patches_v1.0.5/patch.Quark_EDKII.sh

5.3. Prepare the build environment (Part 4.2.1 of the User Guide)
    cd Quark_EDKII_v1.0.2
    ./svn_setup.py
    svn update

5.4. Build EDKII (Part 4.4 of the User Guide)
    export GCCVERSION=$(gcc -dumpversion | cut -c 3)
    ./quarkbuild.sh -r32 GCC4${GCCVERSION} QuarkPlatform

5.5. Create a symlink for SPI Flash Tools
    export GCCVERSION=$(gcc -dumpversion | cut -c 3)
    ln -s RELEASE_GCC4${GCCVERSION} Build/QuarkPlatform/RELEASE_GCC


6. Create a flash Image (SPI) (Part 8 of the User Guide)

6.1. Unarchive sysimage package
    tar -xf sysimage_v1.0.1.tar.gz

6.2. Patch the sysimage package
    ./patches_v1.0.5/patch.sysimage.sh

6.3. Build SPI
    tar -xf spi-flash-tools_v1.0.1.tar.gz
    ./sysimage_v1.0.1/create_symlinks.sh
    cd sysimage/sysimage.CP-8M-release
    ../../spi-flash-tools/Makefile

6.4. Congratulations!
    Flash-missingPDAT.cap is the firmware that you can flash onto Galileo.
