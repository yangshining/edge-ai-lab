#!/bin/sh

for file in /etc/profile.d/zsys*; do
	if [ -f "$file" ]; then
		echo "Sourcing $file"
		. "$file"
	fi
done

default_debug_ip="192.168.234.106"
env_network_str="debug_network"
env_ipaddr_str="debug_ip"
env_netmask_str="debug_netmask"
env_gateway_str="debug_gateway"
env_debug_mac_str="debug_mac"
debug_interface_name="debug"

debug2_interface_name="debug2"
default_debug2_ip="192.168.234.108"

env_static_ip=`fw_printenv |grep ${env_ipaddr_str} |awk -F '=' '{print $2}'`
env_static_netmask=`fw_printenv |grep ${env_netmask_str} |awk -F '=' '{print $2}'`
env_static_gateway=`fw_printenv |grep ${env_gateway_str} |awk -F '=' '{print $2}'`
env_network_flg=`fw_printenv |grep ${env_network_str} |awk -F '=' '{print $2}'`
env_debug_mac=`fw_printenv |grep ${env_debug_mac_str} |awk -F '=' '{print $2}'`

function check_ip(){
	IP=$1
	VALID_CHECK=$(echo $IP|awk -F. '$1<=255&&$2<=255&&$3<=255&&$4<=255{print "yes"}')
	if echo $IP|grep -E "^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$">/dev/null; then
		if [ ${VALID_CHECK:-no} == "yes" ]; then
			echo "IP $IP available."
			return 0
		else
			echo "IP $IP not available!"
			return -1
		fi
	else
		echo "IP $IP format error!"
		return -2
	fi
}

function check_env_ip(){
	check_ip ${env_static_ip}
	if [ $? -ne 0 ]; then
		return -1
	fi
	check_ip ${env_static_netmask}
	if [ $? -ne 0 ]; then
		return -2
	fi
	check_ip ${env_static_gateway}
	if [ $? -ne 0 ]; then
		return -3
	fi
	return 0
}

function set_debug_ip(){


	current_ip=`ifconfig ${debug_interface_name} |grep "inet addr:" |awk '{print $2}'|cut -c 6-`

	if [ "${env_network_flg}" = "1" ]; then
		check_env_ip
		if [ $? -eq 0 ]; then
			if [ "${current_ip}" = "${env_static_ip}" ]; then
				echo "${debug_interface_name} ipaddr ${env_static_ip} has been modified"
			else
				echo "ipaddr: ${env_static_ip}"
				echo "netmask: ${env_static_netmask}"
				echo "gateway: ${env_static_gateway}"
				ip link set ${debug_interface_name} down
				ifconfig ${debug_interface_name} hw ether ${env_debug_mac}
				ip link set ${debug_interface_name} up
				ip addr flush dev ${debug_interface_name}
				echo "modify ${debug_interface_name} ipaddr ${current_ip} to ${env_static_ip}"
				ip addr add ${env_static_ip}/${env_static_netmask} dev ${debug_interface_name}
				ip ro add default via ${env_static_gateway} dev ${debug_interface_name}
			fi
		else
			echo "modify ${debug_interface_name} ipaddr to ${default_debug_ip}"
			ifconfig ${debug_interface_name} ${default_debug_ip}
		fi
	else
		echo "modify ${debug_interface_name} ipaddr to ${default_debug_ip}"
		ifconfig ${debug_interface_name} ${default_debug_ip}
		case "$VARI_PLAT" in
			sakura|apple)
				#product with debug2 need config ip for it
				echo "modify ${debug2_interface_name} ipaddr to ${default_debug2_ip}"
				ifconfig ${debug2_interface_name} ${default_debug2_ip}
				;;
			*)
				;;
		esac
	fi
}

