
CXXFLAGS= -g -Wall -Werror -Wno-sign-compare
CXXFLAGS+= -DDEBUGIO=3 -I../Cpu6502Core

CXXSRCS=	../Cpu6502Core/Cpu6502.cpp \
		Pet2001.cpp		\
		Pet2001Hw.cpp		\
		Pet2001Io.cpp		\
		PetCassHw.cpp		\
		PetIeeeHw.cpp		\
		PetVideoStub.cpp 	\
		test.cpp

CSRCS=		petrom1.c


OBJS=$(CXXSRCS:.cpp=.o) $(CSRCS:.c=.o)

.PHONY: clean

pet: $(OBJS)
	g++ -o $@ $(OBJS)

clean:
	$(RM) $(OBJS) pet

