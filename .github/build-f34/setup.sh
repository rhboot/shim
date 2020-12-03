#!/bin/bash

set -eu
set -x

cd /root

git clone https://github.com/rhboot/efivar
git clone https://github.com/rhboot/efibootmgr
git clone https://github.com/vathpela/gnu-efi
git clone https://github.com/rhboot/pesign
git clone https://github.com/rhboot/shim
cd shim
git config --local --add shim.efidir test
mkdir build-aa64 build-arm build-ia32 build-x64 
make clean
cd ..


