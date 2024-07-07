#include ".test.h"
#include "pt.h"

TEST
{
	pt::Tracer<float>::Framebuffer<pt::RGB8> fb(128, 128);

	for (size_t r = 0; r < 128; r++)
	for (size_t c = 0; c < 128; c++)
	{
		fb[r][c].r = 255 * r / 128;
		fb[r][c].g = 255 * c / 128;
	}

	fb.save_as_ppm("/tmp/test.ppm", 255);
}