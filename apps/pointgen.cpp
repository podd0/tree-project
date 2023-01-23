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

shape_data generic_shape(int samples, rng_state rng, string file) {
	shape_data sh = quads_to_triangles(load_shape(file));
	shape_data sp;
	bbox3f bbox = bbox3f{};
	vector<vec3f> normals;
	for (vec3f p : sh.positions)
		bbox = merge(bbox, p);
	for (int i = 0; i < sh.triangles.size(); i++) {
		vec3f normal = eval_element_normal(sh, i);
		normals.push_back(normal);
	}
	float height = bbox.max.y - bbox.min.y;
	float length = bbox.max.x - bbox.min.x;
	float width = bbox.max.z - bbox.min.z;
	//creo il quadrato di base della bbox
	shape_bvh sh_bvh = make_shape_bvh(sh, false);
	
	vector<vec3f> positions_copy;
	for (int i : range(samples)) {
		positions_copy.push_back({rand1f(rng)*length + bbox.min.x, rand1f(rng) *height + bbox.min.y, rand1f(rng)*width + bbox.min.z});
	}
	for (vec3f p : positions_copy) {
		//tracciare il raggio
		ray3f r = {r.o = p, {1, 0, 0}};
		shape_intersection si = intersect_shape_bvh(sh_bvh, sh, r);
		if (si.hit) {
			vec3f p_out = sh.positions[sh.triangles[si.element].x] + normals[si.element];
			vec3f p_in = sh.positions[sh.triangles[si.element][0]] - normals[si.element];
			if (distance (p_out, p) > distance (p_in, p)) {
				sp.positions.push_back(p);
				sp.points.push_back(sp.positions.size()-1);
			}
		}
	}
	return sp;

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
	return sh;
}

void run(const vector<string> &args)
{
	uint64_t seed = time(0);
	string shape = "sphere";
	string output = "points.ply";
	string file;
	int samples = 1000;

	auto cli = make_cli("pointgen", "generate points inside a shape");
	add_option(cli, "seed", seed, "rng seed (defaults time)");
	add_option(cli, "shape", shape, "shape to fill with points (available sphere)");
	add_option(cli, "file", file, "name of the file from which you want to generate the points");
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
	else if (shape == "generic_shape") 
		sh = generic_shape(samples, rng, file);
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
