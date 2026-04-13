FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://crontab"

do_install:append(){
    install -m 0755 ${WORKDIR}/crontab ${D}${sysconfdir}/crontab
    chmod 600 ${D}${sysconfdir}/crontab
}

FILES:${PN} += "${sysconfdir}/cron*"
CONFFILES:${PN} += "${sysconfdir}/crontab"
