#!/bin/sh

for file in /etc/profile.d/zsys*; do
	if [ -f "$file" ]; then
		echo "Sourcing $file"
		. "$file"
	fi
done

install_versal_pld(){
     echo "install pld pdi"> /dev/console
     fpgautil -b /lib/firmware/titan_fh_fpga_xsa_pld.pdi -f Full -n Full
}

init_device_tree_overlay(){
     echo "insert overlay device tree" > /dev/console
     mkdir /sys/kernel/config/device-tree/overlays/xilinx
     cat /lib/firmware/overlay.dtbo > /sys/kernel/config/device-tree/overlays/xilinx/dtbo
}

case "$1" in
  start)
    echo "##---Starting install versal pld init---" > /dev/console
    install_versal_pld
    init_device_tree_overlay
    ;;
  stop)
    ;;
  restart)
    ;;
  *)
    echo "Usage: $0 { start | stop | restart }" >&2
    exit 1
    ;;
esac

exit 0
