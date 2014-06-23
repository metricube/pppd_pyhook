#!/usr/bin/python

import syslog
import os, sys
class logsyslog:
    def write(self, data):
        syslog.syslog(data)

# Pushes output to syslog for debugging etc
syslog.openlog('pyhook[%u]' % os.getpid() )
sys.stderr=sys.stdout=logsyslog()

def get_secret_for_user(user):
    print "Looking up user %s" % user
    return "user_secret"

def allowed_address_hook(ip):
    return True

def chap_check_hook():
    return True

