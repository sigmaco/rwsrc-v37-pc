# This builds the library file of the 3D graphics API
#

all: checkopt message dirsetup header equate all-recursive dllgen

# all directories are defined relative to the rwsdk BASEDIR
ifndef BASEDIR
BASEDIR = $(CURDIR)
endif

COREDIR=$(BASEDIR)/src

# Include target specific make options

include $(BASEDIR)/makeincl/makeopt
include $(BASEDIR)/makeincl/rwtarget/$(RWTARGET)/maketarg

# Define pipe type (maketarg/$(RWTARGET) had first go)
ifndef PIPETYPE
PIPETYPE = p2
endif

include $(BASEDIR)/makeincl/rwtarget/pipe/p2/maketarg

include $(BASEDIR)/makeincl/makedll

message:
	@echo Building RenderWare target - $(RWTARGET)
	@echo Using compiler - $(RWCOMPILER)
	@echo Using operating system - $(RWOS)
	@echo

PLUGINDIR = plugin
ifndef PLUGINS
PLUGINS = $(notdir $(wildcard $(PLUGINDIR)/*))
endif

TOOLKITDIR = tool
ifndef TOOLKITS
TOOLKITS = $(notdir $(wildcard $(TOOLKITDIR)/*))
endif

all-recursive clean-recursive header-recursive equate-recursive	\
distclean-recursive doxy-recursive doc-recursive defgroup-recursive	\
csrc-recursive verify-recursive longline-recursive tabs-recursive	\
cleanheaders-recursive:
	-@cd $(COREDIR); 					      \
	$(MAKE) $(subst -recursive,,$@) BASEDIR=$(BASEDIR) RWOPTIONS=$(RWOPTIONS) PIPETYPE=$(PIPETYPE)
ifeq ($(RWONLYPLCORE), 0)
	-@cd $(WORLDDIR); 					      \
	$(MAKE) $(subst -recursive,,$@) BASEDIR=$(BASEDIR) RWOPTIONS=$(RWOPTIONS) PIPETYPE=$(PIPETYPE)
	-@cd $(PLUGINDIR); 					      \
	for plugin in $(PLUGINS); do 				      \
		$(MAKE) -C $$plugin $(subst -recursive,,$@) 	      \
		BASEDIR=$(BASEDIR) RWOPTIONS=$(RWOPTIONS) PIPETYPE=$(PIPETYPE); 	      \
	done
	-@cd $(TOOLKITDIR); 					      \
	for toolkit in $(TOOLKITS); do 				      \
		$(MAKE) -C $$toolkit $(subst -recursive,,$@) 	      \
		BASEDIR=$(BASEDIR) RWOPTIONS=$(RWOPTIONS) PIPETYPE=$(PIPETYPE); 	      \
	done
else
	-@cd $(TOOLKITDIR); 					      	\
	$(MAKE) -C fsyst $(subst -recursive,,$@) 	      		\
	BASEDIR=$(BASEDIR) RWOPTIONS=$(RWOPTIONS) PIPETYPE=$(PIPETYPE);
endif

SDKDIRS =							      \
	$(SDKLIBDIR) $(SDKINCDIR)

# softras has a DLL folder too
ifeq ("$(RWTARGET)", "softras")
SDKDIRS +=							      \
	$(BASEDIR)/dll/$(RWPLATFORM)
endif

$(SDKDIRS) $(DEPDIR) $(SDKHLPDIR)/$(RWTARGET):
	$(MD) $@

dirsetup: $(SDKDIRS)

depend: $(DEPDIR)

header: header-recursive

equate: equate-recursive

doxy: header doxy-recursive

docfileclean:
	$(RM) $(SDKHLPDIR)/$(RWTARGET)/Docfile

SRCTUTORIAL = $(BASEDIR)/../examples/tutorials

DOXYIMAGE = $(BASEDIR)/doxy/images

# List of directories containing example files used by Doxygen
DOXYEXAMPLE +=							\
	$(SRCTUTORIAL)/tutorial1/src		\
	$(SRCTUTORIAL)/tutorial1/src/win	\
	$(SRCTUTORIAL)/tutorial2/src		\
	$(SRCTUTORIAL)/tutorial2/src/win	\
	$(SRCTUTORIAL)/tutorial3/src		\
	$(SRCTUTORIAL)/tutorial3/src/win	\
	$(SRCTUTORIAL)/tutorial4/src		\
	$(SRCTUTORIAL)/tutorial4/src/win	\
	$(SRCTUTORIAL)/tutorial5/src		\
	$(SRCTUTORIAL)/tutorial5/src/win	\
	$(SRCTUTORIAL)/tutorial6/src		\
	$(SRCTUTORIAL)/tutorial6/src/win	\
	$(SRCTUTORIAL)/tutorial7/src		\
	$(SRCTUTORIAL)/tutorial7/src/win

# List of directories containing image files used by Doxygen. 
# Doxygen copies files if \image used.
IMAGEPATH  =		\
	$(BASEDIR)/doxy/images				\

# Copies and pastes listed images if <img> used.
SRCIMAGE =	\
	$(DOXYIMAGE)/*.*					\

ifeq ("$(RWOS)", "win")
SRCIMAGE +=								\
	$(SRCTUTORIAL)/tutorial1/doc/*.png	\
	$(SRCTUTORIAL)/tutorial2/doc/*.png	\
	$(SRCTUTORIAL)/tutorial3/doc/*.png	\
	$(SRCTUTORIAL)/tutorial4/doc/*.png	\
	$(SRCTUTORIAL)/tutorial5/doc/*.png	
endif #("$(RWOS)", "win")

# See rwsdk\doxy\doxycfg.c for more info.
rwdocbuild:
	echo $(DOXYSRC) |						\
	$(BINDIR)/doxycfg						\
	  -b$(BASEDIR)							\
	  -d$(SDKHLPDIR)/$(RWTARGET)/			\
	  -i"$(IMAGEPATH)"						\
	  -m$(BASEDIR)/doxy/format/html					\
	  -o$(RWOS)								\
	  -s"$(DOXYEXAMPLE)" 					\
	  -t$(RWTARGET)							\
	<  $(SDKHLPDIR)/$(RWTARGET)/Docfile		\
	>  $(SDKHLPDIR)/$(RWTARGET)/Doxygen

# Builds the chm
# subtopic.sed tidies the API's index, pdf.sed enables pdf hyperlinking 
rwdoc:
	$(BINDIR)/doxygen $(SDKHLPDIR)/$(RWTARGET)/Doxygen
	-$(CP) $(SRCIMAGE) $(SDKHLPDIR)/$(RWTARGET)/html
	$(BINDIR)/UNIX2DOS $(SDKHLPDIR)/$(RWTARGET)/html/index.hhc

	-$(CP)  $(SDKHLPDIR)/$(RWTARGET)/html/index.hhk \
            $(SDKHLPDIR)/$(RWTARGET)/html/index.hhk_old
	$(SED) -f $(BINDIR)/subtopic.sed < $(SDKHLPDIR)/$(RWTARGET)/html/index.hhk_old > \
            $(SDKHLPDIR)/$(RWTARGET)/html/index.hhk
	for myhtml in $(SDKHLPDIR)/$(RWTARGET)/html/*.html; do \
	$(SED) -f $(BINDIR)/pdf.sed < $$myhtml > $(SDKHLPDIR)/$(RWTARGET)/html/tmp._html; \
	$(RM) $$myhtml; \
	$(MV) $(SDKHLPDIR)/$(RWTARGET)/html/tmp._html $$myhtml;\
	done

	@echo About to run -cmd /C $(BINDIR)/hhc autodocs/hlp/$(RWTARGET)/html/index.hhp
	-cmd /C $(BINDIR)/hhc autodocs/hlp/$(RWTARGET)/html/index.hhp

doc: header $(SDKHLPDIR)/$(RWTARGET) docfileclean doc-recursive rwdocbuild rwdoc

defgroup: defgroup-recursive

clean: checkopt clean-recursive dllclean
	$(RM) *.cmd *.err *.ilk *.lst *.obj *.pdb

cleanheaders: checkopt cleanheaders-recursive

distclean: checkopt distclean-recursive dlldistclean
	$(RM) -r $(SDKDIRS)
	$(RM) -r $(BASEDIR)/lib $(BASEDIR)/include $(BASEDIR)/dll
	$(RM) *.cmd *.err *.ilk *.lst *.obj *.pdb

verify: verify-recursive

longline: longline-recursive

tabs: tabs-recursive

maintainers-ext:
	-@cd $(PLUGINDIR); 					      \
	for plugin in $(PLUGINS); do 				      \
		$(MAKE) -C $$plugin $(subst -ext,,$@) 		      \
		BASEDIR=$(BASEDIR) RWOPTIONS=$(RWOPTIONS) PIPETYPE=$(PIPETYPE); 	      \
	done
	-@cd $(TOOLKITDIR); 					      \
	for toolkit in $(TOOLKITS); do 				      \
		$(MAKE) -C $$toolkit $(subst -ext,,$@) 		      \
		BASEDIR=$(BASEDIR) RWOPTIONS=$(RWOPTIONS) PIPETYPE=$(PIPETYPE); 	      \
	done

maintainers: maintainers-ext

csrc:	csrc-recursive
