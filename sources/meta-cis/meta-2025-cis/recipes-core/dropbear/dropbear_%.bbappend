FILESEXTRAPATHS:prepend := "${THISDIR}/dropbear:"

SRC_URI:append = " \
			file://init_new \
		"

do_install:append() {
	sed -e 's,/usr,${prefix},g' ${WORKDIR}/init_new > ${D}${sysconfdir}/init.d/dropbear
}
