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

case "$1" in
  start)
    echo "##---Starting aux dtbo init---" > /dev/console
    setup_auxfpga
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