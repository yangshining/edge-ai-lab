#!/bin/sh

for file in /etc/profile.d/zsys*; do
	if [ -f "$file" ]; then
		echo "Sourcing $file"
		. "$file"
	fi
done


start () 
{
  echo "##---Security for SUB-PLAT:[$BAT_IMPRV] or [$VARI_PLAT] init---" > /dev/console

  if [ -n "$BAT_IMPRV" ]; then
      plat="$BAT_IMPRV"
  elif [ -n "$VARI_PLAT" ]; then
      plat="$VARI_PLAT"
  elif [ -n "$BASE_PLAT" ]; then
      plat="$BASE_PLAT"
  else
      echo "Error: Neither BAT_IMPRV nor VARI_PLAT is set."
      exit 1
  fi

  first_char=$(echo "${plat:0:1}" | tr '[:lower:]' '[:upper:]')
  rest_chars=$(echo "${plat:1}")
  new_hostname="${first_char}${rest_chars}"

  if [ -n "$PKG_MODE" ]; then
      new_hostname="${new_hostname}-${PKG_MODE}"
  fi
  echo "$new_hostname" > /etc/hostname
  hostname "$new_hostname"

  echo "Hostname changed to $new_hostname"
}

case "$1" in
  start)
    echo "##---Starting security setup init---" > /dev/console
	start;
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
