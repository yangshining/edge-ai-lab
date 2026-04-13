SUMMARY = "U-Boot libraries and tools to access environment"

DESCRIPTION = "This package contains tools and libraries to read \
and modify U-Boot environment. \
It provides a hardware-independent replacement for fw_printenv/setenv utilities \
provided by U-Boot"

HOMEPAGE = "https://github.com/sbabic/libubootenv"
LICENSE = "LGPL-2.1-or-later"
LIC_FILES_CHKSUM = "file://Licenses/lgpl-2.1.txt;md5=4fbd65380cdd255951079008b364516c"
SECTION = "libs"

SRC_URI = "git://github.com/sbabic/libubootenv;protocol=https;branch=master; \
			file://0001-fix-ubootenv-lock.patch \
			file://fw_env.config \
			"
SRCREV = "824551ac77bab1d0f7ae34d7a7c77b155240e754"

S = "${WORKDIR}/git"

inherit uboot-config cmake lib_package

EXTRA_OECMAKE = "-DCMAKE_BUILD_TYPE=Release"

DEPENDS = "zlib"
PROVIDES = "ubootenv"
PACKAGE_ARCH = "${MACHINE_ARCH}"
TARGET_CC_ARCH += "${LDFLAGS}"

SRC_URI:remove:versal = "file://fw_env.config"
SRC_URI:append:versal = "file://fw_env.config.versal"

do_install() {
	install -d ${D}${libdir}
	install -d ${D}${includedir}
	install -d ${D}${sysconfdir}
	install -d ${D}${base_sbindir}
	install -m 0755 ${WORKDIR}/build/src/fw_printenv ${D}${base_sbindir}
	cd ${D}${base_sbindir}
	ln -s fw_printenv fw_setenv
        if [ -f ${WORKDIR}/fw_env.config* ];then
          install -m 0644 ${WORKDIR}/fw_env.config* ${D}${sysconfdir}/fw_env.config
        fi
	cp ${S}/src/libuboot.h ${S}
	install -m 0644 ${S}/*.h ${D}${includedir}
	cp ${WORKDIR}/build/src/libubootenv.so.0.3.1 ${S}/
	oe_soinstall ${S}/libubootenv.so.0.3.1 ${D}${libdir}
}
FILES_${PN} = "${libdir}/*so.* ${includedir}/*.h ${sysconfdir}/* ${base_sbindir}/*"

BBCLASSEXTEND = "native nativesdk"
