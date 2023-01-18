#include <iostream>
#include <yocto/yocto_sampling.h>
#include <yocto/yocto_cli.h>
#include <yocto/yocto_shape.h>
#include <yocto/yocto_sceneio.h>

using std::cout;
using std::endl;

using namespace yocto;

void run(const vector<string> &args)
{
	vector<string> inputs = {};
	string output = "merging.ply";

	auto cli = make_cli("merge", "merge different shapes (only points, lines and quads, the rest of the shape is lost)");
	add_option(cli, "inputs", inputs, "files to merge");
	add_option(cli, "output", output, "output file");
	parse_cli(cli, args);

	shape_data res;
	for (string s : inputs)
	{
		int n = res.positions.size();
		auto sh = load_shape(s);
		for (vec3f p : sh.positions)
			res.positions.push_back(p);
		for (auto p : sh.points)
			res.points.push_back(p + n);
		for (auto [a, b] : sh.lines)
			res.lines.push_back({a + n, b + n});
		for (auto [a, b, c] : sh.triangles)
			res.triangles.push_back({a + n, b + n, c + n});
		for (auto [a, b, c, d] : sh.quads)
			res.quads.push_back({a + n, b + n, c + n, d + n});
	}
	save_shape(output, res);
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
