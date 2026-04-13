#!/bin/bash

file_path="$1"  # first arg as file path

echo 3 > /proc/sys/vm/drop_caches

if [[ ! -f "$file_path" ]]; then
  echo -n "fail1"
  exit 1
fi


rm -f *.bin *.xml

file_name=$(basename "$file_path")
dir=$(echo "$file_path" | sed 's/'"${file_name}"'//g')
cd $dir
#file_header=$(head -c 262 "$file_name" 2>/dev/null)
#file_header=$(dd if="$file_name" bs=1 count=262 2>/dev/null)

#hex_output=$(hexdump -n 262 -e '1/1 "%02x"' "$file_name")
#file_header=$(echo "$hex_output" | sed 's/\(..\)/\\x\1/g' | xargs -0 printf "%b")
xml_file="upgrade.xml"
if tar -tf "$file_name" | grep -q "$xml_file"; then
    tar -xf "$file_name" "$xml_file"
fi

  result=$(ubus call boardenv get '{"key":"hw_type"}')
  hw_type=$(echo "$result" | grep -o '"value": "[^"]*' | awk -F'"' '{print $4}')
  #hw_type="oru6226_b28a"
  #echo "rewrite hw_type $hw_type"

  #xml parse and search
  prefix=$(awk -v hw_type="$hw_type" '
    /<package>/ {
      package_found = 1
    }
    package_found && /<REC/ {
      for (i = 1; i <= NF; i++) {
        if ($i ~ /hw_type=".*"/) {
          gsub(/.*hw_type="/, "", $i)
          gsub(/".*/, "", $i)
          if ($i == hw_type) {
            hw_type_found = 1
          } else {
            hw_type_found = 0
          }
          break
        }
      }
    }
    package_found && hw_type_found && /sw_product_name=/ {
      gsub(/.*sw_product_name="/, "")
      gsub(/".*/, "")
      print
      exit
    }
    /<\/package>/ {
      package_found = 0
    }
  ' "$xml_file")

  if [[ -n "$prefix" ]]; then
    
	matching_files=$(tar -tf "$file_name" | grep -E "^$prefix")
	if [ -n "$matching_files" ]; then
		tar -xf "$file_name" "$matching_files"
		echo -n "$dir""$matching_files"
	else
	  echo -n "fail2"
      rm -f *.bin *.xml
	fi
  else
    echo -n "fail3"
    rm -f *.bin *.xml
  fi
  echo 3 > /proc/sys/vm/drop_caches
