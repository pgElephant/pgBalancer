SUBDIRS = pgmd5 pgenc pgproto watchdog

bin_SCRIPTS =  pgbalancer_setup

pgbalancer_setup:
	ln -s ../test/pgbalancer_setup .

clean-local:
	-rm -f $(bin_SCRIPTS)
