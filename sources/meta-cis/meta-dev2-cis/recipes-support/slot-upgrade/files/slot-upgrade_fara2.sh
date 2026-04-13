#!/bin/sh
path=$1
number=$2
xml_file=upgrade.xml

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
local PART_OFF_256=256M
local PART_OFF_768=768M

mkdir /media/sd-mmcblk0p{1..8}

if [ -e '/dev/mmcblk0p1' ]; then
    echo -e "\033[33memmc1 exists.\033[0m\n"
else
    echo -e "\033[31mcreate partition first, size is ${PART_OFF_256}.\033[0m\n"
    umount /dev/mmcblk0p*
fdisk ${BOOT_DISK} << EOF
p
n
p
1

+${PART_OFF_256}

p
w
q
EOF
sleep 1
    umount /dev/mmcblk0p1
    mkfs.ext4 -O ^metadata_csum -F /dev/mmcblk0p1
    mount /dev/mmcblk0p1 /media/sd-mmcblk0p1
fi

if [ -e '/dev/mmcblk0p2' ]; then
    echo -e "\033[33memmc2 exists.\033[0m\n"
else
    echo -e "\033[31mcreate partition second, size is ${PART_OFF_256}.\033[0m\n"
    umount /dev/mmcblk0p*
fdisk ${BOOT_DISK} << EOF
p
n
p
2

+${PART_OFF_256}

p
w
q
EOF
sleep 1
    umount /dev/mmcblk0p2
    mkfs.ext4 -O ^metadata_csum -F /dev/mmcblk0p2
    mount /dev/mmcblk0p2 /media/sd-mmcblk0p2
fi

if [ -e '/dev/mmcblk0p3' ]; then
    echo -e "\033[33memmc3 exists.\033[0m\n"
else
    echo -e "\033[31mcreate partition third, size is ${PART_OFF_768}.\033[0m\n"
    umount /dev/mmcblk0p*
fdisk ${BOOT_DISK} << EOF
p
n
p
3

+${PART_OFF_768}

p
w
q
EOF
sleep 1
    umount /dev/mmcblk0p3
    mkfs.ext4 -O ^metadata_csum -F /dev/mmcblk0p3
    mount /dev/mmcblk0p3 /media/sd-mmcblk0p3
fi

if [ -e '/dev/mmcblk0p4' ]; then
    echo -e "\033[33memmc4 exists.\033[0m\n"
else
    echo -e "\033[31mcreate partition Extended, size is default.\033[0m\n"
    umount /dev/mmcblk0p*
fdisk ${BOOT_DISK} << EOF
p
n
e


p
w
q
EOF
sleep 1
fi

if [ -e '/dev/mmcblk0p5' ]; then
    echo -e "\033[33memmc5 exists.\033[0m\n"
else
    echo -e "\033[31mcreate partition fifth, size is ${PART_OFF_768}.\033[0m\n"
    umount /dev/mmcblk0p*
fdisk ${BOOT_DISK} << EOF
p
n

+${PART_OFF_768}

p
w
q
EOF
sleep 1
    umount /dev/mmcblk0p5
    mkfs.ext4 -O ^metadata_csum -F /dev/mmcblk0p5
    mount /dev/mmcblk0p5 /media/sd-mmcblk0p5
fi

if [ -e '/dev/mmcblk0p6' ]; then
    echo -e "\033[33memmc6 exists.\033[0m\n"
else
    echo -e "\033[31mcreate partition sixth, size is ${PART_OFF_768}.\033[0m\n"
    umount /dev/mmcblk0p*
fdisk ${BOOT_DISK} << EOF
p
n

+${PART_OFF_768}

p
w
q
EOF
sleep 1
    umount /dev/mmcblk0p6
    mkfs.ext4 -O ^metadata_csum -F /dev/mmcblk0p6
    mount /dev/mmcblk0p6 /media/sd-mmcblk0p6
fi

if [ -e '/dev/mmcblk0p7' ]; then
    echo -e "\033[33memmc7 exists.\033[0m\n"
else
    echo -e "\033[31mcreate partition seventh, size is ${PART_OFF_768}.\033[0m\n"
    umount /dev/mmcblk0p*
fdisk ${BOOT_DISK} << EOF
p
n

+${PART_OFF_768}

p
w
q
EOF
sleep 1
    umount /dev/mmcblk0p7
    mkfs.ext4 -O ^metadata_csum -F /dev/mmcblk0p7
    mount /dev/mmcblk0p7 /media/sd-mmcblk0p7
fi

if [ -e '/dev/mmcblk0p8' ]; then
    echo -e "\033[33memmc8 exists.\033[0m\n"
else
    echo -e "\033[31mcreate partition eighth, size is ${PART_OFF_768}.\033[0m\n"
    umount /dev/mmcblk0p*
fdisk ${BOOT_DISK} << EOF
p
n


p
w
q
EOF
sleep 1
    umount /dev/mmcblk0p8
    mkfs.ext4 -O ^metadata_csum -F /dev/mmcblk0p8
    mount /dev/mmcblk0p8 /media/sd-mmcblk0p8
fi
}

if [ -e '/dev/mmcblk0p1' ] && [ -e '/dev/mmcblk0p2' ] && [ -e '/dev/mmcblk0p3' ] && [ -e '/dev/mmcblk0p4' ] && [ -e '/dev/mmcblk0p5' ] && [ -e '/dev/mmcblk0p6' ] && [ -e '/dev/mmcblk0p7' ] && [ -e '/dev/mmcblk0p8' ]; then
    echo -e "\033[33memmc has been create\033[0m\n"
else
    create_partition
fi

echo -e "\033[33mwrite_emmc_start\033[0m\n"
emmc_off=4
if [[ "$2" =~ ^[12]$ ]]; then
    part_num=$(($2+$emmc_off))
    mount_check /dev/mmcblk0p${part_num}
    if [ $? == 0 ]; then
        echo -e "\033[31memmc${part_num} is not mounted...\033[0m\n"
        mkfs.ext4 -O ^metadata_csum -F /dev/mmcblk0p${part_num}
        mount /dev/mmcblk0p${part_num} /media/sd-mmcblk0p${part_num}
    else
        echo -e "\033[33memmc${part_num} is mounted...\033[0m\n"
    fi
    if [ $# == 3 ]; then
        #if exsit 3 param, it means that we are updating a tool package, the param 1 is tool package, param 3 is common package
        #use tool package as start up image first, common pakage as next step start up image
        cp $1 /media/sd-mmcblk0p${part_num}/lm.bin
        cp $3 /media/sd-mmcblk0p${part_num}/lm_common.bin
        rm $1 $3
    else
        cp $1 /media/sd-mmcblk0p${part_num}/lm.bin
    fi
    sync
else
    echo -e "\033[31mslot num [$2] is not a digit num, update failed\033[0m"
    exit 1
fi
echo 3 > /proc/sys/vm/drop_caches
echo -e "\033[33mwrite_emmc_end\033[0m\n"
file_name=$(basename "$path")
dir=$(echo "$path" | sed 's/'"${file_name}"'//g')
if [ -f "$dir$xml_file" ];then
    rm -f ${dir}*.bin ${dir}*.xml
fi
