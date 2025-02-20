# What plugin are we building ?

PLUGIN = rpcrowd

OPTDISTCLEAN = localdistclean

# Who are the maintainers ?
MAINTAINERS = jamesa mattt

sky2HDRS    = $(CURDIR)/sky2/stddata.h

sky2CSRC    = $(CURDIR)/sky2/crowdsky.c

sky2DSMSRC += $(CURDIR)/sky2/nulltrans.dsm \
              $(CURDIR)/sky2/RpCrowd_II.dsm

gcnCSRC     = $(CURDIR)/gcn/crowdgcn.c

xboxHDRS    = $(CURDIR)/xbox/crowdhdr.h

xboxCSRC    = $(CURDIR)/xbox/crowdxbox.c \
              $(CURDIR)/crowdstaticbb.c

d3d8CSRC    = $(CURDIR)/d3d8/crowdd3d8.c \
              $(CURDIR)/crowdstaticbb.c

d3d9CSRC    = $(CURDIR)/d3d9/crowdd3d9.c \
              $(CURDIR)/crowdstaticbb.c

openglCSRC  = $(CURDIR)/opengl/crowdopengl.c \
              $(CURDIR)/crowdstaticbb.c

genericCSRC = $(CURDIR)/crowdgeneric.c \
              $(CURDIR)/crowdstaticbb.c

nullCSRC    = $(CURDIR)/null/crowdnull.c
nullgcnCSRC = $(CURDIR)/null/crowdnull.c
nullskyCSRC = $(CURDIR)/null/crowdnull.c
nullxboxCSRC = $(CURDIR)/null/crowdnull.c

ifndef BASEDIR
BASEDIR = $(CURDIR)/../..
endif
include $(BASEDIR)/makeincl/makeplug

# Indicate where to look for included platform file when generating the
# plugin header.
#
PLUGININCDIR = . $(CURDIR)/$(RWTARGET)
CFLAGS += -I$(CURDIR)/$(RWTARGET)

ifeq ("$(RWTARGET)", "sky2")

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/sky2/%.dsm
	$(DVPASM) $(DVPASMFLAGS) -alhs $(DVPAOUT)$@ $< > $(CURDIR)/$(OBJDIR)/$*.lsd

# Rule for stddata.h headers
%stddata.h : %stddata.i %stddata.sed
	$(SED) -f $*stddata.sed $< > $@

DVPASMFLAGS += -I$(CURDIR)/sky2

endif

#####################
# xbox specific stuff
#####################

ifeq ("$(RWTARGET)", "xbox")

localdistclean: $(LOCALDISTCLEAN)
	$(RM) $(RWOS)/crowdhdr.h

# add new rules for vertex shader headers
%.h : %.vsh
	$(VSHASM) $(VSHFLAGS) /I xbox $^

$(CURDIR)/$(OBJDIR)/crowdxbox.$(O): \
	$(CURDIR)/xbox/crowdxbox.c \
	$(CURDIR)/xbox/crowdhdr.h

else # RWTARGET==xbox

localdistclean: $(LOCALDISTCLEAN)

endif # RWTARGET==xbox
