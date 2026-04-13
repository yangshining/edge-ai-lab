#!/bin/sh

REV=1
LEVEL="C"
PREFIX="aux-fpga"
#echo ${LM_RSTATE}
filename="RESTATE.rev"
bif_conf="slave_output_zboot.bif"

#REV=$(($(ls -la ${PREFIX}_$(date +'%m%d')* | tail -n 1 | grep -o '[0-9]\+\.bin' | sed 's/\.bin//')+1))

#LM_RSTATE=$(date +'%m%d')${LEVEL}${REV}

petalinux-package --boot --bif ${bif_conf} -o ${PREFIX}_tmp.bin --force > /dev/null

chmod 755 aux_pack_handler
#./aux_pack_handler r ${PREFIX}_tmp.bin ${PREFIX}_rec.bin
#./aux_pack_handler d ${PREFIX}_rec.bin ${PREFIX}.bin

./aux_pack_handler r ${PREFIX}_tmp.bin ${PREFIX}.bin 7 1024
rm -rf ${PREFIX}_tmp.bin

#rm -rf ${PREFIX}_rec.bin

echo -e "\033[1;32mGenerate aux-fpga successful! file: \033[0m\033[1;42m${PREFIX}.bin\033[0m"

if [ $1 = "raw" ]; then
   ./aux_pack_handler d ${PREFIX}.bin ${PREFIX}-raw.bin
   echo -e "\033[1;32mGenerate aux-fpga raw successful! file: \033[0m\033[1;42m${PREFIX}-raw.bin\033[0m"
fi