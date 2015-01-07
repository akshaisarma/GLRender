#ifndef PARSE_H
#define PARSE_H

#include <iostream>
#include <vector>
using namespace std;

class bezier_surf;

void read_wavefront_file (const char *file, vector<int> &tris, vector<float> &verts);

void read_bezier_file (const char *file, vector<bezier_surf> &sfs);

// Returns true if file is OBJ
bool checkIfOBJFileType (const char *file);

#endif
