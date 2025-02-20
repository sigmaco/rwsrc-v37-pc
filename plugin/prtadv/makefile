# What plugin are we building ?

PLUGIN = rpprtadv
PLUGINAPI = prtadvapi.h
LIBNAME = rpprtadv

# Who are the maintainers ?
MAINTAINERS = colinh

OPTDISTCLEAN = localdistclean
OPTCLEAN = localclean

CSRC =                                          \
	$(CURDIR)/rpprtadv.c                    \
	$(CURDIR)/prtemt.c                    	\
	$(CURDIR)/prtmcol.c                    	\
	$(CURDIR)/prttexst.c                  	\
	$(CURDIR)/circle.c                    	\
	$(CURDIR)/sphere.c                    	\
	$(CURDIR)/plist.c                    	

#genericCSRC = \
	$(CURDIR)/generic/stdgen.c

#d3d7CSRC 	= $(CURDIR)/$(RWTARGET)/ .c

#d3d8CSRC 	= $(CURDIR)/$(RWTARGET)/prtadvd3d8.c

#d3d9CSRC 	= $(CURDIR)/$(RWTARGET)/prtadvd3d9.c

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

# sky2CSRC 	= \

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


