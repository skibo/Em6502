#include <stdint.h>

#include "PetVideoStub.h"
#include "Pet2001.h"

extern const uint8_t petrom1[];

int
main(int argc, char *argv[])
{
	PetVideoStub video;
	Pet2001 pet(&video);

	pet.writeRom(0xC000, petrom1, 0x2800);
	pet.writeRom(0xF000, petrom1 + 0x2800, 0x1000);

	pet.reset();
	pet.cycle();
	video.reset();

	for (;;) {
		pet.cycle();
	}
}
