#!/bin/sh

# ===== 配置参数 =====
RETRY_MAX=60
RETRY_DELAY=0.5

# ===== 颜色定义 =====
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# ===== 工具函数 =====
write_gpio() {
  local num=$1
  local val=$2
  gpioset gpiochip1 "$num"="$val"
}

read_gpio() {
  local num=$1
  gpioget gpiochip1 "$num"
}

write_fpga() {
  local addr=$1
  local val=$2
  ccfpga fpga w "$addr" "$val"
}

read_fpga() {
  local addr=$1
  ccfpga fpga r "$addr" | awk -F'= ' '{print $2}'
}

print_exec_time() {
  echo "Script execution time ${SECONDS} seconds" > /dev/console
}

check_ext_rst() {
  if [ "$(fw_printenv reset_reason | awk -F'=' '{print $2}')" = "EXTERNAL" ]; then
    echo "Cold reset!"
    return 0
  else
    echo "Warm reset!"
    return 1
  fi
}

check_sw_rst() {
  local appleP1a="oru6229_apple"
  local hwVersion=$(ubus call db read '{"key":"/prod_db/version"}' | grep value | tr -dc '[:alnum:]:' | awk -F ':' '{print $2}')
  local hwType=$(ubus call boardenv get '{"key":"hw_type"}' | grep value | tr -dc '[:alnum:]:_-' | awk -F ':' '{print $2}')

  echo "Device info: ${hwType}-${hwVersion}"
  case "${hwType}" in
    "${appleP1a}")
      return 0
      ;;
    *)
      return 1
      ;;
  esac

  #should not be here
  return 1
}

check_gsm_product() {
  local ratGsm="GSM"
  local supportRat=$(ubus call boardenv get '{"key":"legacy_rat"}' | grep value | tr -dc '[:alnum:]:_-' | awk -F ':' '{print $2}')

  echo "Device info: Support Rat Type-${supportRat}"
  case "${supportRat}" in
    "${ratGsm}")
      return 0
      ;;
    *)
      return 1
      ;;
  esac

  #should not be here
  return 1
}

trap print_exec_time EXIT
# 重试机制检查函数
check_with_retry() {
  local expected=$1
  local cmd=$2
  local desc=$3
  local retry_time=$4

  local count=0
  local result

  if ! [[ "$retry_time" =~ ^[0-9]+$ ]] || (( retry_time > RETRY_MAX )); then
    retry_time=RETRY_MAX
  fi

  while (( count < retry_time )); do
    result=$(eval "$cmd")
    #echo "result = $result"
    if [[ "$result" == "$expected" ]]; then
      echo -e "${GREEN}[OK]${NC} $desc (value: $result)" > /dev/console
      return 0
    fi
    ((count++))
    sleep "$RETRY_DELAY"
  done

  echo -e "${RED}[FAIL]${NC} $desc (expected: $expected, got: $result)" > /dev/console
  return 1
}

# 使能 Main
enable_main() {
  local setVal=0x1f
  local checkVal=0x5b6d

  if check_gsm_product; then
    echo "Main check, Device support GSM!"
    setVal=0x7f
    checkVal=0x16db6d
  fi

  write_fpga 0x481c 0x0
  write_fpga 0x481c ${setVal}
  check_with_retry "${checkVal}" "read_fpga 0x481d" "CRC Check 1" "4"
  if [ $? -eq 1 ]; then
    echo "CRC Check 1 failed, need try again later!"
    return 1
  fi

  return 0
}

# 使能 Aux
enable_aux() {
  local setVal=0x1f
  local checkVal=0x5b6d

  if check_gsm_product; then
    echo "Aux check, Device support GSM!"
    setVal=0x7f
    checkVal=0x16db6d
  fi

  write_fpga 0x00403015 0x0
  write_fpga 0x00403015 ${setVal}
  check_with_retry "${checkVal}" "read_fpga 0x00403016" "CRC Check 2" "4"
  if [ $? -eq 1 ]; then
    echo "Establish Data Link failed!"
    return 1
  fi

  return 0
}

# ===== 步骤执行 =====
SECONDS=0
echo -e "${YELLOW}########### Start set up interlink for AUX fpga! ###########${NC}" > /dev/console
echo -e "${YELLOW}=== Step 0: Reset Aux ===${NC}" > /dev/console

reset_pin=0
if check_sw_rst; then
  echo "Entering SW Reset..."
  reset_pin="25"
  if [ -n "$AUX_SRST_GPIO" ]; then
    reset_pin="$AUX_SRST_GPIO"
  else
    echo "Get SW_RST pin failed! use default reset pin"
  fi
  if check_ext_rst; then
    echo "Skip Reset Aux"
  else
    write_gpio $reset_pin 0
    write_gpio $reset_pin 1
  fi
