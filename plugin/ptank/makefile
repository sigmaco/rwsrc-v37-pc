# What plugin are we building ?

PLUGIN = rpptank
PLUGINAPI = ptankapi.h
LIBNAME = rpptank

# Who are the maintainers ?
MAINTAINERS = alexh

OPTDISTCLEAN = localdistclean
OPTCLEAN = localclean

# Platform independent ptank source - added for
# certain platforms (see below).

PTANKCSRC =					\
	$(CURDIR)/ptank.c

PTANKCROOT = \
	$(notdir $(basename $(PTANKCSRC)))

PTANKCOBJS = \
	$(addprefix $(CURDIR)/$(OBJDIR)/, $(addsuffix .$(O), $(PTANKCROOT)))

genericCSRC = $(CURDIR)/generic/ptankgen.c	\
	      $(PTANKCSRC)

d3d8CSRC        = $(CURDIR)/$(RWTARGET)/ptankd3d8.c \
                  $(PTANKCSRC)

d3d9CSRC        = $(CURDIR)/$(RWTARGET)/ptankd3d9.c \
                  $(PTANKCSRC)

openglCSRC 	= $(CURDIR)/$(RWTARGET)/ptankopengl.c \
		  $(PTANKCSRC)

softrasCSRC 	= $(CURDIR)/$(RWTARGET)/ptanksoftras.c \
                  $(PTANKCSRC)

#nullCSRC 	= $(CURDIR)/null/ .c

#
# GCN
#

gcnCSRC 	= $(CURDIR)/$(RWTARGET)/ptankgcn.c \
		  $(CURDIR)/$(RWTARGET)/ptankgcncallbacks.c \
		  $(CURDIR)/$(RWTARGET)/ptankgcnrender.c \
		  $(CURDIR)/$(RWTARGET)/ptankgcntransforms.c \
		  $(CURDIR)/$(RWTARGET)/ptankgcn_nc_ppm.c \
		  $(CURDIR)/$(RWTARGET)/ptankgcn_cc_ppm.c \
		  $(CURDIR)/$(RWTARGET)/ptankgcn_nc_cs_nr.c \
		  $(CURDIR)/$(RWTARGET)/ptankgcn_cc_cs_nr.c \
		  $(CURDIR)/$(RWTARGET)/ptankgcn_nc_pps_nr.c \
		  $(CURDIR)/$(RWTARGET)/ptankgcn_cc_pps_nr.c \
		  $(CURDIR)/$(RWTARGET)/ptankgcn_nc_cs_ppr.c \
		  $(CURDIR)/$(RWTARGET)/ptankgcn_cc_cs_ppr.c \
		  $(CURDIR)/$(RWTARGET)/ptankgcn_nc_pps_ppr.c \
		  $(CURDIR)/$(RWTARGET)/ptankgcn_cc_pps_ppr.c \
		  $(CURDIR)/$(RWTARGET)/ptankgcngeneric.c


#nullgcnCSRC 	= $(CURDIR)/null/ .c

#
# XBOX
#

xboxCSRC       = $(CURDIR)/$(RWTARGET)/ptankxbox.c             \
                 $(CURDIR)/$(RWTARGET)/ptankxboxopt.c          \
		 $(PTANKCSRC)

#nullxboxCSRC 	= $(CURDIR)/null/ .c

#
# SKY2 specific:-
#

sky2CSRC = 	$(CURDIR)/$(RWTARGET)/ptanksky.c			\
		$(CURDIR)/$(RWTARGET)/spr2dgen/spr2dgen.c		\
		$(CURDIR)/$(RWTARGET)/spr3dgen/spr3dgen.c	 	\
		$(CURDIR)/$(RWTARGET)/fastpipe/fastpipe.c		\
		$(CURDIR)/$(RWTARGET)/genpipe/genpipe.c	  		\
		$(CURDIR)/$(RWTARGET)/clonpipe/clonpipe.c		\
		$(CURDIR)/$(RWTARGET)/rotuv2pc/rotuv2pc_dt.c		\
	 	$(CURDIR)/$(RWTARGET)/rotuv2vc/rotuv2vc_dt.c		\
		$(CURDIR)/$(RWTARGET)/rotuv4pc/rotuv4pc_dt.c		\
		$(CURDIR)/$(RWTARGET)/rotuv4vc/rotuv4vc_dt.c		\
		$(CURDIR)/$(RWTARGET)/spruv2pc/spruv2pc_dt.c		\
		$(CURDIR)/$(RWTARGET)/spruv2vc/spruv2vc_dt.c		\
		$(CURDIR)/$(RWTARGET)/spruv4pc/spruv4pc_dt.c		\
		$(CURDIR)/$(RWTARGET)/spruv4vc/spruv4vc_dt.c		\
           	$(CURDIR)/$(RWTARGET)/skyptank.c			\
	 	$(PTANKCSRC)

