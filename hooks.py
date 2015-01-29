#!/usr/bin/python

def get_secret_for_user(user, ipparam):
    print "Looking up user %s with ipparam %s" % (user, ipparam)
    return "user_secret"

def allowed_address_hook(ip):
    return True

def chap_check_hook():
    return True

def ip_up_notifier(ifname, ouraddr, hisaddr):
    print "ip_up_notifier"

def ip_down_notifier(arg):
    print "ip_down_notifier"

def auth_up_notifier(arg):
    print "auth_up_notifier"

def link_down_notifier(arg):
    print "link_down_notifier"
