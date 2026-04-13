
#!/bin/sh
for file in /etc/profile.d/zsys*; do
	if [ -f "$file" ]; then
		echo "Sourcing $file"
		. "$file"
	fi
done

fstype=ext4
logsys_mount_point="/ram1"
racd_mount_point="/ram2"
if [ -z $LOG_DIR_NAME ]; then
    logsys_softlink_in_varlog="zlog"
else
    logsys_softlink_in_varlog=$LOG_DIR_NAME
fi
racd_softlink_in_varlog="racd"
run() {
    $@ > /dev/null 2> /dev/null
    rc=$?
    if [ $rc -ne 0 ]; then
        string="mtvram.sh failed ($rc): $@"
        echo $string >>/dev/ttyPS0
    fi
}
printer() {
    string="mtvram.sh: $@"
    echo $string >>/dev/ttyPS0
}
format_res_mem_partition() {
    printer "Creating $rd in format of $fstype: "
    run mkfs.$fstype -L zrram $rd > /dev/null 2> /dev/null
}
# if return 0, no action just mount
# if larger than 3, that should be formated again; eg. cold reboot
# if equal to 1, that means do some corrections by fsck
fs_checker() {
    fsck.$fstype -y $rd > /dev/null 2> /dev/null
    rc=$?
    if [ $(($rc & 1)) -eq 1 ]; then
        printer "Correction of file system errors"
    fi
    if [ $(($rc & 2)) -eq 2 ]; then
        printer "Restart to recover file system errors"
        reboot -f
    fi
    if [ $rc -gt 3 ]; then
        printer "Can't correct fs errors ($rc), ongoing to format the partiton: $rd!"
        format_res_mem_partition
    fi
}
check_rst_reason() {
  if [ "$(fw_printenv reset_reason | awk -F'=' '{print $2}')" = "EXTERNAL" ]; then
    echo "Cold reset!" > /dev/null
    return 0
  else
    echo "Warm reset!" > /dev/null
    return 1
  fi
}
fscheck_mount_res_divmm() {
    rd=$1
    mp=$2
    mkdir -p $mp
    umount $mp 2>/dev/null
    printer "File-system Verifying for $rd"
    if check_rst_reason; then
      printer "Creating $rd in format of $fstype: "
      mkfs.$fstype -F -L zrram $rd > /dev/null 2> /dev/null
    else
      fs_checker
    fi
    run mount -t $fstype -o sync $rd $mp
    printer "Mounted $rd ($fstype) on $mp"
}
attach_mem_in_varlog() {
    mp=$1 #mount point
    slp=$2 #softlink point
	  printer "### $mp setup -> $slp"
    # Create sub-dir for remaining ram
    mkdir -p $mp/$slp
    chmod 0755 $mp/$slp
    # Attach soft link to the mount point
    ln -sf $mp/$slp /var/log/$slp
    chmod 0755 /var/log/$slp
    # bug fix RHS1166086-657
    rm -f /var/log/$slp/$slp    
    ln -sf $mp/$slp /var/volatile/log/$slp
    chmod 0755 /var/volatile/log/$slp
    rm -f /var/volatile/log/$slp/$slp    
}
case "$1" in
  start)
    #pmem0 for log system 
    #pmem1 for RACD or Retrospective Analysis Core Dump 
    fscheck_mount_res_divmm /dev/pmem0 ${logsys_mount_point}
	  fscheck_mount_res_divmm /dev/pmem1 ${racd_mount_point}
    attach_mem_in_varlog ${logsys_mount_point} ${logsys_softlink_in_varlog}
	  attach_mem_in_varlog ${racd_mount_point} ${racd_softlink_in_varlog}
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

