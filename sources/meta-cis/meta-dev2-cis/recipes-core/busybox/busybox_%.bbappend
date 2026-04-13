FILESEXTRAPATHS:prepend := "${THISDIR}/files:${SYSCONFIG_PATH}:"

SRC_URI += "   file://syslog.conf"


#replace syslog.conf default with add rlog infra slot
#

do_install:append () {
       install -D -m 0644 ${WORKDIR}/syslog.conf ${D}${sysconfdir}/syslog.conf
       # echo "##" > ${D}${sysconfdir}/init.d/hwclock.sh
}
       
FILES:${PN} += " ${sysconfdir}/* "