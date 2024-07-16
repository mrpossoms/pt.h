/**
 * @file pt.h
 *
 * The example header file for foo
 *
 * All Rights Reserved
 *
 * Author: Kirk Roerig [mr.possoms@gmail.com]
 */

#pragma once
#include "xmath.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <optional>
#include <assert.h>
#include <cstdint>

using namespace xmath;

namespace pt
{

union RGB8
{
	uint8_t v[3];
	struct {
		uint8_t r, g, b;
	};
};

// All units unless stated otherwise are same and in the same coordinate system. The coordinate
// system convention from the camera's perspective is +x to the right, +y up, and +z deep.
template<typename S>
struct Tracer
{

using vec2 = vec<2,S>;
using vec3 = vec<3,S>;
using vec4 = vec<4,S>;
using mat4 = mat<4,4,S>;

struct Sample;

// using Material = vec3 (&)(const Sample&);
typedef vec4 (*Material)(const Sample&);

struct Sample
{
	S dist_travelled;
	S dist_to_surface;
	// S attenuation;
	std::optional<vec3> normal;
	std::optional<vec3> position;
	std::optional<vec2> uv;
	Material material;
};

typedef S (*SDF)(const vec3&, void* ctx);

typedef Sample (*Surface)(const vec3&);

struct sdf 
{
	static inline S sphere(const vec3& p, const vec3& origin, S radius)
	{
		return (origin - p).magnitude() - radius;
	}

	static inline S plane(const vec3& p, const vec3& normal, S height)
	{
		return p.dot(normal) - height;
	}

	static S box(const vec3& p, const vec3& o, const vec3& b)
	{
	  auto q = (p - o).abs() - b;
	  return q.clamp({0, 0, 0}, q).magnitude() + std::min(q.max_component(),0.f);
	}
};

// TODO: this might be better using templates and concepts
struct Scene
{
	virtual S sample_sdf(const vec3& p) = 0;

	virtual Sample sample_surface(const vec3& p) = 0;

	virtual vec3 sample_space(const vec3& p0, const vec3& p1) { return {0, 0, 0}; }

	virtual vec3 sample_light(const Sample& s) = 0;
};

struct Ray { 
	vec3 o; vec3 d;
	void operator +=(S t) { o += d * t; }
	vec3 position(S t) { return o + d * t; }
};

struct Sensor
{
	vec3 corners[2]; // Lying on the xy plane 0 z
	unsigned rows;
	unsigned cols;

	vec3 plane_point(S u, S v)
	{
		assert(u >= 0 && u <= 1);
		assert(v >= 0 && v <= 1);

		return vec3{
			corners[0][0] + u * (corners[1][0] - corners[0][0]),
			corners[0][1] + v * (corners[1][1] - corners[0][1]),
			0,
		};
	}
};

struct Pinhole
{
	S focal_length;
	Sensor& sensor;
	mat4 T_sensor;
	mat4 T;

	Pinhole() = delete;

	Pinhole(S focal_length, Sensor& sensor, const mat4& T_sensor=mat4::I()) : 
	sensor(sensor), T_sensor(T_sensor), T(mat4::I())
	{
		this->focal_length = focal_length;
	}

	Ray ray(S u, S v)
	{
		auto pp_0 = sensor.plane_point(u, v);
		auto pp_w = T_sensor * pp_0 + vec3{0, 0, -focal_length}; // point on the plane in global coords
		
		return Ray{
			.o = T * vec3{0, 0, 0},
			.d = T.R_T() * (-pp_w).unit(),
		};
	}
};

template<typename PIX>
struct Framebuffer
{
	std::vector<PIX> buffer;
	size_t rows;
	size_t cols;

	Framebuffer() = delete;

	Framebuffer(size_t rows, size_t cols) : buffer(rows * cols) {
		this->rows = rows;
		this->cols = cols;
	}

	inline PIX* operator[](size_t r) { return buffer.data() + (cols * r); }

