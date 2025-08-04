#include <stdint.h>

#include "Atari2600VideoStub.h"
#include "Atari2600.h"

extern const uint8_t testrom[];

int
main(int argc, char *argv[])
{
	Atari2600VideoStub video;
	Atari2600 atari(&video);

	atari.setRom(testrom, 0x1000);

	atari.reset();
	atari.cycle();
	video.reset();

	for (;;) {
		atari.cycle();
	}
}
