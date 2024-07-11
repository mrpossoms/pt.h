#include ".test.h"
#include "pt.h"
#include <optional>

using vec3 = pt::Tracer<float>::vec3;
using vec4 = pt::Tracer<float>::vec4;
using Ray = pt::Tracer<float>::Ray;
using Sample = pt::Tracer<float>::Sample;
using Material = pt::Tracer<float>::Material;
// using num_norm = pt::Tracer<float>::numerical_normal;

struct Sphere
{
	vec3 origin;
	float radius;
};

struct Plane
{
	float height = -5;
	vec3 normal = {0,1,0};
};

inline float sphere_sdf(const vec3& p, const vec3& origin, float radius)
{
	return (origin - p).magnitude() - radius;
}

inline float plane_sdf(const vec3& p, const vec3& normal, float height)
{
	return p.dot(normal) - height;
}

float box_sdf(const vec3& p, const vec3& o, const vec3& b)
{
  vec3 q = (p - o).abs() - b;
  return q.clamp({0,0,0}, q).magnitude() + std::min(0.f, q.max_component());
}

vec4 normal_material(const Sample& s)
{
	return ((*s.normal + 1) / 2).append<4>({0.5f});
}

vec3 white(const Sample& s)
{
	return {1, 1, 1};
}

struct SphereScene : public pt::Tracer<float>::Scene
{
	Sphere sphere;
	Plane plane;
	struct {
		vec3 position;
		vec3 color;
	} light;

	SphereScene()
	{
		sphere.origin = {4, -5, 10};
		sphere.radius = 2.5;
		light.position = {0, 100, 10};
		light.color = { 1, 0.5, 0.5 };
	}

	virtual float sample_sdf(const vec3& p) override
	{
		return std::min(
			std::min(sphere_sdf(p, sphere.origin, sphere.radius), sphere_sdf(p, {-4, 0, 8}, sphere.radius)),//box_sdf(p, {-4,0,10}, {1, 1, 1})), 
			plane_sdf(p, plane.normal, plane.height));
	}

	virtual Sample sample_surface(const vec3& p) override
	{
		auto dist = sample_sdf(p);
		auto normal = std::optional<vec3>{};
		auto position = std::optional<vec3>{};
		Material material = nullptr;

		assert(!isnan(dist));

		if (dist < 0.001) {
			position = std::optional<vec3>{ p };
			normal = std::optional<vec3>{ pt::Tracer<float>::numerical_normal(*this, p) };
			material = normal_material;
		}

		return Sample{
			.dist_to_surface = dist,
			.normal = normal,
			.position = position,
			.material = material,
		};
	}

	virtual vec3 sample_light(const Sample& s) override
	{
		const auto k = 4;
		auto p = *s.position;
		auto dir = (light.position - p).unit();
		Ray r = { .o = p, .d = dir };
		float t = 0;
		float res = 1;

		for (unsigned i = 0; i < 128 && t < 1000; i++) {
			auto sample = sample_surface(r.position(t));
			t += abs(sample.dist_to_surface);

			if (i > 0 && sample.dist_to_surface < std::numeric_limits<float>::epsilon()) {
				return {0, 0, 0};
			}
			res = std::min(res, k * sample.dist_to_surface / t);
		}

		return light.color *  res;
	}

	virtual vec3 sample_space(const vec3& p0, const vec3& p1) override
	{
		return {0, 0, 0};
	}
};

TEST
{
	pt::Tracer<float>::Sensor sensor = {
		.corners = {
			{-0.001, 0.001, 0},
			{ 0.001,-0.001, 0},
		},
		.rows = 256,
		.cols = 256,
	};

	SphereScene scene;

	pt::Tracer<float>::Pinhole cam(0.01, sensor);

	pt::Tracer<float>::trace<pt::RGB8>(cam, scene).save_as_ppm("/tmp/sphere.ppm");
}