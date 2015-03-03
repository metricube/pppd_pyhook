# Makefile for pppd_pyhook plugin
#
# Uncomment this line to get more debug messages,
# or run `DEBUG=y make`.
#DEBUG=y

HOOKS=hooks.py
HOOKSDIR=/etc/ppp

PLUGIN=pyhook.so
PLUGINDIR=/usr/lib*/pppd/*/

ifdef DEBUG
HOOKSDEBUG=CFLAGS=-DHOOKSDEBUG=1
endif

all: $(PLUGIN)

$(PLUGIN):
	$(HOOKSDEBUG) python setup.py build_ext --inplace

install: all
	install -o root -g root -m 0755 $(PLUGIN) $(PLUGINDIR)
	install -o root -g root -m 0755 $(HOOKS) $(HOOKSDIR)
	#chcon -t pppd_exec_t $(PLUGINDIR)/$(PLUGIN)
	#chcon -t pppd_exec_t $(HOOKSDIR)/$(HOOKS)

clean:
	rm -rf build $(PLUGIN)
