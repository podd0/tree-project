#include <iostream>
#include <yocto/yocto_sampling.h>
#include <yocto/yocto_cli.h>
#include <yocto/yocto_shape.h>
#include <yocto/yocto_sceneio.h>

using std::cout;
using std::endl;

using namespace yocto;

shape_data make_cone(float rad1, float rad2, float height)
{
	shape_data sh;
	int steps = 16;
	float stepangle = 2 * pi / steps;
	for (int i = 0; i < steps; i++)
	{
		auto x = cosf(i * stepangle), z = sinf(i * stepangle);
		sh.positions.push_back(vec3f{x, 0, z} * rad1);
		sh.positions.push_back(vec3f{x, 0, z} * rad2 + vec3f{0, height, 0});
	}
	for (int ci = 0; ci < steps; ci++)
	{
		int i = ci * 2;
		int mod = sh.positions.size();
		for (int h : range(1))
		{
			sh.triangles.push_back({i, i + 1, (i + 2) % mod});
			sh.triangles.push_back({(i + 2) % mod, i + 1, (i + 3) % mod});
			i++;
		}
	}
	sh.normals = compute_normals(sh);
	return sh;
}

void run(const vector<string> &args)
{
	vector<string> inputs = {};
	string output = "cone.ply";

	auto cli = make_cli("merge", "merge different shapes (only points, lines and quads, the rest of the shape is lost)");
	add_option(cli, "inputs", inputs, "files to merge");
	add_option(cli, "output", output, "output file");
	// parse_cli(cli, args);

	save_shape(output, make_uvcylinder());
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