emimoa_ethconfig() {
    ipaddr="10.11.1."
    ETH="debug"
    ETHADDR="00:0a:35:00:01:"
    pins=(50 49 48)

    cpuid=""
    for pin in ${pins[@]}; do
        value=$(gpioget gpiochip1 $pin 2>/dev/null || echo "E")
        [ "$value" != "0" -a "$value" != "1" ] && value="E"
	echo "##for($pin)---$value -- $cpuid" > /dev/console
        cpuid="${cpuid}${value}"
    done

    for iface in /sys/class/net/*; do
        ifconfig $(basename $iface) down
    done

    declare -A id_map=(
        ["000"]="101"
        ["001"]="102"
        ["010"]="103"
        ["011"]="104"
        ["100"]="105"
        ["101"]="106"
        ["110"]="107"
        ["111"]="108"
    )

    if [ -n "${id_map[$cpuid]}" ]; then
        chip_id=${id_map[$cpuid]}
        echo -e "\nCHIP_ID=\"-c $chip_id\"" >> /etc/default/slave-node
        ifconfig $ETH hw ether "${ETHADDR}${chip_id: -2}"
        ifconfig $ETH ${ipaddr}${chip_id}
        CHIP_ID=$chip_id
    else
        echo "Error: Invalid GPIO values. cpuid=$cpuid"
        return 1
    fi

    echo "##---emimoa set for chip_id: [$chip_id] - ip: [${ipaddr}${chip_id}] - mac: [${ETHADDR}${chip_id: -2}] init---" > /dev/console
    ifconfig $ETH up
    ifconfig lo up
}

sysctl_conf(){
	sysctl -w net.ipv4.conf.all.arp_accept=0
	sysctl -w net.ipv4.conf.default.arp_accept=0
	sysctl -w net.ipv4.icmp_echo_ignore_broadcasts=1
	sysctl -w net.ipv4.conf.all.send_redirects=0
	sysctl -w net.ipv4.conf.default.send_redirects=0
	sysctl -w net.ipv4.tcp_syncookies=1
	sysctl -w net.ipv4.conf.all.log_martians=1
	sysctl -w net.ipv4.conf.all.accept_source_route=0
	sysctl -w net.ipv4.conf.default.accept_source_route=0
	sysctl -w net.ipv4.conf.all.rp_filter=1
	sysctl -w net.ipv4.conf.default.rp_filter=1
	sysctl -w net.ipv6.conf.all.mldv1_unsolicited_report_interval=10000
	sysctl -w net.ipv6.conf.all.mldv2_unsolicited_report_interval=1000
	sysctl -w net.ipv4.ip_forward=0
	sysctl -w net.ipv4.icmp_echo_ignore_all=1
}

start ()
{
    result=$(ubus call boardenv get '{"key":"hw_type"}')
    hw_type=$(echo "$result" | sed -n 's/.*"value":\s*"\([^"]*\)".*/\1/p')
    echo "##checking $hw_type"
    if [ "x$hw_type" = "xoru6229_n8n20n28b" ]; then
        sysctl_conf
        echo "set sysctl conf for ip4/6"
    fi

    ifconfig lo up
    sleep 1
	echo "##---debugip set for SUB-PLAT: [$VARI_PLAT] init---" > /dev/console
	case "$VARI_PLAT" in
		zulu_v2|echo)
			default_debug_ip="10.11.1.100"
			;;
		mega)
			default_debug_ip="192.168.234.102"
			;;
		sakura)
			default_debug_ip="192.168.234.107"
			default_debug2_ip="192.168.235.107"
			;;
		apple)
			default_debug_ip="192.168.234.108"
			default_debug2_ip="192.168.235.108"
			;;
		apricot)
			default_debug_ip="192.168.234.121"
			;;
		grape)
			default_debug_ip="192.168.234.122"
			;;
		blueberry)
			default_debug_ip="192.168.234.123"
			;;
		mru)
			default_debug_ip="192.168.234.109"
			;;
		mu)
			default_debug_ip="192.168.234.125"
			;;
		hu)
			default_debug_ip="192.168.234.126"
			;;
		phoenix)
			default_debug_ip="192.168.234.110"
			;;
		vpk120|emimom)
			default_debug_ip="192.168.234.111"
			;;
		emimoa)
			default_debug_ip="10.11.1.101"
			;;
		*)
			;;
	esac

	echo "##---fheth set for PLAT: [$BASE_PLAT] init---" > /dev/console
	case "$BASE_PLAT" in
		zulu)
			ip link set eth0 down
			ip link set eth0 name fheth0
			;;
		mega)
			set_debug_ip
			ip link set eth0 name fheth0
			ifconfig fheth0 mtu 9000
			ifconfig fheth0 up
			ifconfig fheth0 down
			ip link set eth1 name fheth1
			ifconfig fheth1 mtu 9000
			ifconfig fheth1 up
			ifconfig fheth1 down
			;;
		fara|fara2)
			set_debug_ip
			case "$VARI_PLAT" in
				emimom)
					ip link set switch up
					ip addr add 10.11.1.100/255.255.255.0 dev switch
					;;
				emimoa)
					emimoa_ethconfig
					;;	
				sakura|mu)
					ip link set eth2 name fheth0
					ip link set eth3 name fheth1
					;;
				apple|grape)
					ip link set eth0 name fheth0
					ip link set eth1 name fheth1
					;;
				*)
					ip link set eth1 name fheth0
					ip link set eth2 name fheth1
					;;
			esac
			;;
		*)
			;;
	esac
}

case "$1" in
  start)
    echo "##---Starting network setup init---" > /dev/console
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
