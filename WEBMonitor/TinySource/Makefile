COMP_NAME = TinySource

all: $(COMP_NAME)Comp

SRCS += $(COMP_NAME).cpp
SRCS += $(COMP_NAME)Comp.cpp

LDFLAGS  += -L/usr/lib/daqmw -L/usr/lib64/daqmw
LDLIBS += -lSock
CPPFLAGS += -I$(shell root-config --incdir) -I../../include -std=c++11

# sample install target
#
# MODE = 0755
# BINDIR = /tmp/mybinary
#
# install: $(COMP_NAME)Comp
#	mkdir -p $(BINDIR)
#	install -m $(MODE) $(COMP_NAME)Comp $(BINDIR)

include /usr/share/daqmw/mk/comp.mk
