# What plugin are we building ?
PLUGIN = rptoon
PLUGINAPI = toonapi.h

# Who are the maintainers ?
MAINTAINERS = antonk

# Additional objects
CSRC=						        \
	$(CURDIR)/toonink.c				\
	$(CURDIR)/toonpaint.c			    	\
	$(CURDIR)/toongeo.c             		\
	$(CURDIR)/toonmaterial.c

#Allow us to include header for appropriate platform
#CFLAGS += -I$(CURDIR)/$(RWTARGET)
CFLAGS += -I$(CURDIR)/generic


####################
#Xbox
####################

xboxCSRC +=						\
	$(CURDIR)/$(RWTARGET)/brutesilhouette.c		\
	$(CURDIR)/$(RWTARGET)/edgerender.c		\
	$(CURDIR)/$(RWTARGET)/crease.c			\
	$(CURDIR)/$(RWTARGET)/xboxpaint.c

####################
#d3d8
####################

d3d8CSRC +=						\
	$(CURDIR)/$(RWTARGET)/brutesilhouette.c		\
	$(CURDIR)/$(RWTARGET)/edgerender.c		\
	$(CURDIR)/$(RWTARGET)/crease.c			\
	$(CURDIR)/$(RWTARGET)/d3d8paint.c

####################
#d3d9
####################

d3d9CSRC +=						\
	$(CURDIR)/$(RWTARGET)/brutesilhouette.c		\
	$(CURDIR)/$(RWTARGET)/edgerender.c		\
	$(CURDIR)/$(RWTARGET)/crease.c			\
	$(CURDIR)/$(RWTARGET)/d3d9paint.c

####################
#opengl
####################

openglCSRC =					        \
	$(CURDIR)/generic/brutesilhouette.c		\
	$(CURDIR)/generic/genericedgerender.c		\
	$(CURDIR)/generic/crease.c			\
	$(CURDIR)/$(RWTARGET)/openglpaint.c

####################
#GCN
####################

gcnCSRC +=					        \
	$(CURDIR)/$(RWTARGET)/silhouetterender.c	\
	$(CURDIR)/$(RWTARGET)/paintrender.c		\
	$(CURDIR)/$(RWTARGET)/creaserender.c		\
	$(CURDIR)/$(RWTARGET)/edgerender.c		\
	$(CURDIR)/$(RWTARGET)/pipelines.c

####################
#null
####################

nullCSRC =						\
	$(CURDIR)/generic/brutesilhouette.c		\
	$(CURDIR)/generic/genericedgerender.c		\
	$(CURDIR)/generic/crease.c			\
	$(CURDIR)/generic/genericpipes.c

nullgcnCSRC = $(nullCSRC)
nullskyCSRC = $(nullCSRC)
nullxboxCSRC = $(nullCSRC)

#####################
# SKY2
#####################

sky2CSRC +=						\
	$(CURDIR)/$(RWTARGET)/toonplatform.c		\
	$(CURDIR)/$(RWTARGET)/toonsky.c			\

######################################################################

ifndef BASEDIR
BASEDIR = $(CURDIR)/../..
endif
include $(BASEDIR)/makeincl/makeplug

######################################################################

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/generic/%.c
	$(CC) $(CFLAGS) $(CODEGENCFLAGS)  \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

######################################################################

PLUGININCDIR = . $(CURDIR)/$(RWTARGET)
CFLAGS += -I$(CURDIR)/$(RWTARGET)

ifeq ("$(RWTARGET)", "sky2")
CFLAGS +=			\
	-DPLATFORM_NATIVE
endif
