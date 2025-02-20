# What plugin are we building ?

PLUGIN = rpcollis
PLUGINAPI = rpcollis.h
PLUGININCDIR = .

# Who are the maintainers ?
MAINTAINERS = mattt

# Additional objects
CSRC  =								\
	$(CURDIR)/ctdata.c				\
	$(CURDIR)/ctbuild.c				\
	$(CURDIR)/ctquery.c				\
	$(CURDIR)/colldata.c			\
	$(CURDIR)/collbuild.c			\
	$(CURDIR)/collquery.c

ifndef BASEDIR
BASEDIR = $(CURDIR)/../..
endif
include $(BASEDIR)/makeincl/makeplug

# Add header dependency for convenience
$(PLUGINHEADER): $(patsubst %.c, %.h, $(CSRC))
