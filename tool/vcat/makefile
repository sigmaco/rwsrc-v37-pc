# What plugin are we building ?

PLUGIN = rtvcat

# Who are the maintainers ?
MAINTAINERS = williame

# for Intel platforms

INTELSRC =					\
	$(CURDIR)/xbstrip.cpp

nullCSRC     = $(INTELSRC)
nullxboxCSRC = $(INTELSRC)
xboxCSRC     = $(INTELSRC)
d3d8CSRC     = $(INTELSRC)
d3d9CSRC     = $(INTELSRC)
softrasCSRC  = $(INTELSRC)
openglCSRC   = $(INTELSRC)

OPTTARGET=recurse   # for specific configuration, build extra versions of the libs - see below

ifndef BASEDIR
BASEDIR = $(CURDIR)/../..
endif
include $(BASEDIR)/makeincl/maketool

ifeq ("$(RWOS)", "xbox")
CFLAGS+= -GX
endif

ifeq ("$(RWOS)", "win")
# Not all x86 runtimes support -GX/__except_list, e.g. MW/CW
CFLAGS+= -GX-
endif

#need multithreaded DLL version for Maya exporter
ifeq ("$(OPTEXT)", "md")

 CFLAGS+=-MD

 LFLAGS+=-NODEFAULTLIB
endif
#need multithreaded static version for Max exporter
ifeq ("$(OPTEXT)", "mt")

 CFLAGS+=-MT

 LFLAGS+=-NODEFAULTLIB
endif

recurse:
# In order for the exporters to build and integrate this plugin, versions for static
# and dynamic runtime are needed. These also must be built with VisualC
# Note this is only called for a NULL library build on the PC
ifeq ("$(RWOS)", "win")
ifeq ("$(RWTARGET)", "null")
ifeq ("$(OPTEXT)", "")
	$(MAKE) RWOPTIONS=$(RWOPTIONS) clean
	$(MAKE) RWOPTIONS=$(RWOPTIONS) RWCOMPILER=visualcpp OPTEXT=md
	$(MAKE) RWOPTIONS=$(RWOPTIONS) RWCOMPILER=visualcpp OPTEXT=md clean
	$(MAKE) RWOPTIONS=$(RWOPTIONS) RWCOMPILER=visualcpp OPTEXT=mt
	$(MAKE) RWOPTIONS=$(RWOPTIONS) RWCOMPILER=visualcpp OPTEXT=mt clean
endif
endif
endif

DOXYCSRC = 					\
	$(CURDIR)/rtvcat.c

DOXYCPPSRC = 					\
	$(CURDIR)/xbstrip.cpp		\
	$(CURDIR)/rtvcat.cpp

CVERIFY = $(filter-out $(CURDIR)/rtvcat.c $(INTELSRC), $(CSRC)) $(CURDIR)/rtvcat.cpp
DOXYSRC = $(filter-out $(CURDIR)/rtvcat.c $(INTELSRC), $(CSRC)) $(CURDIR)/rtvcat.cpp
