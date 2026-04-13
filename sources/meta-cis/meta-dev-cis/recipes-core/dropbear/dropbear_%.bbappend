FILESEXTRAPATHS_prepend := "${THISDIR}/dropbear:"

SRC_URI_append = " \
			file://init_new \
		"

do_install_append() {
	sed -e 's,/usr,${prefix},g' ${WORKDIR}/init_new > ${D}${sysconfdir}/init.d/dropbear
}