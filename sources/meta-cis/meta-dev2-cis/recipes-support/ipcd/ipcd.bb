#
# xi.he@zillnk.com
#
SUMMARY = "ipc bus server"
DESCRIPTION = "ipc bus server for bsp"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"
SECTION = "PETALINUX/bsp"
DEPENDS = "json-c libubox ubus ubootenv zlib sqlite3"

SRC_URI = "git://172.16.1.21:29418/3pp/bsp/ipcd;protocol=ssh;user=${USER};branch=master;"

SRCREV = "746b817d66c57376b8272d7ecc3344108dd6f511"

SRC_URI:append = "\
			file://ipcd.init \
			file://board_env_mtd5.config \
			file://board_env_mtd6.config \
			file://board_env_mtd14.config \
			file://board_env_mtd13.config \
		  "

SRC_URI:append:auxs = "\
					file://0001-add-tool-package-update.patch \
					"

S = "${WORKDIR}/git"

inherit cmake pkgconfig

PACKAGECONFIG_CONFARGS = "-DABIVERSION=2.0.0"
PACKAGECONFIG_CONFARGS:append:fara:updtool = " -DAUAPP_UPD=ON "
PACKAGECONFIG_CONFARGS:append:soga = " -DFARA_V2=ON "
PACKAGECONFIG_CONFARGS:append:fara2 = " -DFARA_V2=ON "
PACKAGECONFIG_CONFARGS:append:fmav = " -DFARA_V2=ON "
PACKAGECONFIG_CONFARGS:append:zulu2 = " -DZULU=ON "

inherit useradd update-rc.d
INITSCRIPT_NAME = "ipcd"
INITSCRIPT_PARAMS = "start 36 S ."
USERADD_PACKAGES = "${PN}"
USERADD_PARAM:${PN} = "--system -d ${localstatedir}/lib/${BPN} -M -s /bin/false -U ipcd"

do_install:append () {
	install -d ${D}${sysconfdir}/init.d
	install -m 0755 ${WORKDIR}/ipcd.init ${D}${sysconfdir}/init.d/ipcd
	install -d ${D}${sysconfdir}
	install -m 0644 ${WORKDIR}/board_env_mtd5.config ${D}${sysconfdir}/board_env.config
	install -m 0644 ${WORKDIR}/board_env_mtd13.config ${D}${sysconfdir}/storage_env.config
}

do_install:append:fara () {
	rm -f ${D}${sysconfdir}/board_env.config
	install -m 0644 ${WORKDIR}/board_env_mtd6.config ${D}${sysconfdir}/board_env.config
}

do_install:append:soga () {
        rm -f ${D}${sysconfdir}/board_env.config
        install -m 0644 ${WORKDIR}/board_env_mtd14.config ${D}${sysconfdir}/board_env.config
}

do_install:append:fara2 () {
        rm -f ${D}${sysconfdir}/board_env.config
        install -m 0644 ${WORKDIR}/board_env_mtd14.config ${D}${sysconfdir}/board_env.config
}

do_install:append:alpha () {
	rm -f ${D}${sysconfdir}/board_env.config
	install -m 0644 ${WORKDIR}/board_env_mtd6.config ${D}${sysconfdir}/board_env.config
}

do_install:append:zulu2 () {
	rm -f ${D}${sysconfdir}/board_env.config
	install -m 0644 ${WORKDIR}/board_env_mtd6.config ${D}${sysconfdir}/board_env.config
}

FILES:${PN} += "${sysconfdir}/*"
