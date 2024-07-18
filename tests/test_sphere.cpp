// #include ".test.h"
#include "pt.h"
#include <optional>
#include <assert.h>

using vec3 = pt::Tracer<float>::vec3;
using vec4 = pt::Tracer<float>::vec4;
using mat4 = pt::Tracer<float>::mat4;
using Ray = pt::Tracer<float>::Ray;
using Sample = pt::Tracer<float>::Sample;
using Material = pt::Tracer<float>::Material;
using sdf = pt::Tracer<float>::sdf;

inline vec3 normal_color(const Sample& s)
{
	return ((*s.normal + 1) / 2);
}

vec4 normal_material(const Sample& s)
{
	return normal_color(s).append<4>({1.f});
}

vec4 band_material(const Sample& s)
{
	auto p = (cos((*s.position)[2]) + 1.f) * 0.5f;
	// return normal_material
	return vec4{0.25f, 0.25f, 0.25f, 1.f}.lerp({0.75f, 0.75f, 0.75f, 1.f}, p);
}

vec4 checker_material(const Sample& s)
{
	auto p = *s.position;
	auto r = p.round().abs();
	// static const auto white = vec4{1, 1, 1, 1};
	// static const auto black = vec4{0, 0, 0, 1};

	auto w = fmodf(r[0] + r[1] + r[2], 2.f);
	w = std::clamp(w, 0.25f, 1.f);

	return (normal_color(s) * w).append<4>({1.f});

	// return (int)(r[0] + r[1] + r[2]) % 2 == 0 ? white : black;
}

struct SphereScene : public pt::Tracer<float>::Scene
{
	struct {
		vec3 position;
		vec3 color;
	} light;

	SphereScene()
	{
		light.position = {0, 100, 10};
		light.color = { 1, 1, 1 };
	}

	virtual float sample_sdf(const vec3& p) override
	{
		return std::min({ 
			sdf::sphere(p, {0, 0, 0}, 2.5f),
			sdf::plane(p, {0, 1, 0}, -10),
		});
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
			normal = std::optional<vec3>{ pt::Tracer<float>::numerical_normal(*this, dist, p) };
			material = checker_material;
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
		return light.color *  pt::Tracer<float>::sample_light_power(*this, s, light.position, 8);
	}
};

struct TestFooter
{
	char msg[16];
};

// TEST
int main(int argc, const char* argv[])
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

	pt::Tracer<float>::Pinhole cam(0.001, sensor);
	for (int i = 0; i < 1; i++)
	{
		float t = M_PI * 2 * i / 30.f;

		vec3 p = {10 * cos(t), 4, 10 * sin(t)};
		vec3 d = -p.unit();
		vec3 r = vec3::cross({0, -1, 0}, d).unit();

		cam.T = mat4{
			{r[0], 0, d[0], p[0]},
			{r[1], 1, d[1], p[1]},
			{r[2], 0, d[2], p[2]},
			{0, 0, 0, 1},
		};

		pt::Tracer<float>::trace<pt::RGB8>(cam, scene)
		.save_as_ppm_with_footer<TestFooter>("/tmp/seq_sphere" + std::to_string(i) + ".ppm", TestFooter{ "hello world" });
	}


}