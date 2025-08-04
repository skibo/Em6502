//
// shorttest.cpp
//

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "MemSpace.h"
#include "MemGeneric.h"
#include "Cpu6502.h"

unsigned int cycles;

static void
doTest(Cpu6502 *cpu)
{
	uint16_t pc = 0xffff, last_pc0 = 0, last_pc1 = 0, last_pc2 = 0;
	int stuck = 16;

	cpu->reset();
	cycles = 0;
	cpu->setPc(0xf000);
	for (;;) {
		last_pc2 = last_pc1;
		last_pc1 = last_pc0;
		last_pc0 = pc;
		pc = cpu->getPc();

		if (last_pc2 == pc || last_pc1 == pc || last_pc0 == pc) {
			if (stuck-- == 0) {
				printf("Test finshed at pc=0x%04x\n", pc);
				printf("A=0x%02x X=0x%02x Y=0x%02x\n",
				       cpu->getA(), cpu->getX(), cpu->getY());
				printf("P=0x%02x SP=0x1%02x\n",
				       cpu->getP(), cpu->getSp());
				break;
			}
		} else
			stuck = 16;
		cpu->cycle();
		cycles++;
	}
}

int
main(int argc, char *argv[])
{
	MemGeneric mem;
	Cpu6502 cpu(&mem);
	const char *testfile = "illgl.bin";

	if (mem.loadfile(testfile, 0xf000) < 0) {
		fprintf(stderr, "failed to load %s\n", testfile);
		exit(1);
	}

	doTest(&cpu);
	printf("    cycles=%d\n", cycles);
}
