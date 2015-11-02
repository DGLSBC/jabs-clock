
APP_NAME         := jabs-clock

APP_C_SRCS       := $(wildcard *.c)
APP_CPP_SRCS     := $(wildcard *.cpp)
APP_SRCS         := $(APP_C_SRCS) $(APP_CPP_SRCS)

APP_C_OBJS       := ${APP_C_SRCS:.c=.o}
APP_CPP_OBJS     := ${APP_CPP_SRCS:.cpp=.o}
APP_OBJS         := $(APP_C_OBJS) $(APP_CPP_OBJS)

#APP_PKG_LIBS      = gsl libarchive libcanberra libcanberra-gtk libglade-2.0 librsvg-2.0 libwnck-1.0 x11
#APP_PKG_LIBS      = gsl libarchive libcanberra libcanberra-gtk libglade-2.0 librsvg-2.0 x11
APP_PKG_LIBS      = gsl libglade-2.0 librsvg-2.0 x11
APP_PKG_CFG_CMP  := $(shell pkg-config --cflags $(APP_PKG_LIBS))
APP_PKG_CFG_LNK  := $(shell pkg-config --libs   $(APP_PKG_LIBS))

APP_INCLUDE_DIRS :=
APP_LIBRARY_DIRS :=
APP_LIBRARIES    :=

#CC        =  g++
#CPPFLAGS += -m32 -O2 -Wall -Wextra -pedantic -std=c11 -fno-strict-aliasing -fmessage-length=0 -D_FORTIFY_SOURCE=2
#CPPFLAGS += -O2 -D_FORTIFY_SOURCE=2 -ffunction-sections -fdata-sections -fmessage-length=0 -fno-strict-aliasing -pedantic -Wno-long-long -Wno-unused-result

CPPFLAGS += -std=c++11 -Os    -march=native -D_FORTIFY_SOURCE=2 -fdata-sections -ffast-math -ffunction-sections -fmessage-length=0 -fno-strict-aliasing -pedantic -Wno-long-long -Wno-unused-result -Wno-variadic-macros
#CPPFLAGS += -std=c++11 -O4    -march=native -D_FORTIFY_SOURCE=2 -fdata-sections -ffast-math -ffunction-sections -fmessage-length=0 -fno-strict-aliasing -pedantic -Wno-long-long -Wno-unused-result -Winline -Wno-variadic-macros
#CPPFLAGS += -std=c++11 -g -O0 -march=native -D_FORTIFY_SOURCE=2 -fdata-sections -ffast-math -ffunction-sections -fmessage-length=0 -fno-strict-aliasing -pedantic -Wno-long-long -Wno-unused-result -Winline -Wno-variadic-macros
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

