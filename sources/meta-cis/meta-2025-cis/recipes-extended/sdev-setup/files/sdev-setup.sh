#!/bin/sh

for file in /etc/profile.d/zsys*; do
	if [ -f "$file" ]; then
		echo "Sourcing $file"
		. "$file"
	fi
done


set_auapp_with_running () {
    if [ -n "$PKG_MODE" ] && [ "$PKG_MODE" = "APP" ]; then
        val=$(fw_printenv runing | awk -F'=' '{print $2}')

        if [ "$val" = "0" ]; then
            export PKG_MODE="AUAPP"
            echo "PKG_MODE set to AUAPP"
            sed -i 's/^export PKG_MODE=.*/export PKG_MODE=AUAPP/' /etc/default/zsys-cis-env.env
            echo "Updated /etc/default/zsys-cis-env.env"
        fi
    fi
}

set_wd_date () {
    watchdog -t 18 /dev/watchdog0
    
    date -s "1970-01-01 00:01:00"
    hwclock -w
}

case "$1" in
  start)
    echo "##---Starting sdev setup init---" > /dev/console
    set_auapp_with_running
    set_wd_date
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
