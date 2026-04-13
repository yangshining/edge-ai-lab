do_install:append(){
    rm -Rf ${D}${sysconfdir}/cron.daily/*
    mkdir -p ${D}${sysconfdir}/cron.hourly
    install -p -m 0755 ${S}/examples/logrotate.cron ${D}${sysconfdir}/cron.hourly/logrotate  
}
