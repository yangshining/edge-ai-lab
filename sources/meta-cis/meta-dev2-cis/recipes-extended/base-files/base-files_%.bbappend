#
#
FILESEXTRAPATHS:prepend := "${THISDIR}/files:${SYSCONFIG_PATH}:"

SRC_URI:append = " file://dot.bashrc \
           file://licenses/GPL-2 \
           "

SRC_URI:append:saku = " \
			file://saku/dot.bashrc \
			"

S = "${WORKDIR}"

do_install:append:fara3 () {
	install -m 0755 ${WORKDIR}/dot.bashrc ${D}${sysconfdir}/skel/.bashrc
	install -m 0644 ${WORKDIR}/profile ${D}${sysconfdir}/profile
}

do_install:append:saku () {
	install -m 0755 ${WORKDIR}/saku/dot.bashrc ${D}${sysconfdir}/skel/.bashrc
}
