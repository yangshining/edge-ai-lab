#!/bin/sh
xml_file=upgrade.xml
if [ $# -eq 2 ]; then
path=$1
number=$2
iszulu=0
else
/etc/init.d/tftpd-hpa stop
path=$1
number=$2
path49=$3
iszulu=0
fi

mount_check() {
	local mtdblock=$1
	if [ -z "${mtdblock}" ]; then
		return 0
	fi
	local is_mount=`cat /proc/mounts |grep $mtdblock |wc -l`
	return ${is_mount}
}

create_partition() {
local BOOT_DISK='/dev/mmcblk0'

if [ ${iszulu} -eq 0 ]; then
local PART_PAR_first=16
local PART_PAR_second=2097168
local PART_OFFS=2097151
else
local PART_PAR_first=2048
local PART_PAR_second=1026048
local PART_OFFS=1023999
fi

if [ -e '/dev/mmcblk0p3' ]; then
umount /dev/mmcblk0p3
fi

if [ -e '/dev/mmcblk0p2' ]; then
    umount /dev/mmcblk0p2
	echo -e "\033[31mcreate partition first, PART_PAR:${PART_PAR_first}, PART_OFFS:${PART_OFFS}, PART_END:default.\033[0m\n"
fdisk ${BOOT_DISK} << EOF
p
n
p
1
${PART_PAR_firstd}
+${PART_OFFS}

p
w
q
EOF
sleep 1
    umount /dev/mmcblk0p*
    mkfs.ext4 -O ^metadata_csum -F /dev/mmcblk0p1
    if [ ${iszulu} -eq 0 ]; then
    mount /dev/mmcblk0p1 /media/sd-mmcblk0p1
    fi
    elif [ -e '/dev/mmcblk0p1' ]; then
    umount /dev/mmcblk0p1
    echo -e "\033[31mcreate partition second, PART_PAR:${PART_PAR_second}, PART_OFFS:${PART_OFFS}, PART_END:default.\033[0m\n"
fdisk ${BOOT_DISK} << EOF
p
n
p
2
${PART_PAR_second}
+${PART_OFFS}

p
w
q
EOF
sleep 1
    umount /dev/mmcblk0p*
    mkfs.ext4 -O ^metadata_csum -F /dev/mmcblk0p2
    if [ ${iszulu} -eq 0 ]; then
    mount /dev/mmcblk0p2 /media/sd-mmcblk0p2
    fi
    else
    echo -e "\033[31mcreate partition, PART_PAR_first:${PART_PAR_first}, PART_PAR_second:${PART_PAR_second}, PART_OFFS:${PART_OFFS}, PART_END:default.\033[0m\n"
fdisk ${BOOT_DISK} << EOF
p
n
p
1
${PART_PAR_firstd}
+${PART_OFFS}
n
p
2
${PART_PAR_second}
+${PART_OFFS}
p
w
q
EOF
sleep 1
    umount /dev/mmcblk0p*
    mkfs.ext4 -O ^metadata_csum -F /dev/mmcblk0p1
    mkfs.ext4 -O ^metadata_csum -F /dev/mmcblk0p2
    if [ ${iszulu} -eq 0 ]; then
    mount /dev/mmcblk0p1 /media/sd-mmcblk0p1
    mount /dev/mmcblk0p2 /media/sd-mmcblk0p2
    mount /dev/mmcblk0p3 /media/sd-mmcblk0p3
    fi
fi

if [ -e '/dev/mmcblk0p3' ]; then
if [ ${iszulu} -eq 0 ]; then
mount /dev/mmcblk0p3 /media/sd-mmcblk0p3
fi
fi

}


create_partition_third() {
local BOOT_DISK='/dev/mmcblk0'
local PART_PAR_third=4194320
local PART_OFFS_third=3309551
echo -e "\033[31mcreate partition, PART_PAR_third:${PART_PAR_third}, PART_OFFS_third:${PART_OFFS_third}.\033[0m\n"
umount /dev/mmcblk0p*
fdisk ${BOOT_DISK} << EOF
p
n
p
3
${PART_PAR_third}
+${PART_OFFS_third}
p
w
q
EOF
sleep 1
    umount /dev/mmcblk0p*
    mkfs.ext4 -O ^metadata_csum -F /dev/mmcblk0p3
    mount /dev/mmcblk0p1 /media/sd-mmcblk0p1
    mount /dev/mmcblk0p2 /media/sd-mmcblk0p2
    mount /dev/mmcblk0p3 /media/sd-mmcblk0p3
}



if [ -e '/dev/mmcblk0p3' ]; then
    echo -e "\033[33mno need to create emmc3\033[0m\n"
else
    if [[ "$1" == *"/media/sd-mmcblk0p3"* ]]; then
        terminals=$(who | awk '{print $2}')
        for terminal in $terminals; do
            echo -e "\033[33mPath contains '/media/sd-mmcblk0p3', it was not initialized yet,\nput package in somewhere else\n\033[0m" > /dev/${terminal}
        done
        exit -1
    fi
    create_partition_third
fi  

if [ -e '/dev/mmcblk0p1' ] && [ -e '/dev/mmcblk0p2' ]; then
    echo -e "\033[33memmc has been create\033[0m\n"
else
    create_partition
fi

echo -e "\033[33mwrite_emmc_start\033[0m\n"
if [ $# -eq 2 ]; then
    if [ $2 == 1 ]; then
        mount_check /dev/mmcblk0p1
        if [ $? == 0 ]; then
            echo -e "\033[31emmc1 is not mounted...\033[0m\n"
            mkfs.ext4 -O ^metadata_csum -F /dev/mmcblk0p1
            mount /dev/mmcblk0p1 /media/sd-mmcblk0p1
        else
            echo -e "\033[33emmc1 is mounted...\033[0m\n"
        fi
        cp $1 /media/sd-mmcblk0p1/lm.bin
        sync
    elif [ $2 == 2 ]; then
        mount_check /dev/mmcblk0p2
        if [ $? == 0 ]; then
            echo -e "\033[31emmc2 is not mounted...\033[0m\n"
            mkfs.ext4 -O ^metadata_csum -F /dev/mmcblk0p2
            mount /dev/mmcblk0p2 /media/sd-mmcblk0p2
        else
            echo -e "\033[33emmc2 is mounted...\033[0m\n"
        fi
        cp $1 /media/sd-mmcblk0p2/lm.bin
        sync
    fi
else
    mount_check /dev/mmcblk0p${2}
    if [ $? == 0 ]; then
        echo -e "\033[31emmc${2} is not mounted...\033[0m\n"
        mkfs.ext4 -O ^metadata_csum -F /dev/mmcblk0p${2}
        mount /dev/mmcblk0p${2} /media/sd-mmcblk0p${2}
    else
        echo -e "\033[33emmc${2} is mounted...\033[0m\n"
    fi
    cp $1 /media/sd-mmcblk0p${2}
    cp $3 /media/sd-mmcblk0p${2}
    rm $1 $3
    sync
    /etc/init.d/tftpd-hpa start
fi
echo 3 > /proc/sys/vm/drop_caches
echo -e "\033[33mwrite_emmc_end\033[0m\n"
file_name=$(basename "$path")
dir=$(echo "$path" | sed 's/'"${file_name}"'//g')
if [ -f "$dir$xml_file" ];then
    rm -f ${dir}*.bin ${dir}*.xml
fi
