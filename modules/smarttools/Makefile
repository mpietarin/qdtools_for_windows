DEFINES = -DUNIX -D_REENTRANT

SUBNAME = smarttools
LIB = smartmet-$(SUBNAME)
SPEC = smartmet-library-$(SUBNAME)
INCDIR = smartmet/$(SUBNAME)

# Installation directories

include common.mk

# clang++ and g++ do not accept the std::async code in NFmiLocalAreaMinMaxMask, hence the DEBUG_LOCAL_EXTREMES flag
DEFINES = -DUNIX -D_REENTRANT -DBOOST -DDEBUG_LOCAL_EXTREMES

# Boost 1.69

ifeq ($(USE_CLANG), yes)

 FLAGS = \
	-std=$(CXX_STD) -fPIC -MD -fno-omit-frame-pointer \
	-Wall \
        -Wextra \
	-Wno-c++98-compat \
	-Wno-float-equal \
	-Wno-padded \
	-Wno-missing-prototypes

 INCLUDES += -isystem $(includedir)/smartmet $(SYSTEM_INCLUDES)

else

 FLAGS = -std=$(CXX_STD) -fPIC -MD -fno-omit-frame-pointer -Wall -W -Wno-unused-parameter -fdiagnostics-color=$(GCC_DIAG_COLOR)

 FLAGS_DEBUG = \
	-Wcast-align \
	-Winline \
	-Wno-multichar \
	-Wno-pmf-conversions \
	-Woverloaded-virtual  \
	-Wpointer-arith \
	-Wcast-qual \
	-Wwrite-strings \
	-Wsign-promo \
	-Wno-inline

 FLAGS_RELEASE = -Wuninitialized

 INCLUDES += \
	-I$(includedir)/smartmet

endif

ifeq ($(TSAN), yes)
  FLAGS += -fsanitize=thread
endif
ifeq ($(ASAN), yes)
  FLAGS += -fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=undefined -fsanitize-address-use-after-scope
endif

# Compile options in detault, debug and profile modes

CFLAGS         = $(DEFINES) $(FLAGS) $(FLAGS_RELEASE) -DNDEBUG -O2 -g
CFLAGS_DEBUG   = $(DEFINES) $(FLAGS) $(FLAGS_DEBUG)   -Werror  -Og -g
CFLAGS_PROFILE = $(DEFINES) $(FLAGS) $(FLAGS_PROFILE) -DNDEBUG -O2 -g -pg

CFLAGS0        = $(DEFINES) $(FLAGS) $(FLAGS_RELEASE) -DNDEBUG -O0 -g

LIBS += -L$(libdir) \
	-lfmt \
	-lboost_regex \
	-lboost_date_time \
	-lboost_filesystem \
	-lboost_iostreams \
	-lboost_thread

# What to install

LIBFILE = libsmartmet-$(SUBNAME).so

# How to install

INSTALL_PROG = install -p -m 775
INSTALL_DATA = install -p -m 664

ARFLAGS = -r

# Compile option overrides

ifneq (,$(findstring debug,$(MAKECMDGOALS)))
  CFLAGS = $(CFLAGS_DEBUG)
  CFLAGS0 = $(CFLAGS_DEBUG)
endif

ifneq (,$(findstring profile,$(MAKECMDGOALS)))
  CFLAGS = $(CFLAGS_PROFILE)
  CFLAGS0 = $(CFLAGS_PROFILE)
endif

# Compilation directories

vpath %.cpp $(SUBNAME)
vpath %.h $(SUBNAME)

# The files to be compiled

SRCS = $(wildcard $(SUBNAME)/*.cpp)
HDRS = $(wildcard $(SUBNAME)/*.h)
OBJS = $(patsubst %.cpp, obj/%.o, $(notdir $(SRCS)))

INCLUDES := -Iinclude $(INCLUDES)

.PHONY: test rpm

# The rules

all: objdir $(LIBFILE)
debug: all
release: all
profile: all

$(LIBFILE): $(OBJS)
	$(CC) $(LDFLAGS) -shared -rdynamic -o $(LIBFILE) $(OBJS) $(LIBS)

clean:
	rm -f $(LIBFILE) *~ $(SUBNAME)/*~
	rm -rf $(objdir)
	rm -f test/*Test

format:
	clang-format -i -style=file $(SUBNAME)/*.h $(SUBNAME)/*.cpp test/*.cpp

install:
	@mkdir -p $(includedir)/$(INCDIR)
	@list='$(HDRS)'; \
	for hdr in $$list; do \
	  HDR=$$(basename $$hdr); \
	  echo $(INSTALL_DATA) $$hdr $(includedir)/$(INCDIR)/$$HDR; \
	  $(INSTALL_DATA) $$hdr $(includedir)/$(INCDIR)/$$HDR; \
	done
	@mkdir -p $(libdir)
	echo $(INSTALL_PROG) $(LIBFILE) $(libdir)/$(LIBFILE)
	$(INSTALL_PROG) $(LIBFILE) $(libdir)/$(LIBFILE)

objdir:
	@mkdir -p $(objdir)

rpm: clean $(SPEC).spec
	rm -f $(SPEC).tar.gz # Clean a possible leftover from previous attempt
	tar -czvf $(SPEC).tar.gz --exclude test --exclude-vcs --transform "s,^,$(SPEC)/," *
	rpmbuild -ta $(SPEC).tar.gz
	rm -f $(SPEC).tar.gz

.SUFFIXES: $(SUFFIXES) .cpp

modernize:
	for F in newbase/*.cpp; do echo $$F; clang-tidy $$F -fix -checks=-*,modernize-* -- $(CFLAGS) $(DEFINES) $(INCLUDES); done

obj/%.o: %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -c -o $@ $<

obj/NFmiEnumConverter.o: NFmiEnumConverter.cpp
	$(CXX) $(CFLAGS0) $(INCLUDES) -c -o $@ $<

ifneq ($(wildcard obj/*.d),)
-include $(wildcard obj/*.d)
endif
