RSRCDIR=resources
SRCDIR=src
CORESRCDIR=../AppleCore
CPUSRCDIR=../Cpu6502Core
BUILDDIR=build

CXXFLAGS = -O -Wall -Werror -Wno-sign-compare
CXXFLAGS += -I$(SRCDIR) -I$(CPUSRCDIR) -I$(CORESRCDIR)
CXXFLAGS += $(shell pkg-config --cflags gtkmm-3.0)

LDFLAGS = -export-dynamic
LDLIBS = $(shell pkg-config --libs gtkmm-3.0)

RSRCSRCS =	$(RSRCDIR)/charrom.c			\
		$(RSRCDIR)/apple2roms.c

APPLEGTKSRCS =	$(SRCDIR)/Apple2GtkApp.cpp		\
		$(SRCDIR)/Apple2GtkAppWin.cpp		\
		$(SRCDIR)/Apple2GtkDisp.cpp		\
		$(SRCDIR)/Apple2GtkInput.cpp

CPUSRCS=	$(CPUSRCDIR)/Cpu6502.cpp

DBGSRCS=	$(CPUSRCDIR)/Cpu6502GtkDebug.cpp

APPLECORESRCS=	$(CORESRCDIR)/Apple2.cpp		\
		$(CORESRCDIR)/Apple2Hw.cpp		\
		$(CORESRCDIR)/Apple2Io.cpp		\
		$(CORESRCDIR)/Apple2Disk2.cpp

GRESOURCE=	$(BUILDDIR)/apple2gtk.gresource.cpp
GRCSOBJ= $(patsubst $(BUILDDIR)/%.cpp,$(BUILDDIR)/%.o,$(GRESOURCE))

RSRCOBJS= $(patsubst $(RSRCDIR)/%.c,$(BUILDDIR)/%.o,$(RSRCSRCS))
APPLEGTKOBJS= $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(APPLEGTKSRCS))
CPUOBJS= $(patsubst $(CPUSRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(CPUSRCS))
DBGOBJS= $(patsubst $(CPUSRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(DBGSRCS))
APPLECOREOBJS= $(patsubst $(CORESRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(APPLECORESRCS))
OBJS=$(RSRCOBJS) $(APPLEGTKOBJS) $(CPUOBJS) $(DBGOBJS) $(APPLECOREOBJS) \
	$(GRCSOBJ)

TARGETS=$(BUILDDIR)/apple2gtk

.PHONY: default clean builddir

default: builddir $(TARGETS)

clean:
	$(RM) $(OBJS) $(TARGETS) $(GRESOURCE)

builddir:
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/apple2gtk: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(GRESOURCE): $(RSRCDIR)/apple2gtk.glade $(RSRCDIR)/applelogo.jpg \
	$(RSRCDIR)/apple2gtk.resources.xml
	glib-compile-resources --target=$(GRESOURCE) --generate-source \
		--sourcedir=$(RSRCDIR) $(RSRCDIR)/apple2gtk.resources.xml

$(BUILDDIR)/%.o : $(RSRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o : $(SRCDIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(BUILDDIR)/%.o : $(CPUSRCDIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(BUILDDIR)/%.o : $(CORESRCDIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@
