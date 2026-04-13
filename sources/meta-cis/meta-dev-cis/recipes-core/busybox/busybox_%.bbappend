FILESEXTRAPATHS_prepend := "${THISDIR}/files:${SYSCONFIG_PATH}:"

SRC_URI += "   file://syslog.conf"


#replace syslog.conf default with add rlog infra slot
#

do_install_append () {
       install -D -m 0644 ${WORKDIR}/syslog.conf ${D}${sysconfdir}/syslog.conf
}
       
FILES_${PN} += " ${sysconfdir}/* "


