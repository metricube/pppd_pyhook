# Python hooks for pppd

A plugin for pppd that allows for pppd hooks and notifications to be
implemented in python. Currently it defines hooks sufficient to use
an alternative CHAP secret provider, and to run custom code on
ip-up and ip-down events.

Note that each pppd instance runs its own instance of the `hooks.py`
script, so you can set global variables and use them in different
hooks (functions) during the same session with no trouble. They will
persist for as long as pppd is running.

See `hooks.py` for details on the hooks available and their arguments.

To disable one or more hooks just comment it out and it'll be ignored
next time pppd runs.

## Installation

You'll need a compiler, pppd and python dev packages. See instructions
for specific Linux distros below.

CentOS:

	sudo yum install -y gcc make git ppp ppp-devel python-devel

Ubuntu:

	sudo apt-get install -y build-essential git ppp ppp-dev python-dev

Build and install:

	make
	sudo make install

Make sure to load the `pyhook.so` plugin in `/etc/ppp/options[.xxx]` by
adding the following line to it:

	plugin pyhook.so

## Troubleshooting & Limitations

This plugin has been tested on CentOS 6 and 7, and Ubuntu (trusty) x86-64.

If you get spurious errors, check if SELinux is enabled. It limits greatly
what pppd (and hence the python hooks) can do. Check the SELinux audit
log (usually `/var/log/audit/audit.log`) for details.
