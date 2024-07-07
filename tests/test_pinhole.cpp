#include ".test.h"
#include "pt.h"

TEST
{
	pt::Tracer<float>::Sensor sensor = {
		.corners = {
			{-1, 1, 0},
			{ 1,-1, 0},
		},
	};

	pt::Tracer<float>::Pinhole cam(1, sensor);

	{ // ray emitted from the exact center of the sensor
		auto ray = cam.ray(0.5f, 0.5f);
		assert(ray.o.is_near({0,0,0}, 0.00001f));
		assert(ray.d.is_near({0,0,1}, 0.00001f));
	}

	{ // ray emitted from the left edge of the sensor
		auto ray = cam.ray(0.0f, 0.5f);
		assert(ray.o.is_near({0,0,0}, 0.00001f));
		assert(fabs(ray.d.angle_to({0,0,1}) - M_PI / 4) <= 0.00001f);
	}

	{ // ray emitted from the top edge of the sensor
		auto ray = cam.ray(0.5f, 0.0f);
		assert(ray.o.is_near({0,0,0}, 0.00001f));
		assert(fabs(ray.d.angle_to({0,0,1}) - M_PI / 4) <= 0.00001f);
	}

	return 0;
}
