
CXXFLAGS = -g -I../Cpu6502Core -Wall -Werror -Wno-sign-compare
CXXFLAGS += -DDEBUGIO=3 -DDEBUGVID=1 -DDEBUG6502=4

CXXSRCS=	../Cpu6502Core/Cpu6502.cpp	\
		Atari2600.cpp			\
		Atari2600Hw.cpp			\
		Atari2600TIA.cpp		\
		Mos6532Riot.cpp			\
		Atari2600VideoStub.cpp		\
		test.cpp

CSRCS=		testrom.c



OBJS=$(CXXSRCS:.cpp=.o) $(CSRCS:.c=.o)

.PHONY: clean

atari: $(OBJS)
	g++ -o $@ $(OBJS)

clean:
	$(RM) $(OBJS) atari

