#!/bin/sh

for file in /etc/profile.d/zsys*; do
	if [ -f "$file" ]; then
		echo "Sourcing $file"
		. "$file"
	fi
done

insmod_ptp_clockmatrix(){
  if [ "x$VARI_PLAT" = "xemimom" ]; then
	echo "Release of RSMU reset, set gpio0 37 1"
        gpioset gpiochip0 37=1
  else
	echo "Release of RSMU reset, set gpio1 24 1"
	gpioset gpiochip1 24=1
  fi

  sleep 1
  insmod /lib/modules/extra/ptp_clockmatrix.ko
}

case "$1" in
  start)
    echo "##---Insert module ptp-clockmatrix---" > /dev/console
    insmod_ptp_clockmatrix
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
