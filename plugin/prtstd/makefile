# What plugin are we building ?

PLUGIN = rpprtstd
PLUGINAPI = prtstdapi.h
LIBNAME = rpprtstd

# Who are the maintainers ?
MAINTAINERS = colinh

OPTDISTCLEAN = localdistclean
OPTCLEAN = localclean

CSRC =                                          \
	$(CURDIR)/eclass.c                      \
	$(CURDIR)/emitter.c                     \
	$(CURDIR)/pclass.c                      \
	$(CURDIR)/prop.c                        \
	$(CURDIR)/pptank.c                      \
	$(CURDIR)/standard.c

#genericCSRC = \
	$(CURDIR)/generic/stdgen.c

#d3d8CSRC 	= $(CURDIR)/$(RWTARGET)/prtstdd3d8.c

#d3d9CSRC 	= $(CURDIR)/$(RWTARGET)/prtstdd3d9.c

#openglCSRC 	= $(CURDIR)/$(RWTARGET)/ .c

#softrasCSRC 	= $(CURDIR)/$(RWTARGET)/ .c

#nullCSRC 	= $(CURDIR)/null/ .c

#
# GCN
#

#gcnCSRC 	= $(CURDIR)/$(RWTARGET)/ .c

#nullgcnCSRC 	= $(CURDIR)/null/ .c

#
# XBOX
#

#xboxCSRC 	= $(CURDIR)/$(RWTARGET)/ .c

#nullxboxCSRC 	= $(CURDIR)/null/ .c

#
# SKY2 specific:-
#

sky2CSRC 	= \
	$(CURDIR)/$(RWTARGET)/stdsky2.c

#nullskyCSRC 	= $(CURDIR)/null/ .c

#
# Make the plugin
#

ifndef BASEDIR
BASEDIR = $(CURDIR)/../..
endif
include $(BASEDIR)/makeincl/makeplug


#
# Indicate where to look for included platform file when generating the
# plugin header.
#
PLUGININCDIR = . $(CURDIR)/$(RWTARGET)

CFLAGS += -I$(CURDIR)/generic

CFLAGS += -I$(CURDIR)/$(RWTARGET)

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/generic/%.c
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/d3d8/%.c
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/d3d9/%.c
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

#####################################################################

localclean: $(LOCALCLEAN)
localdistclean: $(LOCALDISTCLEAN)


