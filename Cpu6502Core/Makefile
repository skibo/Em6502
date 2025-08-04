
CXXFLAGS= -g -Wall -Werror -Wno-sign-compare
# CXXFLAGS += -DILL6502 -DDEBUG6502=5

CXXSRCS=	Cpu6502.cpp		\
		MemGeneric.cpp


OBJS=$(CXXSRCS:.cpp=.o)

.PHONY: clean

TARGETS=klaustests shorttest test.bin illgl.bin

default: $(TARGETS)

klaustests: $(OBJS) klaustests.o
	g++ -o $@ $(OBJS) klaustests.o

shorttest: $(OBJS) shorttest.o
	g++ -o $@ $(OBJS) shorttest.o

test.bin: test.asm
	dasm $< -o$@ -f3 -ltest.lst

illgl.bin: illgl.asm
	dasm $< -o$@ -f3 -lillgl.lst

clean:
	$(RM) *.o *.lst $(TARGETS)

