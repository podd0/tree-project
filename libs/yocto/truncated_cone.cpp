#include <iostream>
#include <yocto/yocto_sampling.h>
#include <yocto/yocto_cli.h>
#include <yocto/yocto_shape.h>
#include <yocto/yocto_sceneio.h>

namespace yocto
{
	using std::cout;
	using std::endl;

	shape_data make_truncated_cone(float rad1, float rad2, float height, int steps = 16)
	{
		shape_data sh;
		float stepangle = 2 * pi / steps;
		for (int i = 0; i < steps; i++)
		{
			auto x = cosf(i * stepangle), y = sinf(i * stepangle);
			sh.positions.push_back(vec3f{x, y, 0} * rad1 + vec3f{0, 0, height});
			sh.positions.push_back(vec3f{x, y, 0} * rad2 - vec3f{0, 0, height});
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
		return sh;
	}

	// void run(const vector<string> &args)
	// {
	// 	vector<string> inputs = {};
	// 	string output = "cone.ply";

	// 	auto cli = make_cli("merge", "merge different shapes (only points, lines and quads, the rest of the shape is lost)");
	// 	add_option(cli, "inputs", inputs, "files to merge");
	// 	add_option(cli, "output", output, "output file");
	// 	// parse_cli(cli, args);

	// 	save_shape(output, make_truncated_cone(1, 0.5, 1));
	// }

	// int main(int argc, const char *argv[])
	// {
	// 	try
	// 	{
	// 		run({argv, argv + argc});
	// 		return 0;
	// 	}
	// 	catch (const std::exception &error)
	// 	{
	// 		print_error(error.what());
	// 		return 1;
	// 	}
	// }
}
