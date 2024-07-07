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

using vec3 = vec<3,S>;
using vec2 = vec<2,S>;
using mat4 = mat<4,4,S>;

struct Sample;

// using Material = vec3 (&)(const Sample&);
typedef vec3 (*Material)(const Sample&);

struct Sample
{
	S dist_to_surface;
	S attenuation;
	std::optional<vec3> normal;
	std::optional<vec3> position;
	std::optional<vec2> uv;
	Material material;
};

typedef S (*SDF)(const vec3&);

// using Surface = Sample (&)(const vec3& p);
typedef Sample (*Surface)(const vec3&);

struct Ray { vec3 o; vec3 d; };

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

	Pinhole(S focal_length, Sensor& sensor, const mat4& T_sensor=mat4::I()) : sensor(sensor), T_sensor(T_sensor)
	{
		focal_length = focal_length;
	}

	Ray ray(S u, S v, const mat4& T=mat4::I())
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

static vec3 numerical_normal(SDF sdf, const vec3& p, const S e=0.0001)
{
	auto d0 = sdf(p);
	auto dx_dd = sdf(p + vec3{e,0,0}) - d0;
	auto dy_dd = sdf(p + vec3{0,e,0}) - d0;
	auto dz_dd = sdf(p + vec3{0,0,e}) - d0;

	return vec3{dx_dd, dy_dd, dz_dd}.unit();
}

template<typename PIX>
static Framebuffer<PIX> trace(Pinhole& camera, Surface scene, unsigned max_steps=8)
{
	Framebuffer<PIX> fb(camera.sensor.rows, camera.sensor.cols);

	for (unsigned r = 0; r < camera.sensor.rows; r++) {
		S v = r / (S)camera.sensor.rows;

		for (unsigned c = 0; c < camera.sensor.cols; c++) {
			if (c == camera.sensor.cols >> 1 && r == camera.sensor.rows >> 1) {
				std::cout << "center\n";
			}

			S u = c / (S)camera.sensor.cols;
			auto ray = camera.ray(u, v);
			auto color = vec3{0, 0, 0};
			S power = (S)1.0;

			for (unsigned i = 0; i < max_steps; i++) {
				if (power < (S)0.00001) { break; }

				auto sample = scene(ray.o);

				// TODO: consider a branchless soln.
				if (sample.position) {
					if (sample.normal) {
						// reflect 
						ray.o = *sample.position;
						auto d = ray.d.dot(*sample.normal) * 2;
						ray.d = ray.d + *sample.normal * d;

						if (sample.material) {
							color += sample.material(sample) * power;
							power *= (1.0 - sample.attenuation);
						}
					}
				}
				else
				{
					ray.o += ray.d * sample.dist_to_surface;
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