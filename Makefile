CC=gcc
CFLAGS=-Wall -ggdb
SRC=src/NetworkStateNotifier.c src/nsn_printfuncs.c src/nsn_commandlauncher.c
TGT=bin/nsn
LDFLAGS=-lrt 
#-static



all: pc man
man: nsn.8.gz

pc: src/NetworkStateNotifier.c
	$(CC) $(CFLAGS) $(SRC) -o $(TGT) $(LDFLAGS)

install: maninstall pc
	cp bin/nsn  /usr/bin/nsn

maninstall: nsn.8.gz
	mv nsn.8.gz /usr/share/man/man8/.
	@echo 'man pages installed to /usr/share/man/man8'
	@echo 'consider running mandb or makewhatis to update apropos database'

nsn.8.gz: man/nsn.8
	cp man/nsn.8 nsn.8
	gzip nsn.8

clean:
	rm -rf $(TGT)
	rm -rf $(TGT_PPCe500)
	rm -rf *.o
	rm nsn.8.gz
