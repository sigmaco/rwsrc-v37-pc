# What plugin are we building ?

PLUGIN = rtpng

# Who are the maintainers ?
MAINTAINERS = robertw

C_INC += -DPNG_USER_MEM_SUPPORTED -DPNG_NO_CONSOLE_IO
C_INC += -DPNG_NO_PROGRESSIVE_READ -DPNG_NO_STDIO -DPNG_NO_WRITE_tIME


# To accomodate CW/PC builds
# C_INC += -DPNG_SETJMP_NOT_SUPPORTED

# This is a bit hacky, but stops the silly inclusion of
# sys/types.h in pngconf.h
C_INC += -DRISCOS


ZLIBCSRC =					\
	$(CURDIR)/zlib113/adler32.c			\
	$(CURDIR)/zlib113/compress.c			\
	$(CURDIR)/zlib113/gzio.c			\
	$(CURDIR)/zlib113/uncompr.c			\
	$(CURDIR)/zlib113/deflate.c			\
	$(CURDIR)/zlib113/trees.c			\
	$(CURDIR)/zlib113/zutil.c			\
	$(CURDIR)/zlib113/inflate.c			\
	$(CURDIR)/zlib113/infblock.c			\
	$(CURDIR)/zlib113/inftrees.c			\
	$(CURDIR)/zlib113/infcodes.c			\
	$(CURDIR)/zlib113/infutil.c			\
	$(CURDIR)/zlib113/inffast.c			\
	$(CURDIR)/zlib113/crc32.c

PNGLIBCSRC =					\
	$(CURDIR)/lpng1012/png.c			\
	$(CURDIR)/lpng1012/pngerror.c			\
	$(CURDIR)/lpng1012/pnggccrd.c			\
	$(CURDIR)/lpng1012/pngget.c			\
	$(CURDIR)/lpng1012/pngmem.c			\
	$(CURDIR)/lpng1012/pngpread.c			\
	$(CURDIR)/lpng1012/pngread.c			\
	$(CURDIR)/lpng1012/pngrio.c			\
	$(CURDIR)/lpng1012/pngrtran.c			\
	$(CURDIR)/lpng1012/pngrutil.c			\
	$(CURDIR)/lpng1012/pngset.c			\
	$(CURDIR)/lpng1012/pngtrans.c			\
	$(CURDIR)/lpng1012/pngvcrd.c			\
	$(CURDIR)/lpng1012/pngwio.c			\
	$(CURDIR)/lpng1012/pngwrite.c			\
	$(CURDIR)/lpng1012/pngwtran.c			\
	$(CURDIR)/lpng1012/pngwutil.c

#	$(CURDIR)/lpng1012/pngtest.c




CSRC = $(ZLIBCSRC) $(PNGLIBCSRC)

OPTTARGET=recurse

ifndef BASEDIR
BASEDIR = $(CURDIR)/../..
endif
include $(BASEDIR)/makeincl/maketool

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/lpng1012/%.c
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/lpng1012/%.cpp
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/zlib113/%.c
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

$(CURDIR)/$(OBJDIR)/%.$(O) : $(CURDIR)/zlib113/%.cpp
	$(CC) $(CFLAGS) $(CODEGENCFLAGS) \
	$(COUT)$(subst /,$(PATHSEP),$@) $<

# VPATH  += $(CURDIR)/zlib113 $(CURDIR)/lpng1012
CFLAGS +=  -I$(CURDIR)/zlib113 -I$(CURDIR)/lpng1012

ifeq ("$(RWCOMPILER)", "cwsky")
# Workaround for cwsky compatibility with
# /usr/local/sce/ee/gcc/ee/include/stdio.h
CFLAGS += -D__STRICT_ANSI__
endif

ifeq ("$(RWTARGET)", "gcn")
C_INC += -DPNG_SETJMP_NOT_SUPPORTED
endif

recurse:
# For a PC release, we need to build both a regular and a codewarrior version of this lib
ifeq ("$(RWRELEASE)", "win")
ifneq ("$(RWCOMPILER)", "cwpc")
	$(MAKE) RWOPTIONS=$(RWOPTIONS) clean
	$(MAKE) RWOPTIONS=$(RWOPTIONS) RWCOMPILER=cwpc OPTEXT=cw
	$(MAKE) RWOPTIONS=$(RWOPTIONS) RWCOMPILER=cwpc OPTEXT=cw clean
endif
endif

EXCLUDED = $(ZLIBCSRC) $(PNGLIBCSRC)
CVERIFY=$(filter-out $(EXCLUDED), $(CSRC))
DOXYCSRC=$(filter-out $(EXCLUDED), $(CSRC))
DOXYSRC = $(DOXYCSRC) $(subst .c,.h,$(DOXYCSRC))

