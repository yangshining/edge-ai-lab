SUMMARY = "Restart Log core & command"
SECTION = "PETALINUX/apps"
LICENSE = "ZILLNK"

LIC_FILES_CHKSUM = " \
	"

PR = "r01"

SRC_URI = " file://rlog \
		  "

DEPENDS = ""

S = "${WORKDIR}/rlog"

inherit autotools

do_install:append() {
	install -d "${D}${sysconfdir}/logrotate.d"
	install -m 644 ${S}/rlog.logrotate ${D}${sysconfdir}/logrotate.d/logrotate.rlog
}
