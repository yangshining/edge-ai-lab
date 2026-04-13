#!/bin/sh

for file in /etc/profile.d/zsys*; do
	if [ -f "$file" ]; then
		echo "Sourcing $file"
		. "$file"
	fi
done


set_wd_date () {
    watchdog -t 18 /dev/watchdog0
    
    date -s "1970-01-01 00:01:00"
    hwclock -w
}

case "$1" in
  start)
    echo "##---Starting sdev setup init---" > /dev/console
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