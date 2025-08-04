#include <stdint.h>

#include "AppleVideoStub.h"
#include "Apple2.h"

int
main(int argc, char *argv[])
{
	AppleVideoStub video;
	Apple2 apple(&video);

	apple.reset();
	apple.cycle();
	video.reset();

	for (;;) {
		apple.cycle();
	}
}
