#!/usr/bin/env bash
#
# pgbalancer regression test driver.
#
# usage: regress.sh [test_name]
# -i install directory of pgpool
# -b pgbench path
# -p installation path of Postgres
# -j JDBC driver path
# -m install (install pgbalancer and use that for tests) / noinstall : Default install
# -s unix socket directory
# -c test pgpool using sample scripts and config files
# -d start pgpool with debug option

dir=`pwd`
MODE=install
PG_INSTALL_DIR=/usr/local/pgsql/bin
PGBALANCER_PATH=/usr/local
JDBC_DRIVER=/usr/local/pgsql/share/postgresql-9.2-1003.jdbc4.jar
#export USE_REPLICATION_SLOT=true
export log=$dir/log
fail=0
ok=0
timeout=0
PGSOCKET_DIR=/tmp,/var/run/postgresql

CRED=$(tput setaf 1)
CGREEN=$(tput setaf 2)
CBLUE=$(tput setaf 4)
CNORM=$(tput sgr0)

# timeout for each test (5 min.)
TIMEOUT=300

function install_pgpool
{
	echo "creating pgbalancer temporary installation ..."
        PGBALANCER_PATH=$dir/temp/installed

	test -d $log || mkdir $log
        
	make install WATCHDOG_DEBUG=1 -C $dir/../../ -e prefix=${PGBALANCER_PATH} >& regression.log 2>&1

	if [ $? != 0 ];then
	    echo "make install failed"
	    exit 1
	fi
	
	echo "moving pgbalancer_setup to temporary installation path ..."
        cp $dir/../pgbalancer_setup ${PGBALANCER_PATH}/pgbalancer_setup
	export PGPOOL_SETUP=$PGBALANCER_PATH/pgbalancer_setup
	echo "moving watchdog_setup to temporary installation path ..."
        cp $dir/../watchdog_setup ${PGBALANCER_PATH}/watchdog_setup
	export WATCHDOG_SETUP=$PGBALANCER_PATH/watchdog_setup
}

function verify_pginstallation
{
	# PostgreSQL bin directory
	PGBIN=`$PG_INSTALL_DIR/pg_config --bindir`
	if [ -z $PGBIN ]; then
		echo "$0: cannot locate pg_config"
		exit 1
	fi

	# PostgreSQL lib directory
	PGLIB=`$PG_INSTALL_DIR/pg_config --libdir`
	if [ -z $PGLIB ]; then
		echo "$0: cannot locate pg_config"
		exit 1
	fi
}

function export_env_vars
{
	if [[ -z "$PGBALANCER_PATH" ]]; then
		# check if pgpool is in the path
		PGBALANCER_PATH=/usr/local
		export PGPOOL_SETUP=$HOME/bin/pgbalancer_setup
		export WATCHDOG_SETUP=$HOME/bin/watchdog_setup
 	fi
	
	if [[ -z "$PGBENCH_PATH" ]]; then
		if [ -x $PGBIN/pgbench ]; then
			PGBENCH_PATH=$PGBIN/pgbench
		else
			PGBENCH_PATH=`which pgbench`
		fi
	fi

	if [ ! -x $PGBENCH_PATH ]; then
		echo "$0] cannot locate pgbench"; exit 1
 	fi
	
	echo "using pgbalancer at "$PGBALANCER_PATH

	export PGBALANCER_VERSION=`$PGBALANCER_PATH/bin/pgbalancer --version 2>&1`

	export PGBALANCER_INSTALL_DIR=$PGBALANCER_PATH
	# where to look for pgpool.conf.sample files.
	export PGPOOLDIR=${PGPOOLDIR:-"$PGBALANCER_INSTALL_DIR/etc"}

	PGPOOLLIB=${PGBALANCER_INSTALL_DIR}/lib
	if [ -z "$LD_LIBRARY_PATH" ];then
	    export LD_LIBRARY_PATH=${PGPOOLLIB}:${PGLIB}
	else
	    export LD_LIBRARY_PATH=${PGPOOLLIB}:${PGLIB}:${LD_LIBRARY_PATH}
	fi

	export TESTLIBS=$dir/libs.sh
	export PGBIN=$PGBIN
	export JDBC_DRIVER=$JDBC_DRIVER
	export PGBENCH_PATH=$PGBENCH_PATH
	export PGSOCKET_DIR=$PGSOCKET_DIR
	export PGVERSION=`$PGBIN/initdb -V|awk '{print $3}'|sed 's/\..*//'`
	export LANG=C

	export ENABLE_TEST=true
}
function print_info
{
	echo ${CBLUE}"*************************"${CNORM}

	echo "REGRESSION MODE          : "${CBLUE}$MODE${CNORM}
	echo "Pgbalancer version        : "${CBLUE}$PGBALANCER_VERSION${CNORM}
	echo "Pgbalancer install path   : "${CBLUE}$PGBALANCER_PATH${CNORM}
	echo "PostgreSQL bin           : "${CBLUE}$PGBIN${CNORM}
	echo "PostgreSQL Major version : "${CBLUE}$PGVERSION${CNORM}
	echo "pgbench                  : "${CBLUE}$PGBENCH_PATH${CNORM}
	echo "PostgreSQL jdbc          : "${CBLUE}$JDBC_DRIVER${CNORM}
	echo ${CBLUE}"*************************"${CNORM}
}

