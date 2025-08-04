RSRCDIR=resources
SRCDIR=src
CORESRCDIR=../PetCore
CPUSRCDIR=../Cpu6502Core
BUILDDIR=build

CXXFLAGS = -O -Wall -Werror -Wno-sign-compare
CXXFLAGS += -I$(SRCDIR) -I$(CPUSRCDIR) -I$(CORESRCDIR)
CXXFLAGS += $(shell pkg-config --cflags gtkmm-3.0)
# CXXFLAGS += -DDEBUGIEEE=2

LDFLAGS = -export-dynamic
LDLIBS = $(shell pkg-config --libs gtkmm-3.0)

RSRCSRCS=	$(RSRCDIR)/petrom1.c 		\
		$(RSRCDIR)/petrom2.c 		\
		$(RSRCDIR)/petrom4.c 		\
		$(RSRCDIR)/charroms.c

PETGTKSRCS=	$(SRCDIR)/Pet2001GtkApp.cpp	\
		$(SRCDIR)/Pet2001GtkAppWin.cpp	\
		$(SRCDIR)/Pet2001GtkDisp.cpp	\
		$(SRCDIR)/Pet2001GtkCass.cpp	\
		$(SRCDIR)/Pet2001GtkIeee.cpp	\
		$(SRCDIR)/Pet2001GtkKeys.cpp

CPUSRCS=	$(CPUSRCDIR)/Cpu6502.cpp

DBGSRCS=	$(CPUSRCDIR)/Cpu6502GtkDebug.cpp

PETCORESRCS=	$(CORESRCDIR)/Pet2001.cpp	\
		$(CORESRCDIR)/Pet2001Hw.cpp	\
		$(CORESRCDIR)/Pet2001Io.cpp	\
		$(CORESRCDIR)/PetCassHw.cpp	\
		$(CORESRCDIR)/PetDisk.cpp	\
		$(CORESRCDIR)/PetIeeeHw.cpp

GRESOURCE=	$(BUILDDIR)/petgtk.gresource.cpp
GRCSOBJ= $(patsubst $(BUILDDIR)/%.cpp,$(BUILDDIR)/%.o,$(GRESOURCE))

RSRCOBJS= $(patsubst $(RSRCDIR)/%.c,$(BUILDDIR)/%.o,$(RSRCSRCS))
PETGTKOBJS= $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(PETGTKSRCS))
CPUOBJS= $(patsubst $(CPUSRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(CPUSRCS))
DBGOBJS= $(patsubst $(CPUSRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(DBGSRCS))
PETCOREOBJS= $(patsubst $(CORESRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(PETCORESRCS))
OBJS=$(RSRCOBJS) $(PETGTKOBJS) $(CPUOBJS) $(DBGOBJS) $(PETCOREOBJS) $(GRCSOBJ)

TARGETS=$(BUILDDIR)/pet2001gtk

.PHONY: default clean builddir

default: builddir $(TARGETS)

clean:
	$(RM) $(OBJS) $(TARGETS) $(GRESOURCE)

builddir:
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/pet2001gtk: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(GRESOURCE): $(RSRCDIR)/petgtk.glade $(RSRCDIR)/keyboard_800x267.png \
	$(RSRCDIR)/pet2001image.jpg $(RSRCDIR)/petgtk.resources.xml
	glib-compile-resources --target=$(GRESOURCE) --generate-source \
		--sourcedir=$(RSRCDIR) $(RSRCDIR)/petgtk.resources.xml

$(BUILDDIR)/%.o : $(RSRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o : $(SRCDIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(BUILDDIR)/%.o : $(CPUSRCDIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(BUILDDIR)/%.o : $(CPUSRCDIR)/%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILDDIR)/%.o : $(CORESRCDIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@
