#!/bin/sh

PORT="9999"
DESTINATIONS="172.16.1.100 172.16.1.101 172.16.1.102 172.16.1.103 
              172.16.1.104 172.16.1.105 172.16.1.106 172.16.1.107 
              172.16.1.108 172.16.1.109 172.16.1.110 172.16.1.111 
              172.16.1.112 172.16.1.113 172.16.1.114 172.16.1.115 
              172.16.1.116 172.16.1.117 172.16.1.118 172.16.1.119"
BCASTD="`dirname $0`/bcastd"
OPTIONS=""

case $1 in
start)
	( $BCASTD $OPTIONS $PORT $DESTINATIONS & ) && echo -n 'bcastd'
	;;

stop)
	killall bcastd
	;;

*)
	echo "Usage: `basename $0` {start|stop}" >&2
	;;
esac

# EOF
