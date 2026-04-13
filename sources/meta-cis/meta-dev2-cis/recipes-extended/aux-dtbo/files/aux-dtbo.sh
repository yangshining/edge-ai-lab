#!/bin/sh

for file in /etc/profile.d/zsys*; do
	if [ -f "$file" ]; then
		echo "Sourcing $file"
		. "$file"
	fi
done

setup_auxfpga(){
	echo "Setup auxfpga module"> /dev/console
}

init_device_tree_overlay(){
	mkdir /sys/kernel/config/device-tree/overlays/xilinx
}

insert_aux_dtbo(){
  echo "Set up interlink and insert dtbo" > /dev/console
  if ! /etc/init.d/interlink_setup.sh; then
    echo "Interlink set up failed!"
    exit 1
  fi
  cat /lib/firmware/overlay.dtbo > /sys/kernel/config/device-tree/overlays/xilinx/dtbo
}

case "$1" in
  start)
    echo "##---Starting aux dtbo init---" > /dev/console
    setup_auxfpga
    init_device_tree_overlay
    insert_aux_dtbo
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
