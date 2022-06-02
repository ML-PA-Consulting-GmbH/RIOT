#!/bin/sh

DOSE_DIR="$(dirname $(readlink -f $0))"
USE_WRITE=0

create_tap() {
    ip tuntap add ${TAP} mode tap user ${USER}
    sysctl -w net.ipv6.conf.${TAP}.forwarding=0
    sysctl -w net.ipv6.conf.${TAP}.accept_ra=1
    ip link set ${TAP} up
}

remove_tap() {
    ip tuntap del ${TAP} mode tap
}

cleanup() {
    echo "Cleaning up..."
    remove_tap
    if [ -n "${UHCPD_PID}" ]; then
        kill ${UHCPD_PID}
    fi
    if [ -n "${DHCPD_PIDFILE}" ]; then
        kill "$(cat ${DHCPD_PIDFILE})"
        rm "${DHCPD_PIDFILE}"
    fi
    trap "" INT QUIT TERM EXIT
}

start_uhcpd() {
    ${UHCPD} ${TAP} ${PREFIX} > /dev/null &
    UHCPD_PID=$!
}

start_dhcpd() {
    DHCPD_PIDFILE=$(mktemp)
    ${DHCPD} -d -p ${DHCPD_PIDFILE} ${TAP} ${PREFIX} 2> /dev/null
}

if [ "$1" = "-d" ] || [ "$1" = "--use-dhcpv6" ]; then
    USE_DHCPV6=1
    USE_WRITE=1
    shift 1
else
    USE_DHCPV6=0
fi

if [ "$1" = "-u" ] || [ "$1" = "--use-uhcp" ]; then
    USE_UHCP=1
    USE_WRITE=1
    shift 1
else
    USE_UHCP=0
fi

if [ "$1" = "-w" ] || [ "$1" = "--write" ]; then
    USE_WRITE=1
    shift 1
fi

PORT=$1
TAP=$2
shift 2

BAUDRATE=115200
START_DOSE=0

if [ ${USE_UHCP} -eq 1 ] || [ ${USE_DHCPV6} -eq 1 ]; then
    PREFIX=$1
    shift 1
else
    PREFIX=none
fi

[ -z "${PORT}" -o -z "${TAP}" -o -z "${PREFIX}" ] && {
    echo "usage: $0 [-w|--write] [-d|--use-dhcp] [-u|--use-uhcp] " \
         "<serial-port> <tap-device> <prefix> " \
         "[baudrate]"
    exit 1
}

[ ! -z $1 ] && {
    BAUDRATE=$1
}

if [ ${USE_WRITE} -eq 1 ]; then
    ARGS="-w"
fi

trap "cleanup" INT QUIT TERM EXIT

create_tap && \
if [ ${USE_DHCPV6} -eq 1 ]; then
    DHCPD="$(readlink -f "${DOSE_DIR}/../dhcpv6-pd_ia/")/dhcpv6-pd_ia.py"
    start_dhcpd
    START_DOSE=$?
elif [ ${USE_UHCP} -eq 1 ]; then
    UHCPD="$(readlink -f "${DOSE_DIR}/../uhcpd/bin")/uhcpd"
    start_uhcpd
    START_DOSE=$?
fi

[ ${START_DOSE} -eq 0 ] && "${DOSE_DIR}/dose" ${ARGS} -b ${BAUDRATE} ${TAP} ${PORT}
