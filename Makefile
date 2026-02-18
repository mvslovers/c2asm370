# 2023-12-29: This Makefile creates the "c2asm370" executable on and for Linux.
#             The resulting "c2asm370" is a 32 bit executable program that *should*
#             run on 32 or 64 bit Linux machines.
#
#             When executing "c2asm370" you must use the '-S' option to create an
#             assembler source file: ./c2asm370 -I pdpclib -S test.c
#
#             The resulting assembler source file will have a '.s' extension.
#
#             To create an object file you have to upload the assembler source
#             to whatever platform you have that has an IBM compatible assembler
#             program.
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# note: In the CFLAGS below we're limited to '-O1' optimization due to the fact
#       that the gcc source (c2asm370 source) is pretty sloppy with memory allocations
#       that never get freed at program termination. I would guess there may be
#       some other issues as well so we'll use either '-g' or '-O1' when building
#       c2asm370.
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# common definitions
DESTDIR  = $(PREFIX)/usr/local/bin

INCLUDES = -I include -I gcc -I i370
PROGRAM_NAME := c2asm370
CC = gcc

# Platform detection
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

ifeq ($(UNAME_S),Darwin)
  # macOS (Apple Silicon or Intel)
  HOST_DEFINE = -DHOST_MACOS
  ARCH_FLAGS =
  LINK_ARCH_FLAGS =
  LINK_LIBS =
  EXTRA_CFLAGS = -fcommon -fgnu89-inline
else
  # Linux (default, 32-bit build)
  HOST_DEFINE = -DHOST_LINUX
  ARCH_FLAGS = -m32 -fno-pie
  LINK_ARCH_FLAGS = -m32
  LINK_LIBS = -lgcc
  EXTRA_CFLAGS =
endif

DEFINES = -DIN_GCC -DHAVE_CONFIG_H -DPUREISO -DTARGET_MVS $(HOST_DEFINE) -DMVSGCC_CROSS
CFLAGS = -std=c99 -fno-stack-protector -fno-builtin -fexceptions $(ARCH_FLAGS) -O1 $(EXTRA_CFLAGS) $(INCLUDES) $(DEFINES)

# sources for I370 (370 assembler source creation)
I370_SRCS= \
  i370/i370.c \
  i370/i370-c.c \
  i370/final.c \
  i370/print-rtl.c \
  i370/toplev.c \
  i370/varasm.c \
  i370/version.c

