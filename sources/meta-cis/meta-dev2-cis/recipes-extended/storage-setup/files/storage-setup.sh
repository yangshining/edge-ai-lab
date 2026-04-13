#!/bin/sh

for file in /etc/profile.d/zsys*; do
	if [ -f "$file" ]; then
		echo "Sourcing $file"
		. "$file"
	fi
done
#change here to change name of dir where app & mplane log put
#must be same as related name "logsys_softlink_in_varlog" in recipes-core/mtvram/files/mtvram.sh
if [ -z $LOG_DIR_NAME ]; then
    log_dir_name="zlog"
else
    log_dir_name=$LOG_DIR_NAME
fi

app_log_dir="/var/log/$log_dir_name/app"
last_sysmgmtlog="/var/log/$log_dir_name/last_save_sysmgmt.tgz"
mplane_log_dir="/var/log/$log_dir_name/mplane"
operation_log="/var/log/$log_dir_name/operation.log*"
log_dir="/var/log/$log_dir_name"
unloading_emmc_dir="/var/log/$log_dir_name/emmc"
fstype=ext4
run() {
    $@ > /dev/null 2> /dev/null
    rc=$?
    if [ $rc -ne 0 ]; then
        string="storage-setup failed ($rc): $@"
        echo $string
    fi
}
printer() {
    string="storage-setup: $@"
    echo $string
}


function check_app_log_directory(){
    if [ ! -d "$app_log_dir" ]; then
        mkdir -p $app_log_dir
    fi
}

function clear_excess_packages(){
    absolute_scan_path="/media/sd-mmcblk0p3"
    if [ -d "$absolute_scan_path" ]; then
        find ${absolute_scan_path} -maxdepth 1 -type f -name '*.bin' ! -name 'lm.bin' -exec rm -f {} +
        find ${absolute_scan_path} -maxdepth 1 -type f -name '*.tar' -exec rm -f {} +
    fi
}

function save_mplane_log(){
  if [ ! -d "$mplane_log_dir" ]; then
      mkdir -p $mplane_log_dir
  else
      if [ -f "$log_dir/messages" ]
      then
          tar -zcvf $last_sysmgmtlog $mplane_log_dir $operation_log $log_dir/messages*
      else
          tar -zcvf $last_sysmgmtlog $mplane_log_dir $operation_log
      fi
      rm -Rf $mplane_log_dir/sysmgmt*
	  rm -Rf $mplane_log_dir/sssysmgmt*
      rm -f $operation_log
      rm -f $log_dir/messages*
  fi
}

function zulu_save_mplane_log(){
  if [ ! -d "$mplane_log_dir" ]; then
      mkdir -p $mplane_log_dir
  else
      tar -zcvf $last_sysmgmtlog $mplane_log_dir/sysmgmt*
      rm -Rf $mplane_log_dir/sysmgmt*
  fi
}

create_o_ran_link_folder(){
    mkdir -p /media/sd-mmcblk0p3/sysrepo
    mkdir -p /media/sd-mmcblk0p3/log/coredump
    mkdir -p /media/sd-mmcblk0p3/users
    mkdir -p /media/sd-mmcblk0p3/images

	if [ ! -d "/O-RAN/config" ]; then
		mkdir -p /O-RAN/config
	fi
    ln -s /media/sd-mmcblk0p3/sysrepo /O-RAN/config/sysrepo
    ln -s /media/sd-mmcblk0p3/log /O-RAN/config/log
    ln -s /media/sd-mmcblk0p3/users /O-RAN/config/users
    ln -s /media/sd-mmcblk0p3/images /O-RAN/config/images

    # for change vlan scan file 
    # /O-RAN/config/sysrepo/o-ran-mplane-int_startup_fara.xml -> /O-RAN/config/sysrepo/o-ran-mplane-int_startup.xml
    vlan_scan_old_file="/O-RAN/config/sysrepo/o-ran-mplane-int_startup_fara.xml"
    vlan_scan_new_file="/O-RAN/config/sysrepo/o-ran-mplane-int_startup.xml"

    if [ -f "$vlan_scan_old_file" ]
    then
        echo "${vlan_scan_old_file} exists"
        mv $vlan_scan_old_file $vlan_scan_new_file
    fi
}

function check_emmc_log_directory(){
    if [ ! -d "$unloading_emmc_dir" ]; then
        mkdir -p $unloading_emmc_dir
    fi
}