function print_usage
{
	printf "Usage:\n"
	printf "  %s: [Options]... [test_name]\n" $(basename $0) >&2
	printf "\nOptions:\n"
	printf "  -p   DIRECTORY           Postgres installed directory\n" >&2
	printf "  -b   PATH                pgbench installed path, if different from Postgres installed directory\n" >&2
	printf "  -i   DIRECTORY           pgpool installed directory, if already installed pgpool is to be used for tests\n" >&2
	printf "  -m   install/noinstall   make install pgpool to temp directory for executing regression tests [Default: install]\n" >&2
	printf "  -j   FILE                Postgres jdbc jar file path\n" >&2
	printf "  -s   DIRECTORY           unix socket directory\n" >&2
	printf "  -t   TIMEOUT             timeout value for each test (sec)\n" >&2
	printf "  -c                       test pgpool using sample scripts and config files\n" >&2
	printf "  -d                       start pgpool with debug option\n" >&2
	printf "  -?                       print this help and then exit\n\n" >&2
	printf "Please read the README for details on adding new tests\n" >&2

}

trap "echo ; exit 0" SIGINT SIGQUIT

while getopts "p:m:i:j:b:s:t:cd?" OPTION
do
  case $OPTION in
    p)  PG_INSTALL_DIR="$OPTARG";;
    m)  MODE="$OPTARG";;
    i)  PGBALANCER_PATH="$OPTARG";;
    j)  JDBC_DRIVER="$OPTARG";;
    b)  PGBENCH_PATH="$OPTARG";;
    s)  PGSOCKET_DIR="$OPTARG";;
    t)  TIMEOUT="$OPTARG";;
    c)  export TEST_SAMPLES="true";;
    d)  export PGPOOLDEBUG="true";;
    ?)  print_usage
        exit 2;;
  esac
done

shift $(($OPTIND - 1))
if [ "$MODE" = "install" ]; then
	install_pgpool

elif [ "$MODE" = "noinstall" ]; then
	echo not installing pgpool for the tests ...
	if [[ -n "$PGBALANCER_INSTALL_DIR" ]]; then
		PGBALANCER_PATH=$PGBALANCER_INSTALL_DIR
	fi
        export PGPOOL_SETUP=$PGBALANCER_PATH/bin/pgbalancer_setup
        export WATCHDOG_SETUP=$PGBALANCER_PATH/bin/watchdog_setup
else
	echo $MODE : Invalid mode
	exit -1
fi 

verify_pginstallation
export_env_vars
print_info
source $TESTLIBS

#Start executing tests
rm -fr $log
mkdir $log

cd tests

if [ $# -eq 1 ];then
	dirs=`ls|grep $1`
else
	dirs=`ls`
fi

for i in $dirs
do
	# skip if it's not a directory
	if [ ! -d $i ];then
	    continue;
	fi
	cd $i

	# skip the test if there's no test.sh
	if [ ! -f test.sh ];then
		cd ..
		continue;
	fi

	echo -n "testing $i..."
	clean_all
	timeout $TIMEOUT ./test.sh > $log/$i 2>&1
	rtn=$?

	check_segfault
	if [ $? -eq 0 ];then
		echo "fail: Segmentation fault detected" >> $log/$i
		segfault=1
		rtn=1
	fi

	if [ $rtn = 0 ];then
		echo ${CGREEN}"ok."${CNORM}
		ok=`expr $ok + 1`
	elif [ $rtn = 124 ];then
		echo "timeout."
		timeout=`expr $timeout + 1`
	else
		if [ "$segfault" = "1" ];then
			echo ${CRED}"segfault."${CNORM}
		else
			echo ${CRED}"failed."${CNORM}
		fi
		fail=`expr $fail + 1`
	fi

	cd ..

done

total=`expr $ok + $fail + $timeout`

echo "out of $total ok:$ok failed:$fail timeout:$timeout"
