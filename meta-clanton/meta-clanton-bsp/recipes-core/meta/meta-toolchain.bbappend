FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

inherit populate_sdk_base

SCRIPT = "install_script.sh"
SRC_URI = "file://${SCRIPT}"

do_deploy() {
  if [ "${SDKMACHINE}" = "i586-tarball" -o "${SDKMACHINE}" = "x86_64-tarball" ]
  then
    install -d ${SDK_DEPLOY}
    sed -i 2s/unknown/${SDK_NAME}-toolchain-${SDK_VERSION}.tar.bz2/ ${WORKDIR}/${SCRIPT} 
    install ${WORKDIR}/${SCRIPT} ${SDK_DEPLOY}/${SCRIPT}
  fi
}

addtask deploy before do_build after do_compile

