
POWERMAKEFILES = 						\
	makefile.skin						\
	makefile.skinmatfx					\
	makefile.skintoon                   			\
    	makefile.skinmatfxtoon

DOCMAKEFILE =							\
	makefile.skindoc

# Remove shell usage
#all clean cleanheaders header equate distclean defgroup verify longline tabs :
#	for powermake in $(POWERMAKEFILES); 			\
#	do							\
#		echo $(MAKE);					\
#		$(MAKE) -f $$powermake $@ 			\
#		RWOPTIONS=$(RWOPTIONS) ; 			\
#	done

SUBMAKEALL = $(patsubst %, %-all,$(POWERMAKEFILES))
.PHONY: all $(SUBMAKEALL)
all: $(SUBMAKEALL)
$(SUBMAKEALL):
	$(MAKE) -f $(subst -all,,$@) all RWOPTIONS=$(RWOPTIONS)

SUBMAKECLEAN = $(patsubst %, %-clean,$(POWERMAKEFILES))
.PHONY: clean $(SUBMAKECLEAN)
clean: $(SUBMAKECLEAN)
$(SUBMAKECLEAN):
	$(MAKE) -f $(subst -clean,,$@) clean RWOPTIONS=$(RWOPTIONS)

SUBMAKECLEANHEADERS = $(patsubst %, %-cleanheaders,$(POWERMAKEFILES))
.PHONY: cleanheaders $(SUBMAKECLEANHEADERS)
cleanheaders: $(SUBMAKECLEANHEADERS)
$(SUBMAKECLEANHEADERS):
	$(MAKE) -f $(subst -cleanheaders,,$@) cleanheaders \
		RWOPTIONS=$(RWOPTIONS)

SUBMAKEHEADER = $(patsubst %, %-header,$(POWERMAKEFILES))
.PHONY: header $(SUBMAKEHEADER)
header: $(SUBMAKEHEADER)
$(SUBMAKEHEADER):
	$(MAKE) -f $(subst -header,,$@) header RWOPTIONS=$(RWOPTIONS)

SUBMAKEEQUATE = $(patsubst %, %-equate,$(POWERMAKEFILES))
.PHONY: equate $(SUBMAKEEQUATE)
equate: $(SUBMAKEEQUATE)
$(SUBMAKEEQUATE):
	$(MAKE) -f $(subst -equate,,$@) equate RWOPTIONS=$(RWOPTIONS)

SUBMAKEDISTCLEAN = $(patsubst %, %-distclean,$(POWERMAKEFILES))
.PHONY: distclean $(SUBMAKEDISTCLEAN)
distclean: $(SUBMAKEDISTCLEAN)
$(SUBMAKEDISTCLEAN):
	$(MAKE) -f $(subst -distclean,,$@) distclean RWOPTIONS=$(RWOPTIONS)

SUBMAKEDEFGROUP = $(patsubst %, %-defgroup,$(POWERMAKEFILES))
.PHONY: defgroup $(SUBMAKEDEFGROUP)
defgroup: $(SUBMAKEDEFGROUP)
$(SUBMAKEDEFGROUP):
	$(MAKE) -f $(subst -defgroup,,$@) defgroup RWOPTIONS=$(RWOPTIONS)

SUBMAKEVERIFY = $(patsubst %, %-verify,$(POWERMAKEFILES))
.PHONY: verify $(SUBMAKEVERIFY)
verify: $(SUBMAKEVERIFY)
$(SUBMAKEVERIFY):
	$(MAKE) -f $(subst -verify,,$@) verify RWOPTIONS=$(RWOPTIONS)

SUBMAKELONGLINE = $(patsubst %, %-longline,$(POWERMAKEFILES))
.PHONY: longline $(SUBMAKELONGLINE)
longline: $(SUBMAKELONGLINE)
$(SUBMAKELONGLINE):
	$(MAKE) -f $(subst -longline,,$@) longline RWOPTIONS=$(RWOPTIONS)

SUBMAKETABS = $(patsubst %, %-tabs,$(POWERMAKEFILES))
.PHONY: tabs $(SUBMAKETABS)
tabs: $(SUBMAKETABS)
$(SUBMAKETABS):
	$(MAKE) -f $(subst -tabs,,$@) tabs RWOPTIONS=$(RWOPTIONS)

# Remove shell usage
#doxy doc :
#	for docsmake in $(DOCMAKEFILE);	 			\
#	do 							\
#		$(MAKE) -f $$docsmake $@ 			\
#		RWOPTIONS=$(RWOPTIONS) ; 			\
#	done

SUBMAKEDOXY = $(patsubst %, %-doxy,$(DOCMAKEFILE))
.PHONY: doxy $(SUBMAKEDOXY)
doxy: $(SUBMAKEDOXY)
$(SUBMAKEDOXY):
	$(MAKE) -f $(subst -doxy,,$@) doxy RWOPTIONS=$(RWOPTIONS)

SUBMAKEDOC = $(patsubst %, %-doc,$(DOCMAKEFILE))
.PHONY: doc $(SUBMAKEDOC)
doc: $(SUBMAKEDOC)
$(SUBMAKEDOC):
	$(MAKE) -f $(subst -doc,,$@) doc RWOPTIONS=$(RWOPTIONS)

