# makefile for easymcu proj

# TOPDIR:=$(CURDIR)
# your app makefile
include proj/app/app.mk 

# third part lib
include proj/third/third.mk

# common makefile
include proj/proj.mk