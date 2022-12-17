DEBUGGER=-g
DEFINE=
CFLAGS=$(DEBUGGER) $(DEFINE) -fPIC -Wall -Werror
CC=gcc
OUTPUT=

all: nav_system drivers 

nav_system: nav_system.o dyn.o boot.o strtable.o disk_array.o block_list.o mm_util.o 
	$(CC) $(DEBUGGER) -o $@ $^ -ldl

drivers: disk_array_driver strtable_driver block_list_driver

disk_array_driver: disk_array_driver.o disk_array.o mm_util.o
	$(CC) $(DEBUGGER) -o $@ $^

strtable_driver: strtable_driver.o strtable.o mm_util.o
	$(CC) $(DEBUGGER) -o $@ $^

block_list_driver: block_list_driver.o block_list.o mm_util.o
	$(CC) $(DEBUGGER) -o $@ $^

restore: restore_params restore_nav restore_log

restore_params:
	cp db/params.arr db/params.arr-backup || :
	cp db-corrupt/params.arr_CORRUPT db/params.arr
	chmod u+w db/params.arr

restore_nav:
	cp db/nav.stb db/nav.stb-backup || :
	cp db-corrupt/nav.stb_CORRUPT db/nav.stb
	chmod u+w db/nav.stb

restore_log:
	cp db/log.ll db/log.ll-backup || :
	cp db-corrupt/log.ll_CORRUPT db/log.ll
	chmod u+w db/log.ll

clean:
	rm -f *.so *.o *.ll *.stb *.arr dyn/*.so
	rm -f nav_system disk_array_driver strtable_driver block_list_driver 
