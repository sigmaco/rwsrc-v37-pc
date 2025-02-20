# What plugin are we building ?

PLUGIN = rtfsyst
PLUGINAPI = fsystapi.h

PLUGININCDIR =		\
	.		\
	./$(RWTARGET)	\
	./$(RWOS)

# Who are the maintainers ?
MAINTAINERS = ejay

all: rwplugin

ifndef BASEDIR
BASEDIR = $(CURDIR)/../..
endif

# Include options for all the platforms
include $(BASEDIR)/makeincl/makeopt

# Switch the build process include depending on the platform and target
ifeq ($(RWOS),win)
MAKEINCLUDE = $(CURDIR)/makefile.$(RWOS)
else
ifeq ($(RWOS),mac)
MAKEINCLUDE = $(CURDIR)/makefile.$(RWOS)
else
MAKEINCLUDE = $(CURDIR)/makefile.$(RWTARGET)
endif
endif

# Only include if we have a supported platform
ifneq ($(wildcard $(MAKEINCLUDE)), )
include $(MAKEINCLUDE)

# Include the rules to build the plugin for all platforms.
include $(BASEDIR)/makeincl/makeplug

# Else the platform doesn't support a file system.
else

all:
	@echo File system tool unsupported on $(RWTARGET) platform.

endif
