#
#
FILESEXTRAPATHS_prepend := "${THISDIR}/files:${SYSCONFIG_PATH}:"

SRC_URI_append = " file://dot.bashrc \
           file://licenses/GPL-2 \
           "

SRC_URI_append_saku = " \
			file://saku/dot.bashrc \
			"

SRC_URI_append_bailong = " \
			file://saku/dot.bashrc \
			"

S = "${WORKDIR}"

do_install_append_fara3 () {
	install -m 0755 ${WORKDIR}/dot.bashrc ${D}${sysconfdir}/skel/.bashrc
	install -m 0644 ${WORKDIR}/profile ${D}${sysconfdir}/profile
}

do_install_append_saku () {
	install -m 0755 ${WORKDIR}/saku/dot.bashrc ${D}${sysconfdir}/skel/.bashrc
}

do_install_append_bailong () {
	install -m 0755 ${WORKDIR}/saku/dot.bashrc ${D}${sysconfdir}/skel/.bashrc
}