else
  echo "Entering HW Reset..."
  reset_pin="78"
  if [ -n "$AUX_POR_RST_GPIO" ]; then
    reset_pin="$AUX_POR_RST_GPIO"
  else
    echo "Get HW_RST pin failed! use default reset pin"
  fi
  write_gpio $reset_pin 0
  write_gpio $reset_pin 1
fi
check_with_retry "1" "read_gpio 12" "Aux reset check"
if [ $? -eq 1 ]; then
  echo "Reset Aux failed!"
  exit 1
fi
sleep 0.5

echo -e "${YELLOW}=== Extra Step 0.5: Release AXI Interlink Reset ===${NC}" > /dev/console
write_fpga 0x1083 0x0
sleep 0.5

echo -e "${YELLOW}=== Step 1: Reset AXI Interconnect ===${NC}" > /dev/console
write_gpio 85 1
sleep 0.5
write_fpga 0x4800 0x4
sleep 0.5

echo -e "${YELLOW}=== Step 2: QPLL Reset & Check Lock ===${NC}" > /dev/console
write_gpio 86 1
write_gpio 86 0
sleep 0.5
write_fpga 0x4802 0x1
write_fpga 0x4802 0x0
check_with_retry "0xf" "read_fpga 0x4803" "QPLL lock status"
if [ $? -eq 1 ]; then
  echo "QPLL Reset & Check Lock failed!"
  exit 1
fi

echo -e "${YELLOW}=== Step 3: Chip2Chip Reset & Link Check ===${NC}" > /dev/console
write_gpio 85 1
sleep 0.5
write_fpga 0x4800 0x4
write_gpio 85 0
sleep 0.5
write_fpga 0x4800 0x0
check_with_retry "0x8c" "read_fpga 0x4801" "Chip2Chip Link Status Main"
if [ $? -eq 1 ]; then
  echo "Chip2Chip Reset & Link Check failed!"
  exit 1
fi

check_with_retry "0x8c" "read_fpga 0x403001" "Chip2Chip Link Status AUX"
if [ $? -eq 1 ]; then
  echo "Chip2Chip Reset & Link Check failed!"
  exit 1
fi

MAX_RETRY=5
MAX_ENABLE_RETRY=3
success=false
for ((i=1; i<=MAX_RETRY; i++)); do
  echo -e "${YELLOW}=== Step 4: Establish Data Link ===${NC}" > /dev/console
  write_fpga 0x481f 0x3
  write_fpga 0x481f 0x0
  check_with_retry "0x3" "read_fpga 0x4820" "InterlinkQPLL Reset&Check Main"
  if [ $? -eq 1 ]; then
    echo "InterlinkQPLL Reset&Check Main failed!"
    continue
  fi

  write_fpga 0x00403018 0x3
  write_fpga 0x00403018 0x0
  check_with_retry "0x3" "read_fpga 0x00403019" "InterlinkQPLL Reset&Check AUX"
  if [ $? -eq 1 ]; then
    echo "InterlinkQPLL Reset&Check AUX failed!"
    continue
  fi

:<<EOF
write_fpga 0x481c 0x0
write_fpga 0x481c 0x1f
check_with_retry "0x5b6d" "read_fpga 0x481d" "CRC Check 1" "1"
if [ $? -eq 1 ]; then
  echo "CRC Check 1 failed, need try again later!"
fi

write_fpga 0x00403015 0x0
write_fpga 0x00403015 0x1f
check_with_retry "0x5b6d" "read_fpga 0x00403016" "CRC Check 2"
if [ $? -eq 1 ]; then
  echo "Establish Data Link failed!"
  exit 1
fi

sleep 1
write_fpga 0x481c 0x0
write_fpga 0x481c 0x1f
check_with_retry "0x5b6d" "read_fpga 0x481d" "CRC Check 1"
if [ $? -eq 1 ]; then
  echo "Establish Data Link failed!"
  exit 1
fi
EOF

  for ((i=1; i<=MAX_ENABLE_RETRY; i++)); do
      echo "=== Attempt $i: Try enable_main then enable_aux ==="
      if enable_main && enable_aux; then
          echo "Success: enable_main then enable_aux"
          success=true
          break
      fi

      sleep 0.5

      echo "=== Attempt $i: Try enable_aux then enable_main ==="
      if enable_aux && enable_main; then
          echo "Success: enable_aux then enable_main"
          success=true
          break
      fi

      sleep 0.5

      echo "Attempt $i failed, retrying..."
  done

  if $success; then
    break
  fi
done
if $success; then
    echo "All good, continuing the rest of the script..."
else
    echo "All attempts failed after $MAX_RETRY tries."
    exit 1
fi

echo -e "${YELLOW}=== Initialization Completed ===${NC}" > /dev/console
