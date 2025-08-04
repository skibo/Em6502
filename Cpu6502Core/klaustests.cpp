//
// klaustests.cpp
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
	cpu->setPc(0x400);
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

	if (mem.loadfile("6502_65C02_functional_tests/bin_files/"
			 "6502_functional_test.bin", 0) < 0) {
		fprintf(stderr, "failed to load 6502_functional_test.bin\n");
		exit(1);
	}
	printf("6502_functional_test:\n");
	doTest(&cpu);
	printf("    cycles=%d\n", cycles);

#ifdef CPU65C02
	if (mem.loadfile("6502_65C02_functional_tests/bin_files/"
			 "65C02_extended_opcodes_test.bin", 0) < 0) {
		fprintf(stderr,
			"failed to load 65C02_extended_opcodes_test.bin");
		exit(1);
	}
	printf("65C02_extended_opcocdes_test:\n");
	doTest(&cpu);
	printf("    cycles=%d\n", cycles);
#else
	printf("Warning: 65C02 tests not executed.\n");
#endif
}
