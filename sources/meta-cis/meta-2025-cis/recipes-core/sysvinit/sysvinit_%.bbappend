#add cis self-defined variables

do_install:append() {
    install -d ${D}${sysconfdir}/init.d

    sed -i '2i [ -f /etc/default/zsys-cis-env.env ] && . /etc/default/zsys-cis-env.env' ${D}${sysconfdir}/init.d/rc
    sed -i '3i [ -f /etc/default/zsys-app-env.env ] && . /etc/default/zsys-app-env.env' ${D}${sysconfdir}/init.d/rc

}

FILES:${PN} += " ${sysconfdir}/* "