# sources for the gcc compiler
GCC_SRCS= \
  gcc/alias.c \
  gcc/attribs.c \
  gcc/bb-reorder.c \
  gcc/bitmap.c \
  gcc/builtins.c \
  gcc/c-aux-info.c \
  gcc/c-common.c \
  gcc/c-convert.c \
  gcc/c-decl.c \
  gcc/c-errors.c \
  gcc/c-format.c \
  gcc/c-lang.c \
  gcc/c-lex.c \
  gcc/c-objc-common.c \
  gcc/c-parse.c \
  gcc/c-pragma.c \
  gcc/c-semantics.c \
  gcc/c-typeck.c \
  gcc/caller-save.c \
  gcc/calls.c \
  gcc/cfg.c \
  gcc/cfganal.c \
  gcc/cfgbuild.c \
  gcc/cfgcleanup.c \
  gcc/cfglayout.c \
  gcc/cfgloop.c \
  gcc/cfgrtl.c \
  gcc/combine.c \
  gcc/conflict.c \
  gcc/convert.c \
  gcc/cppdefault.c \
  gcc/cpperror.c \
  gcc/cppexp.c \
  gcc/cppfiles.c \
  gcc/cpphash.c \
  gcc/cppinit.c \
  gcc/cpplex.c \
  gcc/cpplib.c \
  gcc/cppmacro.c \
  gcc/cppmain.c \
  gcc/cppspec.c \
  gcc/cse.c \
  gcc/cselib.c \
  gcc/debug.c \
  gcc/dependence.c \
  gcc/df.c \
  gcc/diagnostic.c \
  gcc/dominance.c \
  gcc/dwarf2asm.c \
  gcc/dwarf2out.c \
  gcc/emit-rtl.c \
  gcc/except.c \
  gcc/explow.c \
  gcc/expmed.c \
  gcc/expr.c \
  gcc/flow.c \
  gcc/fold-const.c \
  gcc/function.c \
  gcc/gcc.c \
  gcc/gccspec.c \
  gcc/gcse.c \
  gcc/genrtl.c \
  gcc/ggc-common.c \
  gcc/ggc-page.c \
  gcc/global.c \
  gcc/graph.c \
  gcc/hash.c \
  gcc/hashtable.c \
  gcc/hooks.c \
  gcc/ifcvt.c \
  gcc/insn-attrtab.c \
  gcc/insn-emit.c \
  gcc/insn-extract.c \
  gcc/insn-opinit.c \
  gcc/insn-output.c \
  gcc/insn-recog.c \
  gcc/integrate.c \
  gcc/jump.c \
  gcc/langhooks.c \
  gcc/lcm.c \
  gcc/line-map.c \
  gcc/lists.c \
  gcc/local-alloc.c \
  gcc/loop.c \
  gcc/mkdeps.c \
  gcc/optabs.c \
  gcc/params.c \
  gcc/predict.c \
  gcc/prefix.c \
  gcc/print-tree.c \
  gcc/profile.c \
  gcc/real.c \
  gcc/recog.c \
  gcc/regclass.c \
  gcc/regmove.c \
  gcc/regrename.c \
  gcc/reload.c \
  gcc/reload1.c \
  gcc/resource.c \
  gcc/rtl-error.c \
  gcc/rtl.c \
  gcc/rtlanal.c \
  gcc/sbitmap.c \
  gcc/sibcall.c \
  gcc/simplify-rtx.c \
  gcc/ssa-ccp.c \
  gcc/ssa-dce.c \
  gcc/ssa.c \
  gcc/stmt.c \
  gcc/stor-layout.c \
  gcc/stringpool.c \
  gcc/timevar.c \
  gcc/tree-dump.c \
  gcc/tree-inline.c \
  gcc/tree.c \
  gcc/unroll.c \
  gcc/varray.c

# sources for libiberty
LIB_SRCS = \
  libiberty/xmalloc.c \
  libiberty/xstrerror.c \
  libiberty/xstrdup.c \
  libiberty/xexit.c \
  libiberty/concat.c \
  libiberty/alloca.c \
  libiberty/hex.c \
  libiberty/hashtab.c \
  libiberty/fibheap.c \
  libiberty/lbasename.c \
  libiberty/make-temp-file.c \
  libiberty/safe-ctype.c \
  libiberty/splay-tree.c \
  libiberty/partition.c \
  libiberty/obstack.c

# unused sources
XXX_SRCS = \
  libiberty/obstack.c \
  libiberty/asprintf.c \
  libiberty/vasprintf.c \
  libiberty/getpagesize.c \
  libiberty/strsignal.c

# all sources for the compiler
SRC_FILES = $(I370_SRCS) $(GCC_SRCS) $(LIB_SRCS)
OBJ_FILES := $(foreach filename, $(SRC_FILES), $(filename:.c=.o))

# dummy for make all
all: $(PROGRAM_NAME)
.PHONY: all

# build executable file
$(PROGRAM_NAME): $(OBJ_FILES)
	@echo "Build executable"
	$(CC) -o $@ $^ $(LINK_ARCH_FLAGS) $(LINK_LIBS)

# compile all object file
%.o: %.c
	@echo "Compile $(notdir $<)"
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f $(OBJ_FILES) $(PROGRAM_NAME)

install:  $(PROGRAM_NAME)
	cp $(PROGRAM_NAME) $(DESTDIR)

# dummy rule to prevent running yacc
gcc/c-parse.c:
	touch $@
