SUMMARY = "pre-act for zillnk sys env variables"
SECTION = "PETALINUX/apps"
LICENSE = "ZILLNK"
LIC_FILES_CHKSUM = " \
	"
PR = "r01"

SRC_URI = " \
            file://zsys-env.sh \
            file://zsys-app-env.env.in \
            file://zsys-cis-env.env.in \
          "

ENV_FILE_PLAT_SAKU = " file://zsys-cis-env_saku.env.in "
ENV_FILE_PLAT_SAKU_remove_hu = " file://zsys-cis-env_saku.env.in "
ENV_FILE_PLAT_SAKU_append_hu = " file://zsys-cis-env_hu.env.in "


ENV_FILE_PLAT_SAKU_remove_apple = " file://zsys-cis-env_saku.env.in "
ENV_FILE_PLAT_SAKU_append_apple = " file://zsys-cis-env_apple.env.in "
SRC_URI_append_saku = "${ENV_FILE_PLAT_SAKU}"

S = "${WORKDIR}"

inherit update-rc.d

INITSCRIPT_PACKAGES = "${PN}"
INITSCRIPT_NAME = "zsys-env.sh"
INITSCRIPT_PARAMS = "start 37 S ."

# please check the products' local-proj.conf or its common_baseline.conf to set the right settings.
# env varibles used when system running
# basic platform name
# BASE_PLAT="fara"
# eg. fara2-3-4-5 sakura all based on fara

# these should be put into sub-product's local-proj.conf
# sub fv-pv name
#VARI_PLAT=""

# other batch improvement name
#BAT_IMPRV=""

do_configure_append() {
    if [ -f ${WORKDIR}/zsys-cis-env_*.in ];then
      cp ${WORKDIR}/zsys-cis-env_*.in  ${WORKDIR}/zsys-cis-env.env.in
    fi

    cp -f ${WORKDIR}/zsys-cis-env.env.in ${WORKDIR}/zsys-cis-env.env
    cp -f ${WORKDIR}/zsys-app-env.env.in ${WORKDIR}/zsys-app-env.env
    echo "" >> ${WORKDIR}/zsys-cis-env.env
    echo "export LICENSE=${LICENSE}" >> ${WORKDIR}/zsys-cis-env.env
    echo "export BASE_PLAT=${BASE_PLAT}" >> ${WORKDIR}/zsys-cis-env.env
    echo "export VARI_PLAT=${VARI_PLAT}" >> ${WORKDIR}/zsys-cis-env.env
    echo "export BAT_IMPRV=${BAT_IMPRV}" >> ${WORKDIR}/zsys-cis-env.env
    echo "export PKG_MODE=${PKG_MODE}" >> ${WORKDIR}/zsys-cis-env.env
}

do_install() {
  install -d ${D}${INIT_D_DIR}
  install -m 0755 ${S}/${INITSCRIPT_NAME} ${D}${sysconfdir}/init.d/${INITSCRIPT_NAME}
  install -d "${D}${sysconfdir}/default"
  install -m 0755  ${WORKDIR}/zsys-cis-env.env ${D}${sysconfdir}/default/
  install -m 0755  ${WORKDIR}/zsys-app-env.env ${D}${sysconfdir}/default/
  install -d "${D}${sysconfdir}/profile.d"
  #show example
  ln -sf /etc/default/zsys-cis-env.env ${D}${sysconfdir}/profile.d/zsys-cis-env.sh
  ln -sf /etc/default/zsys-app-env.env ${D}${sysconfdir}/profile.d/zsys-app-env.sh
}

FILES_${PN} += "${sysconfdir}/*"
