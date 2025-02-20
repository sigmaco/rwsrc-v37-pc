# What plugin are we building ?

PLUGIN = rprandom

# Who are the maintainers ?
MAINTAINERS = johns

CSRC =	\
    $(CURDIR)/mertwist.c

ifndef BASEDIR
BASEDIR = $(CURDIR)/../..
endif
include $(BASEDIR)/makeincl/makeplug

EXCLUDED = $(CURDIR)/mertwist.h
DOXYSRC = $(filter-out $(EXCLUDED), $(CSRC) $(subst .c,.h,$(CSRC)))
