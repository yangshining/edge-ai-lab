#
# xi.he@zillnk.com
#
SUMMARY = "unix socket message bus"
DESCRIPTION = "OpenWrt system message/RPC bus"
HOMEPAGE = "https://git.openwrt.org/project/ubus.git"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"
SECTION = "PETALINUX/bsp"
DEPENDS = "json-c libubox"

SRC_URI = "\
			git://git.openwrt.org/project/ubus.git \
			file://0001-not-build-lua-examples-by-default.patch \
			file://ubusd.init \
		  "

SRCREV = "9913aa61de739e3efe067a2d186021c20bcd65e2"
PV = "master+git${SRCPV}"

S = "${WORKDIR}/git"

inherit cmake pkgconfig

PACKAGECONFIG_CONFARGS = "-DABIVERSION=2.0.0"

inherit useradd update-rc.d
INITSCRIPT_NAME = "ubusd"
INITSCRIPT_PARAMS = "start 35 S ."
USERADD_PACKAGES = "${PN}"
USERADD_PARAM_${PN} = "--system -d ${localstatedir}/lib/${BPN} -M -s /bin/false -U ubusd"

do_install_append () {
	install -d ${D}${sysconfdir}/init.d
	install -m 0755 ${WORKDIR}/ubusd.init ${D}${sysconfdir}/init.d/ubusd
}
FILES_${PN} += "${sysconfdir}/*"