	void save_as_ppm(const std::string& path, unsigned max_val=255) {
		std::ofstream stream(path, std::ios::binary);
		stream << "P6\n" << cols << " " << rows << "\n" << max_val << "\n";
		stream.write((const char*)(buffer.data()), sizeof(PIX) * cols * rows);
	}
};

static vec3 numerical_normal(Scene& scene, const S d0, const vec3& p, const S e=0.0001)
{
	auto dx_dd = scene.sample_sdf(p + vec3{e,0,0}) - d0;
	auto dy_dd = scene.sample_sdf(p + vec3{0,e,0}) - d0;
	auto dz_dd = scene.sample_sdf(p + vec3{0,0,e}) - d0;

	assert(dx_dd != 0 || dy_dd != 0 || dz_dd != 0);

	return vec3{dx_dd, dy_dd, dz_dd}.unit();
}

static float sample_light_power(Scene& scene, const Sample& s, const vec3& light_pos, S light_area, unsigned max_steps=256)
{
	const auto k = light_area;
	auto p = *s.position;
	auto dir = (light_pos - p).unit();

	Ray r = { .o = p, .d = dir };
	float t = 0.001f;
	float res = 1;

	for (unsigned i = 0; i < max_steps && t < 1000; i++) {
		auto sample = scene.sample_surface(r.position(t));
		t += abs(sample.dist_to_surface);

		if (i > 0 && sample.dist_to_surface < std::numeric_limits<float>::epsilon()) {
			return 0;
		}
		res = std::min(res, k * sample.dist_to_surface / t);
	}

	return res;
}


template<typename PIX>
static Framebuffer<PIX> trace(Pinhole& camera, Scene& scene, unsigned max_steps=256)
{
	Framebuffer<PIX> fb(camera.sensor.rows, camera.sensor.cols);

	for (unsigned r = 0; r < camera.sensor.rows; r++) {
		S v = r / (S)camera.sensor.rows;

		for (unsigned c = 0; c < camera.sensor.cols; c++) {
			S u = c / (S)camera.sensor.cols;
			auto ray = camera.ray(1 - u, 1 - v);
			auto color = vec3{0, 0, 0};
			S power = (S)1.0;
			S t = 0.0001;

			assert(ray.o.is_finite());
			assert(ray.d.is_finite());

			auto p_i_1 = ray.o; // the last sample location

			// TODO: max distance should probably be computed as a function of the camera characteristics
			// the focal length and sensor size. Use these to determine the distance at which a M^2 area
			// projects into a single pixel.
			for (unsigned i = 0; i < max_steps && t < 1000; i++) {
				if (power < (S)0.00001) { break; }

				auto p_i = ray.position(t);

				auto sample = scene.sample_surface(p_i);
				sample.dist_travelled = t;

				t += sample.dist_to_surface;

				// TODO: consider a branchless soln.
				if (!sample.position || !sample.normal) { continue; }

				ray.o = *sample.position;
				ray.d = vec3::reflect(ray.d, *sample.normal);

				if (sample.material) {
					// if the sample is inside the surface, move it to the surface
					if (sample.dist_to_surface < 0) {
						*sample.position += *sample.normal * -sample.dist_to_surface;
						sample.dist_to_surface = 0;
					}

					auto mat_samp = sample.material(sample);
					auto attenuation = std::clamp(mat_samp[3], (S)0, (S)1);
					color += mat_samp.template slice<3>(0) * scene.sample_light(sample) * power;
					power *= (1.0 - attenuation);
				}
			}

			// TODO: this should be a constructor for the pixel format
			fb[r][c].r = std::min<uint8_t>(color[0] * 255, 255);
			fb[r][c].g = std::min<uint8_t>(color[1] * 255, 255);
			fb[r][c].b = std::min<uint8_t>(color[2] * 255, 255);
		}		
	}

	return fb;
}

}; // end struct Tracer



} // namespace pt