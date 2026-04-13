FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://crontab"

do_install_append(){
    install -m 0755 ${WORKDIR}/crontab ${D}${sysconfdir}/crontab
    chmod 600 ${D}${sysconfdir}/crontab
}

FILES_${PN} += "${sysconfdir}/cron*"
CONFFILES_${PN} += "${sysconfdir}/crontab"