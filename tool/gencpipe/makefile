# What plugin are we building ?
PLUGIN    = rtgncpip

# Who are the maintainers ?
MAINTAINERS = blakem

# Additional objects
CSRC =								\
        $(CURDIR)/rtgncpip.c					\
        $(CURDIR)/p2clpcom.c					\
        $(CURDIR)/p2stdclsw.c					\
	$(CURDIR)/nodeClipLine.c				\
	$(CURDIR)/nodeClipTriangle.c				\
	$(CURDIR)/nodeCullTriangle.c				\
	$(CURDIR)/nodeImmInstance.c				\
	$(CURDIR)/nodeImmMangleLineIndices.c			\
	$(CURDIR)/nodeImmMangleTriangleIndices.c		\
	$(CURDIR)/nodeImmRenderSetup.c				\
	$(CURDIR)/nodeImmStash.c				\
	$(CURDIR)/nodeClone.c					\
	$(CURDIR)/nodeScatter.c					\
	$(CURDIR)/nodeSubmitLine.c				\
	$(CURDIR)/nodeSubmitTriangle.c				\
	$(CURDIR)/nodeTransform.c				\
	$(CURDIR)/nodeUVInterp.c				\
	$(CURDIR)/nodeRGBAInterp.c				\
	$(CURDIR)/nodeAtomicEnumerateLights.c			\
	$(CURDIR)/nodeAtomicInstance.c				\
	$(CURDIR)/nodeFastPathSplitter.c			\
	$(CURDIR)/nodeLight.c					\
	$(CURDIR)/nodeMaterialScatter.c				\
	$(CURDIR)/nodePostLight.c				\
	$(CURDIR)/nodePreLight.c				\
	$(CURDIR)/nodeWorldSectorEnumerateLights.c		\
	$(CURDIR)/nodeWorldSectorInstance.c
	

######################################################################

ifndef BASEDIR
BASEDIR = $(CURDIR)/../..
endif
include $(BASEDIR)/makeincl/makeplug

######################################################################