#nullskyCSRC 	= $(CURDIR)/null/ .c

sky2DSMSRC +=								\
	$(CURDIR)/$(RWTARGET)/spr2dgen/spr2dgenPFL.dsm			\
	$(CURDIR)/$(RWTARGET)/spr2dgen/spr2dgenPxL.dsm			\
	$(CURDIR)/$(RWTARGET)/spr2dgen/spr2dgenPFx.dsm			\
	$(CURDIR)/$(RWTARGET)/spr2dgen/spr2dgenPxx.dsm			\
	$(CURDIR)/$(RWTARGET)/spr2dgen/spr3dgenPFL.dsm			\
	$(CURDIR)/$(RWTARGET)/spr2dgen/spr3dgenPxL.dsm			\
	$(CURDIR)/$(RWTARGET)/spr2dgen/spr3dgenPFx.dsm			\
	$(CURDIR)/$(RWTARGET)/spr2dgen/spr3dgenPxx.dsm

DVPASMFLAGS += -I$(CURDIR)/sky2/spr2dgen/				\
			   -I$(CURDIR)/sky2/spr3dgen/


SKYPIPEDIRS :=	sky2/spr2dgen			\
			sky2/spr3dgen			\
			sky2/rotuv2pc			\
			sky2/rotuv2vc			\
			sky2/rotuv4pc			\
			sky2/rotuv4vc			\
			sky2/spruv2pc			\
			sky2/spruv2vc			\
			sky2/spruv4pc			\
			sky2/spruv4vc			

sky2HDRS += $(addsuffix /stddata.h,$(SKYPIPEDIRS))

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

CFLAGS += -I$(CURDIR)/$(RWTARGET)

CFLAGS += -I$(CURDIR)/generic

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/generic/%.c
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/$(RWTARGET)/%.c
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

ifeq ($(RWTARGET),sky2)

# Rule for stddata.h headers
%stddata.h : %stddata.i %stddata.sed
	$(SED) -f $*stddata.sed $< > $@

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/$(RWTARGET)/spr2dgen/%.c
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/$(RWTARGET)/spr2dgen/%.dsm
	$(DVPASM) $(DVPASMFLAGS) -alhs $(DVPAOUT)$@ $< > $(CURDIR)/$(OBJDIR)/$*.lsd

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/$(RWTARGET)/spr3dgen/%.c
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/$(RWTARGET)/spr3dgen/%.dsm
	$(DVPASM) $(DVPASMFLAGS) -alhs $(DVPAOUT)$@ $< > $(CURDIR)/$(OBJDIR)/$*.lsd

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/$(RWTARGET)/fastpipe/%.c
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/$(RWTARGET)/genpipe/%.c
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/$(RWTARGET)/clonpipe/%.c
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/$(RWTARGET)/rotuv2pc/%.c
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/$(RWTARGET)/rotuv2vc/%.c
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/$(RWTARGET)/rotuv4pc/%.c
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/$(RWTARGET)/rotuv4vc/%.c
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/$(RWTARGET)/spruv2pc/%.c
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/$(RWTARGET)/spruv2vc/%.c
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/$(RWTARGET)/spruv4pc/%.c
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/$(RWTARGET)/spruv4vc/%.c
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

endif

#####################################################################

########################
# softras specific stuff
########################

ifeq ("$(RWTARGET)", "softras")

CFLAGS +=								\
	-I$(BASEDIR)/driver/softras/hybrid/include

endif # RWTARGET==softras

#####################################################################

localclean: $(LOCALCLEAN)
localdistclean: $(LOCALDISTCLEAN)


