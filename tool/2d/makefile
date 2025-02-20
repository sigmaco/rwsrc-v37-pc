# What plugin are we building ?

PLUGIN = rt2d
# Who are the maintainers ?
MAINTAINERS = ads colinh gregb

CSRC =						\
	$(CURDIR)/brush.c			\
	$(CURDIR)/fill.c			\
	$(CURDIR)/font.c			\
	$(CURDIR)/gstate.c			\
	$(CURDIR)/path.c			\
	$(CURDIR)/stroke.c			\
	$(CURDIR)/2dstr.c           		\
	$(CURDIR)/object.c          		\
	$(CURDIR)/pickreg.c         		\
	$(CURDIR)/scene.c           		\
	$(CURDIR)/shape.c           		\
	$(CURDIR)/tri.c				\

sky2CSRC =					\
	$(CURDIR)/sky2/ps2pipes.c

ifndef BASEDIR
BASEDIR = $(CURDIR)/../..
endif
include $(BASEDIR)/makeincl/maketool


ifeq ("$(RWTARGET)", "sky2")
CFLAGS += -I./sky2
endif
