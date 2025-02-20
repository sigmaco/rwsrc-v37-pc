PLUGIN = rtimport

# Who are the maintainers ?
MAINTAINERS = ads blakem johns jamesc adamj

# since this tool contains multiple object files - define the extra ones
# here. The standard ones are added by 'maketool'

CSRC =						\
	$(CURDIR)/nhssplit.c			\
	$(CURDIR)/nhsstats.c			\
	$(CURDIR)/nhsutil.c			\
	$(CURDIR)/nhsscheme.c			\
	$(CURDIR)/nullkd.c			\
	$(CURDIR)/nhsworld.c 

ifndef BASEDIR
BASEDIR = $(CURDIR)/../..
endif
include $(BASEDIR)/makeincl/maketool
