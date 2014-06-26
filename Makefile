# Makefile for PPPD_PYHOOK plugin
#
# Uncomment this line to get more debug messages

#
#DEBUG=y
PYTHON_VERSION=$(shell python -c "import sys;t='{v[0]}.{v[1]}'.format(v=list(sys.version_info[:2]));sys.stdout.write(t)")
PPPD_VERSION=$(shell pppd --version 2>&1 | cut -d' ' -f3  | tr -d '\n')
PLUGIN=pppd_pyhook.so
DESTINATION=/usr/lib64/pppd/$(PPPD_VERSION)
CC=gcc
LD=ld 
CFLAGS=-I/usr/include/pppd -I/usr/include/python$(PYTHON_VERSION) -O2 -fPIC
LDFLAGS=-lc -lpython$(PYTHON_VERSION)

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

