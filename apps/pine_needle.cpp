#include <iostream>
#include <yocto/yocto_sampling.h>
#include <yocto/yocto_cli.h>
#include <yocto/yocto_shape.h>
#include <yocto/yocto_sceneio.h>
#include <yocto/branch.h>

using std::cout;
using std::endl;

using namespace yocto;

void generator_pine_needle()
{
  shape_data sh;
	sh.positions = { {0, 0, 0},
					 {0, 0, -1},
					 {0, 0, -2},
					 {0, 0, -3}};
	sh.lines = {{0, 1}, {1, 2}, {2, 3}};
	vector<vec3f> needles_center = sh.positions;
	int s = needles_center.size();
	for (int i = 0;  i < s; i++) {
		vec3f center = needles_center[i];
		sh.positions.push_back({center.x, center.y + 1.0f, center.z - 1.0f});
		sh.positions.push_back({center.x, center.y - 1.0f, center.z - 1.0f});
		sh.positions.push_back({center.x + 1.0f, center.y, center.z - 1.0f});
		sh.positions.push_back({center.x - 1.0f, center.y, center.z - 1.0f});
		sh.lines.push_back({i, 4*i + 4});
		sh.lines.push_back({i, 4*i + 5});
		sh.lines.push_back({i, 4*i + 6}); 
		sh.lines.push_back({i, 4*i + 7});
	}	
	save_shape("pine_needle.ply", sh);
	shape_data leaf = points_to_spheres(sh.positions, 2, 0.03);
	merge_shape_inplace(leaf, lines_to_cylinders(sh.lines, sh.positions, 4, 0.04));
	save_shape("pine_needle_3d.ply", leaf);
}

int main(int argc, const char *argv[])
{
	generator_pine_needle();
}
