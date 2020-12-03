FROM fedora:33
MAINTAINER Peter Jones <pjones@redhat.com>

RUN dnf --nodocs -y --best --allowerasing install @buildsys-build dnf-plugins-core git make popt-devel nss-devel nspr-devel gettext elfutils-libelf-devel
RUN dnf --nodocs -y --best --allowerasing builddep efivar gnu-efi pesign 'shim-unsigned*'
RUN dnf -y clean all

RUN rm -rf /usr/share/doc/* /usr/share/man/*

RUN mkdir -p /root
WORKDIR /root

ENV PREFIX=/usr

ADD cov-build /bin/cov-build
ADD scan-build /bin/scan-build
ADD setup.sh /setup.sh
RUN /setup.sh
ADD build.sh /build.sh
RUN rpm -qla 'gnu-efi*'
ENTRYPOINT ["/build.sh"]
