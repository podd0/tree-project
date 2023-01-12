#include <iostream>
#include <yocto/yocto_sampling.h>
#include <yocto/yocto_cli.h>
#include <yocto/yocto_shape.h>
#include <yocto/yocto_sceneio.h>

using std::cout;
using std::endl;

using namespace yocto;

shape_data pointgen_sphere(int samples, rng_state rng)
{
	shape_data sh;
	for (int i : range(samples))
	{
		vec3f rsu = sample_sphere(rand2f(rng));
		float radius = rand1f(rng);
		radius = pow(sinf(radius * pi / 2), 0.8f);
		rsu *= radius;
		sh.positions.push_back(rsu);
		sh.points.push_back(i);
	}
	return sh;
}

shape_data pointgen_cylinder(int samples, rng_state rng)
{
	shape_data sh;
	for (int i : range(samples))
	{
		auto [x, z] = sample_disk(rand2f(rng));
		sh.positions.push_back({x, rand1f(rng) * 2, z});
		sh.points.push_back(i);
	}
	auto [mx, my, mz] = sh.positions[0];
	float Mx = mx, My = my, Mz = mz;
	return sh;
}

shape_data pointgen_cone(int samples, rng_state rng)
{
	shape_data sh;
	for (int i : range(samples))
	{
		float y = pow(rand1f(rng), 1.0f / 3);
		auto [x, z] = sample_disk(rand2f(rng)) * y;
		sh.positions.push_back({x, -2 * y + 2, z});
		sh.points.push_back(i);
	}
	sh.positions.push_back({0, 0, 0});
	sh.positions.push_back({1, 0, 0});
	sh.positions.push_back({0, 0, 1});
	int n = sh.positions.size();
	sh.triangles.push_back({n - 1, n - 2, n - 3});
	auto [mx, my, mz] = sh.positions[0];
	float Mx = mx, My = my, Mz = mz;
	return sh;
}

void run(const vector<string> &args)
{
	uint64_t seed = time(0);
	string shape = "sphere";
	string output = "points.ply";
	int samples = 1000;

	auto cli = make_cli("pointgen", "generate points inside a shape");
	add_option(cli, "seed", seed, "rng seed (defaults time)");
	add_option(cli, "shape", shape, "shape to fill with points (available sphere)");
	add_option(cli, "samples", samples, "number of samples");
	add_option(cli, "output", output, "output file");
	parse_cli(cli, args);

	rng_state rng = make_rng(seed);
	shape_data sh;
	if (shape == "sphere")
		sh = pointgen_sphere(samples, rng);
	else if (shape == "cone")
		sh = pointgen_cone(samples, rng);
	else if (shape == "cylinder")
		sh = pointgen_cylinder(samples, rng);
	else
		return;
	save_shape(output, sh);
}

int main(int argc, const char *argv[])
{
	try
	{
		run({argv, argv + argc});
		return 0;
	}
	catch (const std::exception &error)
	{
		print_error(error.what());
		return 1;
	}
}