n8n20n28b_create_images_link()
{
    rm -rf /O-RAN/images
    rm -rf /media/sd-mmcblk0p3/images/*.bin
    ln -s /media/sd-mmcblk0p3/images /O-RAN/images

    # for persistence log
    rm -rf /O-RAN/log
    ln -s /var/log/$log_dir_name/mplane /O-RAN/log

    mkdir -p /sysrepo
    mkdir -p /var/log/$log_dir_name/sysrepo/data
    mkdir -p /var/log/$log_dir_name/sysrepo/yang
    # for persistence sysrepo
    ln -s /var/log/$log_dir_name/sysrepo/data /sysrepo/data
    ln -s /var/log/$log_dir_name/sysrepo/yang /sysrepo/yang
}

zulu_mnt_boot_spec()
{
    #get env
    zboot=$(fw_printenv zboot)
    slot=`echo  $zboot | tr -cd "[0-9]"`

    #mount emmc
    fsck.ext4 -y /dev/mmcblk0p$slot > /dev/null 2> /dev/null
    rc=$?
    if [ $(($rc & 1)) -eq 1 ]; then
        echo "Correction of file system errors"
    fi
    if [ $(($rc & 2)) -eq 2 ]; then
        echo "Restart to recover file system errors"
        reboot -f
    fi
    if [ $rc -gt 3 ]; then
        echo "Can't correct fs errors ($rc), ongoing to format the partiton: /dev/mmcblk0p$slot!"
        echo "Creating /dev/mmcblk0p$slot in format of ext4: "
        mkfs.ext4 /dev/mmcblk0p$slot > /dev/null 2> /dev/null
    fi  

	ln -s /media/sd-mmcblk0p$slot /mnt/boot
}

zulu_create_o_ran_link_folder(){
    mkdir -p /media/sd-mmcblk0p1
    mkdir -p /media/sd-mmcblk0p2
    mkdir -p /media/sd-mmcblk0p3
    mount /dev/mmcblk0p1 /media/sd-mmcblk0p1/
    mount /dev/mmcblk0p2 /media/sd-mmcblk0p2/
    mount /dev/mmcblk0p3 /media/sd-mmcblk0p3/

    mkdir -p /media/sd-mmcblk0p3/sysrepo
    mkdir -p /media/sd-mmcblk0p3/log
    mkdir -p /media/sd-mmcblk0p3/users
    mkdir -p /media/sd-mmcblk0p3/log/coredump

    ln -s /media/sd-mmcblk0p3/sysrepo /O-RAN/config/sysrepo
    ln -s /media/sd-mmcblk0p3/log /O-RAN/config/log
    ln -s /media/sd-mmcblk0p3/users /O-RAN/config/users
}

format_res_mem_partition() {
    printer "Creating $1 in format of $fstype: "
    run mkfs.$fstype $1 > /dev/null 2> /dev/null
}
# if return 0, no action just mount
# if larger than 3, that should be formated again; eg. cold reboot
# if equal to 1, that means do some corrections by fsck
fs_checker() {
    device="/dev/mmcblk0p$1"
    if [ -e "$device" ]; then
        fsck.$fstype -y $device > /dev/null 2> /dev/null
        rc=$?
        if [ $(($rc & 1)) -eq 1 ]; then
            printer "Correction of file system errors"
        fi
        if [ $(($rc & 2)) -eq 2 ]; then
            printer "Restart to recover file system errors"
            reboot -f
        fi
        if [ $rc -gt 3 ]; then
            printer "Can't correct fs errors ($rc), ongoing to format the partiton: $device!"
            format_res_mem_partition $device
        fi
    fi
}
create_mmc() {
	device="/dev/mmcblk0p$1"
    partition="/media/sd-mmcblk0p$1"
	if [ -e "$device" ]; then
        mkdir -p "$partition"
	fi
}
mount_mmc() {
    device="/dev/mmcblk0p$1"
    if [ -e "$device" ]; then 
        mount "$device" "/media/sd-mmcblk0p$1"
    fi
}

fara_mount_all() {
	#mmc init, partition1,2,3, note: 4 as extend partitions
	#if only 1,2,3, create_mmc and mount_mmc auto return without any prompt
	for i in 1 2 3 4 5 6 7 8; do
		if [ "$i" -ne 4 ]; then
			create_mmc "$i"
	        fs_checker "$i"
		    mount_mmc "$i"
		fi
	done
}

HWlogMount ()
{
    PartitionName=log
    mkdir -p /mnt/jffs2_2
    dev=`lsmtd | grep $PartitionName | awk -F' ' '{print $1}' | sed -e 's/mtd/mtdblock/'`
    mount -t jffs2 -o sync /dev/$dev /mnt/jffs2_2
}

##mega specific variables
mplane_log_dir="/var/log/$log_dir_name/mplane"
unloading_nor_dir="/var/log/$log_dir_name/nor"
last_save_log="/var/log/$log_dir_name/last_save_log.tgz"
last_startup_pcap="/var/log/$log_dir_name/startup_ptp.tar.gz"

function mega_save_last_log(){
    if [ ! -d "$unloading_nor_dir" ]; then
        mkdir -p $unloading_nor_dir
    fi

    if [ ! -d "$app_log_dir" ]; then
        mkdir -p $app_log_dir
    fi

    if [ ! -d "$mplane_log_dir" ]; then
        mkdir -p $mplane_log_dir
    else
        if [ -f "$unloading_nor_dir/messages" ]
        then
            tar -zcvf $last_save_log $mplane_log_dir $unloading_nor_dir/operation.log* $last_startup_pcap $unloading_nor_dir/messages*
        else
            tar -zcvf $last_save_log $mplane_log_dir $unloading_nor_dir/operation.log* $last_startup_pcap
        fi
        rm -Rf $mplane_log_dir/*
        rm -f $unloading_nor_dir/operation.log*
        rm -f $unloading_nor_dir/messages*
        rm -f $last_startup_pcap
    fi
}

mega_create_o_ran_link_folder(){
	if [ -e "/etc/sysrepo" ];then
		if [ -L "/etc/sysrepo" ]; then
			echo "the /etc/sysrepo file is a soft link"
		else
			echo "the /etc/sysrepo file is not a soft link"
			rm -rf /etc/sysrepo/
			mkdir -p /tmp/sysrepo
			ln -s /tmp/sysrepo /etc/sysrepo
		fi
	else
		echo "/etc/sysrepo folder does not exist"
		mkdir -p /tmp/sysrepo
		ln -s /tmp/sysrepo /etc/sysrepo
	fi
    mkdir -p /mnt/jffs2_2/sysrepo
    mkdir -p /mnt/jffs2_2/log
    mkdir -p /mnt/jffs2_2/users

    ln -s /mnt/jffs2_2/sysrepo /O-RAN/config/sysrepo
    ln -s /mnt/jffs2_2/log /O-RAN/config/log
    ln -s /mnt/jffs2_2/users /O-RAN/config/users
}

start () 
{
	echo "##---Storage for PLAT:[$BASE_PLAT] init---" > /dev/console
	case "$BASE_PLAT" in
		zulu)
			zulu_mnt_boot_spec
			check_app_log_directory
			zulu_save_mplane_log
			zulu_create_o_ran_link_folder
			;;
		mega)
			HWlogMount
			mega_create_o_ran_link_folder
			mega_save_last_log
			;;			
		fara)
			fara_mount_all
			sleep 1
			check_app_log_directory
			check_emmc_log_directory
			# Can't do compress in P memory
			#save_mplane_log
			create_o_ran_link_folder
			#n8n20n28b specific
			if [ "x$hw_type" = "xoru6229_n8n20n28b" ]; then
				n8n20n28b_create_images_link
				echo "create n8n20n28b images link"
				#delete /var/log/$log_dir_name/last_save_sysmgmt.tgz when initialize update Mplane for n8n20n28b
				rm /var/log/$log_dir_name/last_save_sysmgmt.tgz
				#fix coredump log can not collect in var/log/$log_dir_name/emmc issue
				mkdir -p /var/log/$log_dir_name/emmc/coredump
			fi
			;;
		*)
			fara_mount_all
			sleep 1
			check_app_log_directory
			check_emmc_log_directory
			#save_mplane_log # useless for cis_python and python mplane 
			create_o_ran_link_folder
			#n8n20n28b specific
			if [ "x$hw_type" = "xoru6229_n8n20n28b" ]; then
				n8n20n28b_create_images_link
				echo "create n8n20n28b images link"
				#delete /var/log/$log_dir_name/last_save_sysmgmt.tgz when initialize update Mplane for n8n20n28b
				rm /var/log/$log_dir_name/last_save_sysmgmt.tgz
				#fix coredump log can not collect in var/log/$log_dir_name/emmc issue
				mkdir -p /var/log/$log_dir_name/emmc/coredump
			fi			
			;;
	esac
	clear_excess_packages
	##storage devices ready and check slot app version
	list crc &
}

case "$1" in
  start)
    echo "##---Starting storage setup init---" > /dev/console
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