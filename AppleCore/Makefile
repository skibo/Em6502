
CXXFLAGS = -g -I../Cpu6502Core -Wall -Werror -Wno-sign-compare
CXXFLAGS += -DDEBUGIO=3 -DDEBUGVID=1 -DDEBUG6502=4

CXXSRCS=	../Cpu6502Core/Cpu6502.cpp	\
		Apple2.cpp			\
		Apple2Hw.cpp			\
		Apple2Io.cpp			\
		Apple2Disk2.cpp			\
		AppleVideoStub.cpp 		\
		test.cpp

CSRCS=		apple2roms.c


OBJS=$(CXXSRCS:.cpp=.o) $(CSRCS:.c=.o)

.PHONY: clean

apple: $(OBJS)
	g++ -o $@ $(OBJS)

clean:
	$(RM) $(OBJS) apple

