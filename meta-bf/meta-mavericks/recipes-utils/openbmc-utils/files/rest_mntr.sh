#!/bin/sh

SERVICE="rest.py"

check_8080_port(){
	for ((i=0; i<=12; i++)); do
		netstat -tanu | grep 8080 | grep LISTEN
		cmd=$?
		if [ "$cmd" == 0 ]; then
			return
		fi
		sleep 5
	done
}
      
while [ 1 ]
do
	RESULT=`ps | sed -n /${SERVICE}/p`
	if [ "${RESULT:-null}" = null ]; then
	    echo "rest.py not running"
	    /usr/local/bin/setup-rest-api.sh restart
	    check_8080_port
	    if [ -e "/tmp/btools_lock" ]; then
	      rm -f "/tmp/btools_lock"
	      echo "Stale btools_lock exists. Removing it"
	    fi
	fi
	sleep 5
done
