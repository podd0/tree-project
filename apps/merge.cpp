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

void merge_shape_inplace(shape_data &shape, const shape_data &merge)
{
    auto offset = (int)shape.positions.size();
    for (auto &p : merge.points)
        shape.points.push_back(p + offset);
    for (auto &l : merge.lines)
        shape.lines.push_back({l.x + offset, l.y + offset});
    for (auto &t : merge.triangles)
        shape.triangles.push_back({t.x + offset, t.y + offset, t.z + offset});
    for (auto &q : merge.quads)
        shape.quads.push_back(
            {q.x + offset, q.y + offset, q.z + offset, q.w + offset});
    shape.positions.insert(
        shape.positions.end(), merge.positions.begin(), merge.positions.end());
    shape.tangents.insert(
        shape.tangents.end(), merge.tangents.begin(), merge.tangents.end());
    shape.texcoords.insert(
        shape.texcoords.end(), merge.texcoords.begin(), merge.texcoords.end());
    shape.colors.insert(
        shape.colors.end(), merge.colors.begin(), merge.colors.end());
    shape.radius.insert(
        shape.radius.end(), merge.radius.begin(), merge.radius.end());
}

int main(int argc, const char *argv[])
{
	shape_data sh;
	sh.positions = { {-1, 0, -1},
					 {-1, 0, 1},
					 {1, 0, 1},
					 {1, 0, -1} };
	sh.quads = {{1, 2, 3, 0}};
	//sh.texcoords = {{0, 0}, {1, 0}, {1, 1}, {1, 0}};
	save_shape("leaf.ply", sh);
	// auto sh2 = sh;

	// for(auto &p:sh2.positions)
	// 	p+= {0,0,1};
	// merge_shape_inplace(sh, sh2);
	// save_shape("quadrati.ply", sh);

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
