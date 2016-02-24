
APP_NAME         := jabs-clock

APP_C_SRCS       := $(wildcard *.c)
APP_CPP_SRCS     := $(wildcard *.cpp)
APP_SRCS         := $(APP_C_SRCS) $(APP_CPP_SRCS)

APP_C_OBJS       := ${APP_C_SRCS:.c=.o}
APP_CPP_OBJS     := ${APP_CPP_SRCS:.cpp=.o}
APP_OBJS         := $(APP_C_OBJS) $(APP_CPP_OBJS)

# one of these specify the build type along with the associated platform.h define
APP_PKG_DEPS      = gtk+-2.0
#APP_PKG_DEPS      = gtk+-3.0
#APP_PKG_DEPS      = QtCore QtGui

# the following two are only used in a gtk build
APP_PKG_DEPS     += libcanberra-gtk
APP_PKG_DEPS     += libglade-2.0

# these are all currently used in all build types
APP_PKG_DEPS     += dbus-glib-1
APP_PKG_DEPS     += gmodule-2.0
APP_PKG_DEPS     += librsvg-2.0
APP_PKG_DEPS     += libcanberra
APP_PKG_DEPS     += libwnck-1.0
APP_PKG_DEPS     += libarchive
APP_PKG_DEPS     += freetype2
APP_PKG_DEPS     += glib-2.0
APP_PKG_DEPS     += geoclue
APP_PKG_DEPS     += cairo
APP_PKG_DEPS     += gsl
APP_PKG_DEPS     += x11

APP_PKG_CFG_CMP  := $(shell pkg-config --cflags $(APP_PKG_DEPS))
APP_PKG_CFG_LNK  := $(shell pkg-config --libs   $(APP_PKG_DEPS))

APP_INCLUDE_DIRS :=
APP_LIBRARY_DIRS :=
APP_LIBRARIES    :=

#CPPFLAGS += -O0 -g
CPPFLAGS += -Os
#CPPFLAGS += -O1
#CPPFLAGS += -O2
#CPPFLAGS += -O3
#CPPFLAGS += -std=c++11
CPPFLAGS += -march=native -D_FORTIFY_SOURCE=2 -DGSEAL_ENABLE -pedantic
#CPPFLAGS += -fdata-sections
#CPPFLAGS += -ffast-math
#CPPFLAGS += -ffunction-sections
#CPPFLAGS += -fmerge-all-constants
#CPPFLAGS += -fmessage-length=0
#CPPFLAGS += -fno-rtti
#CPPFLAGS += -fno-strict-aliasing
CPPFLAGS += -Wno-inline
CPPFLAGS += -Wno-long-long
CPPFLAGS += -Wno-unused-result
CPPFLAGS += -Wno-variadic-macros

CPPFLAGS += $(foreach includedir, $(APP_INCLUDE_DIRS), -I$(includedir))
CPPFLAGS += $(APP_PKG_CFG_CMP)

LDFLAGS  += $(foreach librarydir, $(APP_LIBRARY_DIRS), -L$(librarydir))
LDLIBS   += $(foreach library,    $(APP_LIBRARIES),    -l$(library))
LDLIBS   += $(APP_PKG_CFG_LNK)

all: $(APP_NAME)

$(APP_NAME): .depend $(APP_OBJS)
	$(LINK.cc) $(APP_OBJS) -o $(APP_NAME) $(LDLIBS)

depend: .depend

.depend: cmd = $(CC) $(CPPFLAGS) -MM -MF .deptmp $(var); cat .deptmp >> .depend;
.depend:
	@echo "Generating dependencies..."
	@$(foreach var, $(APP_SRCS), $(cmd))
	@- $(RM) .deptmp

-include .depend

clean:
	@- $(RM) $(APP_NAME)
	@- $(RM) $(APP_OBJS)
	@- $(RM) .dep*

distclean: clean

.PHONY: all clean distclean depend

