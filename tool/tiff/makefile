# What plugin are we building ?

PLUGIN = rttiff

CFLAGS += -DTIF_PLATFORM_CONSOLE

# Who are the maintainers ?
MAINTAINERS = blakem

TIFFCSRC =					\
	$(CURDIR)/libtiff/fax3sm_winnt.c 	\
	$(CURDIR)/libtiff/tif_aux.c		\
	$(CURDIR)/libtiff/tif_close.c		\
	$(CURDIR)/libtiff/tif_codec.c		\
	$(CURDIR)/libtiff/tif_compress.c	\
	$(CURDIR)/libtiff/tif_dir.c		\
	$(CURDIR)/libtiff/tif_dirinfo.c		\
	$(CURDIR)/libtiff/tif_dirread.c		\
	$(CURDIR)/libtiff/tif_dirwrite.c	\
	$(CURDIR)/libtiff/tif_dumpmode.c	\
	$(CURDIR)/libtiff/tif_error.c		\
	$(CURDIR)/libtiff/tif_fax3.c		\
	$(CURDIR)/libtiff/tif_flush.c		\
	$(CURDIR)/libtiff/tif_getimage.c	\
	$(CURDIR)/libtiff/tif_jpeg.c		\
	$(CURDIR)/libtiff/tif_luv.c		\
	$(CURDIR)/libtiff/tif_lzw.c		\
	$(CURDIR)/libtiff/tif_next.c		\
	$(CURDIR)/libtiff/tif_open.c		\
	$(CURDIR)/libtiff/tif_packbits.c	\
	$(CURDIR)/libtiff/tif_pixarlog.c	\
	$(CURDIR)/libtiff/tif_predict.c		\
	$(CURDIR)/libtiff/tif_print.c		\
	$(CURDIR)/libtiff/tif_read.c		\
	$(CURDIR)/libtiff/tif_rw.c		\
	$(CURDIR)/libtiff/tif_strip.c		\
	$(CURDIR)/libtiff/tif_swab.c		\
	$(CURDIR)/libtiff/tif_thunder.c		\
	$(CURDIR)/libtiff/tif_tile.c		\
	$(CURDIR)/libtiff/tif_version.c		\
	$(CURDIR)/libtiff/tif_warning.c		\
	$(CURDIR)/libtiff/tif_write.c		\
	$(CURDIR)/libtiff/tif_zip.c

CSRC = $(TIFFCSRC) 

ifndef BASEDIR
BASEDIR = $(CURDIR)/../..
endif
include $(BASEDIR)/makeincl/maketool

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/libtiff/%.c
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/libtiff/%.cpp
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

# VPATH  += $(CURDIR)/libtiff
CFLAGS += -I$(CURDIR)/libtiff

ifeq ("$(RWOS)", "xbox")
CFLAGS += -DBSDTYPES
endif

ifeq ("$(RWOS)", "gcn")
ifeq ("$(RWCOMPILER)", "cwdfin")
CFLAGS += -DBSDTYPES
endif
endif

ifeq ("$(RWOS)", "sky")
#libtiff seems to default to big endian, but PS2 is little endian!
#(this flag is internal to libtiff, see tiffconf.h)
CFLAGS += -DHOST_BIGENDIAN=0
CFLAGS += -D_WINSOCK_H
endif

EXCLUDED = $(TIFFCSRC)
CVERIFY=$(filter-out $(EXCLUDED), $(CSRC))
DOXYCSRC=$(filter-out $(EXCLUDED), $(CSRC))
DOXYSRC = $(DOXYCSRC) $(subst .c,.h,$(DOXYCSRC))
