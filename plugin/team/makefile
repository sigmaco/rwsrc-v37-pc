# What plugin are we building ?
PLUGIN = rpteam

# Who are the maintainers ?
MAINTAINERS = jamesa

OPTDISTCLEAN = localdistclean
OPTCLEAN = localclean

# Additional objects
CSRC =									\
	$(CURDIR)/team.c						\
	$(CURDIR)/teamdictionary.c					\
	$(CURDIR)/teamlod.c						\
	$(CURDIR)/teammaterial.c					\
	$(CURDIR)/teammatfx.c						\
	$(CURDIR)/teamoptimize.c					\
	$(CURDIR)/teampipes.c						\
	$(CURDIR)/teamplayer.c						\
	$(CURDIR)/teamshadow.c						\
	$(CURDIR)/teamskin.c						\
	$(CURDIR)/teamstatic.c						\
	$(CURDIR)/teamstream.c

#####################
# sky2 specific stuff
#####################

sky2CSRC =								\
	$(CURDIR)/$(RWTARGET)/teamskyrender.c				\
	$(CURDIR)/$(RWTARGET)/teamsky.c					\
	$(CURDIR)/$(RWTARGET)/teamshadowsky.c				\
	$(CURDIR)/$(RWTARGET)/teammatfxsky.c                     	\
	$(CURDIR)/$(RWTARGET)/teamskinsky.c                      	\
	$(CURDIR)/$(RWTARGET)/teamstaticsky.c

###################
# pc specific stuff
###################

openglCSRC =					     			\
	$(CURDIR)/$(RWTARGET)/teamrenderopengl.c			\
	$(CURDIR)/$(RWTARGET)/teammatfxopengl.c          		\
	$(CURDIR)/$(RWTARGET)/teamshadowopengl.c         		\
	$(CURDIR)/$(RWTARGET)/teamskinopengl.c           		\
	$(CURDIR)/$(RWTARGET)/teamstaticopengl.c			\
	$(CURDIR)/$(RWTARGET)/teamopengl.c

d3d8CSRC =                                         			\
	$(CURDIR)/$(RWTARGET)/teamrenderd3d8.c      			\
	$(CURDIR)/$(RWTARGET)/teammatfxd3d8.c       			\
	$(CURDIR)/$(RWTARGET)/teamshadowd3d8.c      			\
	$(CURDIR)/$(RWTARGET)/teamskind3d8.c        			\
	$(CURDIR)/$(RWTARGET)/teamstaticd3d8.c		    		\
	$(CURDIR)/$(RWTARGET)/teamd3d8.c

d3d9CSRC =                                         			\
	$(CURDIR)/$(RWTARGET)/teamrenderd3d9.c      			\
	$(CURDIR)/$(RWTARGET)/teammatfxd3d9.c       			\
	$(CURDIR)/$(RWTARGET)/teamshadowd3d9.c      			\
	$(CURDIR)/$(RWTARGET)/teamskind3d9.c        			\
	$(CURDIR)/$(RWTARGET)/teamstaticd3d9.c		    		\
	$(CURDIR)/$(RWTARGET)/teamd3d9.c

#####################
# Xbox specific stuff
#####################

xboxHDRS =								\
	$(CURDIR)/$(RWOS)/skinshdr.h					\
	$(CURDIR)/$(RWOS)/skinshdr_unlit.h				\
	$(CURDIR)/$(RWOS)/rigidshdr.h					\
	$(CURDIR)/$(RWOS)/dualskinshdr.h                        	\
	$(CURDIR)/$(RWOS)/envmapskinshdr.h

xboxCSRC =								\
	$(CURDIR)/$(RWOS)/teammatfxxbox.c      				\
	$(CURDIR)/$(RWOS)/teamrenderxbox.c      			\
	$(CURDIR)/$(RWOS)/teamshadowxbox.c           			\
	$(CURDIR)/$(RWOS)/teamskinxbox.c             			\
	$(CURDIR)/$(RWOS)/teamstaticxbox.c           			\
	$(CURDIR)/$(RWOS)/teamxbox.c           				\
	$(CURDIR)/$(RWOS)/xboxmatbl.c		

####################
# GCN specific stuff
####################

gcnCSRC =								\
	$(CURDIR)/$(RWOS)/teamgcnrender.c				\
	$(CURDIR)/$(RWOS)/teammatfxgcn.c             			\
	$(CURDIR)/$(RWOS)/teamshadowgcn.c            			\
	$(CURDIR)/$(RWOS)/teamskingcn.c              			\
	$(CURDIR)/$(RWOS)/teamstaticgcn.c				\
	$(CURDIR)/$(RWOS)/teamgcn.c

#####################
# null specific stuff
#####################

nullCSRC =								\
	$(CURDIR)/null/teamnull.c

nullgcnCSRC = $(nullCSRC)
nullskyCSRC = $(nullCSRC)
nullxboxCSRC = $(nullCSRC)

######################################################################
######################################################################

ifndef BASEDIR
BASEDIR = $(CURDIR)/../..
endif
include $(BASEDIR)/makeincl/makeplug

ifeq ($(findstring null,$(RWTARGET)),null)
CFLAGS += -I./null
else
CFLAGS += -I./$(RWTARGET)
endif

######################################################################
######################################################################

#####################
# sky2 specific stuff
#####################

ifeq ("$(RWTARGET)", "sky2")

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/$(RWTARGET)/%.c
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) $(COUT)$@ $<

endif

#####################################################################

#####################
# xbox specific stuff
#####################

ifeq ("$(RWTARGET)", "xbox")

LOCALDISTCLEAN=localxboxdistclean

localxboxdistclean:
	$(RM) $(RWOS)/skinshdr.h
	$(RM) $(RWOS)/skinshdr_unlit.h
	$(RM) $(RWOS)/rigidshdr.h
	$(RM) $(RWOS)/dualskinshdr.h
	$(RM) $(RWOS)/envmapskinshdr.h

# add new rules for vertex shader headers
%.h : %.vsh
	$(VSHASM) $(VSHFLAGS) /I xbox $^


# add new rules for vertex shader headers
%.h : %.vsh
	$(VSHASM) $(VSHFLAGS) /I xbox $^

$(OBJDIR)/xboxmatbl.$(O):			\
	xboxmatbl.c				\
	skinshdr.h				\
	skinshdr_unlit.h	    \
	rigidshdr.h				\
	envmapskinshdr.h     	\
	dualskinshdr.h

endif

####################
# gcn specific stuff
####################

ifeq ("$(RWTARGET)", "gcn")

CFLAGS +=             				\
	-DSKIN_ATOMIC_PLATFORM_DATA		\
	-I./../matfx				\
	-I./../matfx/gcn			\
	-I./../skin2				\
	-I./../skin2/gcn

endif

#####################################################################

localclean: $(LOCALCLEAN)
localdistclean: $(LOCALDISTCLEAN)
