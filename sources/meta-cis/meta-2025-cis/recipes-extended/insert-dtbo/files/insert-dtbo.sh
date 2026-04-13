#!/bin/sh

for file in /etc/profile.d/zsys*; do
	if [ -f "$file" ]; then
		echo "Sourcing $file"
		. "$file"
	fi
done

insmod_select_map(){
	echo "insmod select map module"> /dev/console
	insmod /lib/modules/extra/select-map.ko
}

init_device_tree_overlay(){
	mkdir /sys/kernel/config/device-tree/overlays/xilinx
}

case "$1" in
  start)
    echo "##---Starting insert dtbo init---" > /dev/console
    insmod_select_map
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