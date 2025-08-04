RSRCDIR=resources
SRCDIR=src
CORESRCDIR=../Atari2600Core
CPUSRCDIR=../Cpu6502Core
BUILDDIR=build

CXXFLAGS = -g -Wall -Werror -Wno-sign-compare
CXXFLAGS += -I$(SRCDIR) -I$(CPUSRCDIR) -I$(CORESRCDIR)
CXXFLAGS += $(shell pkg-config --cflags gtkmm-3.0)

LDFLAGS = -export-dynamic
LDLIBS = $(shell pkg-config --libs gtkmm-3.0)

RSRCSRCS=	$(RSRCDIR)/spaceinvaders.c		\
		$(RSRCDIR)/colortab.c

ATARIGTKSRCS =	$(SRCDIR)/Atari2600GtkApp.cpp		\
		$(SRCDIR)/Atari2600GtkAppWin.cpp	\
		$(SRCDIR)/Atari2600GtkDisp.cpp		\
		$(SRCDIR)/Atari2600GtkInput.cpp

CPUSRCS=	$(CPUSRCDIR)/Cpu6502.cpp

DBGSRCS=	$(CPUSRCDIR)/Cpu6502GtkDebug.cpp

ATARICORESRCS=	$(CORESRCDIR)/Atari2600.cpp		\
		$(CORESRCDIR)/Atari2600Hw.cpp		\
		$(CORESRCDIR)/Atari2600TIA.cpp		\
		$(CORESRCDIR)/Mos6532Riot.cpp

GRESOURCE=	$(BUILDDIR)/atarigtk.gresource.cpp
GRCSOBJ= $(patsubst $(BUILDDIR)/%.cpp,$(BUILDDIR)/%.o,$(GRESOURCE))

RSRCOBJS= $(patsubst $(RSRCDIR)/%.c,$(BUILDDIR)/%.o,$(RSRCSRCS))
ATARIGTKOBJS= $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(ATARIGTKSRCS))
CPUOBJS= $(patsubst $(CPUSRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(CPUSRCS))
DBGOBJS= $(patsubst $(CPUSRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(DBGSRCS))
ATARICOREOBJS= $(patsubst $(CORESRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(ATARICORESRCS))
OBJS=$(RSRCOBJS) $(ATARIGTKOBJS) $(CPUOBJS) $(DBGOBJS) $(ATARICOREOBJS) \
	$(GRCSOBJ)

TARGETS=$(BUILDDIR)/atarigtk

.PHONY: default clean builddir

default: builddir $(TARGETS)

clean:
	$(RM) $(OBJS) $(TARGETS) $(GRESOURCE)

builddir:
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/atarigtk: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(GRESOURCE): $(RSRCDIR)/atari2600.glade $(RSRCDIR)/atarilogo.jpg \
	$(RSRCDIR)/atarigtk.resources.xml
	glib-compile-resources --target=$(GRESOURCE) --generate-source \
		--sourcedir=$(RSRCDIR) $(RSRCDIR)/atarigtk.resources.xml

$(BUILDDIR)/%.o : $(RSRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o : $(SRCDIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(BUILDDIR)/%.o : $(CPUSRCDIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(BUILDDIR)/%.o : $(CORESRCDIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@
