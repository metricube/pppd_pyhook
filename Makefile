# Makefile for PPPD_PYHOOK plugin
#
# Uncomment this line to get more debug messages

#
#DEBUG=y
PYV=$(shell python -c "import sys;t='{v[0]}.{v[1]}'.format(v=list(sys.version_info[:2]));sys.stdout.write(t)")
PLUGIN=pppd_pyhook.so
PPPV=2.4.5
DESTINATION=/usr/lib64/pppd/$(PPPV)
CC=gcc
LD=ld 
CFLAGS=-I/usr/include/pppd -I/usr/include/python$(PYV) -O2 -fPIC
LDFLAGS=-lc -lpython$(PYV)

ifdef DEBUG
CFLAGS += -DDEBUG=1
endif

all : $(PLUGIN) 

pppd_pyhook.so: main.o
	$(LD) -shared -o pppd_pyhook.so main.o $(LDFLAGS)

main.o: main.c
	$(CC) $(CFLAGS) -c -o main.o main.c

install: all
	cp -f pppd_pyhook.so $(DESTINATION)/pyhook.so
	chcon -t pppd_exec_t $(DESTINATION)/pyhook.so
	cp -n hooks.py /etc/ppp/hooks.py
	chcon -t pppd_exec_t /etc/ppp/hooks.py

clean:
	rm -rf $(PLUGIN) *.o *.so *~

