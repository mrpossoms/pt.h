#include ".test.h"
#include "pt.h"
#include <optional>

using vec3 = pt::Tracer<float>::vec3;
using Sample = pt::Tracer<float>::Sample;
using Material = pt::Tracer<float>::Material;
// using num_norm = pt::Tracer<float>::numerical_normal;

vec3 normal_material(const Sample& s)
{
	return (*s.normal + vec3{1.0, 1.0, 1.0}) / 2.0;
}

float sphere_sdf(const vec3& p)
{
	auto p0 = vec3{0, 0, 10};
	return (p0 - p).magnitude() - 5;	
}

Sample sphere_surface(const vec3& p)
{
	auto dist = sphere_sdf(p);
	auto normal = std::optional<vec3>{};
	auto position = std::optional<vec3>{};
	Material material = nullptr;

	assert(!isnan(dist));

	if (dist < 0.001) {
		position = std::optional<vec3>{ p };
		normal = std::optional<vec3>{ pt::Tracer<float>::numerical_normal(sphere_sdf, p) };//(p - p0).unit() };
		material = normal_material;
	}

	return Sample{
		.dist_to_surface = dist,
		.attenuation = 1.0,
		.normal = normal,
		.position = position,
		.material = material,
	};
}

float box_sdf(const vec3& p)
{
	auto p0 = vec3{0, 0, 10};
	const vec3 r = {1, 2, 3};
	return ((p - p0).abs() - r).take_max({0, 0, 0}).magnitude();
}

Sample box_surface(const vec3& p)
{
	auto dist = box_sdf(p);
	auto normal = std::optional<vec3>{};
	auto position = std::optional<vec3>{};
	Material material = nullptr;
	assert(!isnan(dist));

	if (dist < 0.001) {
		position = std::optional<vec3>{ p };
		normal = std::optional<vec3>{ pt::Tracer<float>::numerical_normal(box_sdf, p) };
		material = normal_material;
	}

	return Sample{
		.dist_to_surface = dist,
		.attenuation = 1.0,
		.normal = normal,
		.position = position,
		.material = material,
	};
}

TEST
{
	pt::Tracer<float>::Sensor sensor = {
		.corners = {
			{-0.001, 0.001, 0},
			{ 0.001,-0.001, 0},
		},
		.rows = 128,
		.cols = 128,
	};

	pt::Tracer<float>::Pinhole cam(0.01, sensor);

	pt::Tracer<float>::trace<pt::RGB8>(cam, sphere_surface).save_as_ppm("/tmp/sphere.ppm");
}