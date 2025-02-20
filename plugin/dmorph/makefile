ifndef RWOPTIONS
RWOPTIONS = $(BASEDIR)/options.mak
endif
-include $(RWOPTIONS)

# What plugin are we building ?
PLUGIN = rpdmorph

# How do we make the plugin header file?
PLUGINAPI = dmorphapi.h

# Who are the maintainers ?
MAINTAINERS = jamesa mattt

# Additional objects
CSRC=						\
	$(CURDIR)/dmphanim.c			\
	$(CURDIR)/dmphgeom.c			\
	$(CURDIR)/dmphintp.c

####################
# PS2 specific stuff
####################
ifeq ("$(RWTARGET)", "sky2")

sky2CSRC =					\
	$(CURDIR)/$(RWTARGET)/dmorphpipe.c	\

CFLAGS +=					\
	-DPLATFORM_PIPELINES			\
	-I./$(RWTARGET)

endif #("$(RWTARGET)", "sky2")

####################
# GCN specific stuff
####################
ifeq ("$(RWTARGET)", "gcn")

gcnCSRC =					\
	$(CURDIR)/$(RWTARGET)/dmorphpipelines.c	\
	$(CURDIR)/$(RWTARGET)/dmorphcommon.c	\
	$(CURDIR)/$(RWTARGET)/dmorphasm.c	\
	$(CURDIR)/$(RWTARGET)/dmorphpipeg.c

CFLAGS +=					\
	-DPLATFORM_PIPELINES			\
	-DPLATFORM_STREAM			\
	-DPLATFORM_DATA				\
	-DPLATFORM_API				\
	-I./$(RWTARGET)

ifeq ($(NOASM), 1)
RWEQUDEP =
else #($(NOASM), 1)
RWEQUDEP =					\
	$(CURDIR)/$(RWTARGET)/dmorphequ.i
endif # ($(NOASM), 1)

endif #("$(RWTARGET)", "gcn")

####################
# Xbox specific stuff
####################
ifeq ("$(RWTARGET)", "xbox")

xboxCSRC =					\
	$(CURDIR)/$(RWTARGET)/dmorphpipelines.c	\
	$(CURDIR)/$(RWTARGET)/dmorphpipeg.c

CFLAGS +=					\
	-DPLATFORM_PIPELINES			\
	-DPLATFORM_GEOM_DTOR         		\
	-DPLATFORM_DATA				\
	-I./$(RWTARGET)

endif #("$(RWTARGET)", "xbox")

####################

ifndef BASEDIR
BASEDIR = $(CURDIR)/../..
endif
include $(BASEDIR)/makeincl/makeplug
