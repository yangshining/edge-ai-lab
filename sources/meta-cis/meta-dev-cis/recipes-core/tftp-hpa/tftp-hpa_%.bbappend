# init-ifupdown_%.bbappend content
  
do_install_append_zulu2() {
	sed -i 's/\/var\/lib\/tftpboot/\/mnt\/boot/' ${D}${sysconfdir}/default/tftpd-hpa
}
