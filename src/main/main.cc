#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/freeglut_ext.h>

#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <string>
#include <cstdlib>
#include <vector>
#include <cassert>

#include "amath.h"
#include "bezier_surface.h"
#include "parse_file.h"

using namespace std;

typedef amath::vec4  point4;
typedef amath::vec4  color4;

GLuint buffers[2];

// camera's position, for lighting calculations
point4 eye;
const point4 at = vec4(0.0, 0.0, 0.0, 1.0);
vec4 up;

bool use_checker = false;
const int MAX_FINE = 64;
const int MIN_COARSE = 1;
unsigned int checkerboard_coarseness = 2; // Number of checkboard patterns in xyz

// Bezier limits
bool bezier_changed = false; // If vertices changed
bool bezier_mode = false;
const int MIN_DETAIL = 2;
const int MAX_DETAIL = 10;
unsigned int bezier_coarseness = 2; // Number of samples per degree

float theta = 0.0;  // rotation around the Y (up) axis
float phi = 90.0; // rotation around the X axis
float r = 5.0; // position on Z axis
const float ZENITH = 175.0;
const float NADIR = 5.0;
const float RMAX = 50.0;
const float RMIN = 2.0;

int NumVertices;
point4 *vertices = NULL;
vec4 *norms = NULL;

vector<bezier_surf> surfaces;

GLint checkerboard, view_pos, ctm, ptm;

GLuint program;

void init()
{
	GLuint vao;
	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );

	glGenBuffers(1, buffers);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);  // make it active

	glBufferData(GL_ARRAY_BUFFER, 2*sizeof(vec4)*NumVertices, NULL, GL_STATIC_DRAW);

	program = InitShader("vshader.glsl", "fshader.glsl");

	glUseProgram(program);

	GLuint loc, loc2;

	loc = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	loc2 = glGetAttribLocation(program, "vNorm");
	glEnableVertexAttribArray(loc2);
	glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(vec4)*NumVertices));

	glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(vec4)*NumVertices, vertices );
	glBufferSubData( GL_ARRAY_BUFFER, sizeof(vec4)*NumVertices, sizeof(vec4)*NumVertices, norms );

	// Get the uniforms
	checkerboard = glGetUniformLocation(program, "checkerboard");
	view_pos = glGetUniformLocation(program, "view_pos");
	ctm = glGetUniformLocation(program, "ctm");
	ptm = glGetUniformLocation(program, "ptm");

	glClearColor(1.0, 1.0, 1.0, 1.0);
}


void mouse_move_rotate (int x, int y)
{
	static int lastx = 0;
	static int lasty = 0;

	int amntX = x - lastx;
	int amntY = y - lasty;


	if (amntX != 0) {
		theta +=  amntX;
		if (theta > 360.0 ) theta -= 360.0;
		if (theta < 0.0 ) theta += 360.0;

		lastx = x;
	}

	if (amntY != 0) {
		phi += amntY;
		if (phi > ZENITH) phi = ZENITH;
		if (phi < NADIR) phi = NADIR;

		lasty = y;
	}
	glutPostRedisplay();
}

void mykey(unsigned char key, int mousex, int mousey)
{
	if(key=='q'|| key=='Q') {
		delete vertices;
		delete norms;
		exit(0);
	}

	// and r resets the view:
	if (key =='r') {
		theta = 0;
		phi = 90.0;
		r = 5.0;
		use_checker = false;
		checkerboard_coarseness = 2;
		glutPostRedisplay();
	}

	// s toggles checkerboard shading
	if (key == 's') {
		use_checker = use_checker ^ true;
		glutPostRedisplay();
	}

	// t makes the checkerboard finer
	if (key == 't' && use_checker && checkerboard_coarseness < MAX_FINE) {
		checkerboard_coarseness++;
		glutPostRedisplay();
	}

	// g makes the checkerboard coarser
	if (key == 'g' && use_checker && checkerboard_coarseness > MIN_COARSE) {
		checkerboard_coarseness--;
		glutPostRedisplay();
	}

	// z moves the view closer
	if (key == 'z' && r > RMIN) {
		r *= 0.9;
		glutPostRedisplay();
	}

	// x moves the view farther
	if (key == 'x' && r < RMAX) {
		r *= 1.1;
		glutPostRedisplay();
	}

	// < decreases detail
	if (key == '<' && bezier_coarseness > MIN_DETAIL) {
		bezier_coarseness--;
		bezier_changed = true;
		glutPostRedisplay();
	}

	// > increases detail
	if (key == '>' && bezier_coarseness < MAX_DETAIL) {
		bezier_coarseness++;
		bezier_changed = true;
		glutPostRedisplay();
	}

}

void loadOBJ(const char *file_name)
{
	vector <int> tris;
	vector <float> verts;
	read_wavefront_file(file_name, tris, verts);

	NumVertices = (int) tris.size();
	vertices = new point4[NumVertices];
	norms = new vec4[NumVertices];

	vec4 *vert_norms = new vec4[verts.size()/3];

	for (unsigned int i = 0; i < verts.size()/3; i++)
		vert_norms[i] = vec4(0.0);

	/* Add all normals for all vertices. Shared */
	for (unsigned int i = 0; i < NumVertices; i+=3) {
		int p1_i = tris[i];
		int p2_i = tris[i+1];
		int p3_i = tris[i+2];
		vertices[i] = point4(verts[3*p1_i], verts[3*p1_i+1], verts[3*p1_i+2], 1.0);
		vertices[i+1] = point4(verts[3*p2_i], verts[3*p2_i+1], verts[3*p2_i+2], 1.0);
		vertices[i+2] = point4(verts[3*p3_i], verts[3*p3_i+1], verts[3*p3_i+2], 1.0);

		vec4 norm = normalize(vec4(cross(vertices[i+1]-vertices[i], vertices[i+2]-vertices[i]), 0.0));
		vert_norms[p1_i] += norm;
		vert_norms[p2_i] += norm;
		vert_norms[p3_i] += norm;
	}

	for (unsigned int i = 0; i < verts.size()/3; i++)
		vert_norms[i] = normalize(vert_norms[i]);

	for (unsigned int i = 0; i < NumVertices; i+=3) {
		int p1_i = tris[i];
		int p2_i = tris[i+1];
		int p3_i = tris[i+2];
		norms[i] = vert_norms[p1_i];
		norms[i+1]= vert_norms[p2_i];
		norms[i+2] = vert_norms[p3_i];
	}
	delete vert_norms;
}

void loadBezierVertsAndNorms() {
	int n_verts = 0;
	for (int i = 0; i < surfaces.size(); ++i) {
		n_verts += 3 * ((2*surfaces[i].getUSamples(bezier_coarseness) - 2) *
				(surfaces[i].getVSamples(bezier_coarseness) - 1)  );
	}
	NumVertices = n_verts;

	delete vertices;
	delete norms;
	vertices = new point4[NumVertices];
	norms = new vec4[NumVertices];

	vector<point4> v_verts;
	vector<vec4> v_norms;
	for (int i = 0; i < surfaces.size(); ++i)
		surfaces[i].sample(bezier_coarseness, v_verts, v_norms);

	// Triangulate
	int to_skip = 0;
	int vPos = 0;
	for (int i = 0; i < surfaces.size(); ++i) {
		int u_sam = surfaces[i].getUSamples(bezier_coarseness);
		int v_sam = surfaces[i].getVSamples(bezier_coarseness);

		int verts_surf = u_sam*v_sam;
		for (int v = 0; v < v_sam-1; v++)
			for (int u = 0; u < u_sam-1; u++) {
				vec4 tri1_1 = v_verts[to_skip + v*u_sam + u];
				vec4 norm1_1 = v_norms[to_skip + v*u_sam + u];
				vec4 tri1_2 = v_verts[to_skip + (v+1)*u_sam + u+1];
				vec4 norm1_2 = v_norms[to_skip + (v+1)*u_sam + u+1];
				vec4 tri1_3 = v_verts[to_skip + (v+1)*u_sam + u];
				vec4 norm1_3 = v_norms[to_skip + (v+1)*u_sam + u];

				vec4 tri2_1 = tri1_2;
				vec4 norm2_1 = norm1_2;
				vec4 tri2_2 = tri1_1;
				vec4 norm2_2 = norm1_1;
				vec4 tri2_3 = v_verts[to_skip + v*u_sam + u+1];
				vec4 norm2_3 = v_norms[to_skip + v*u_sam + u+1];

				vertices[vPos] = tri1_1;
				vertices[vPos+1] = tri1_2;
				vertices[vPos+2] = tri1_3;
				vertices[vPos+3] = tri2_1;
				vertices[vPos+4] = tri2_2;
				vertices[vPos+5] = tri2_3;

				norms[vPos] = norm1_1;
				norms[vPos+1] = norm1_2;
				norms[vPos+2] = norm1_3;
				norms[vPos+3] = norm2_1;
				norms[vPos+4] = norm2_2;
				norms[vPos+5] = norm2_3;

				vPos += 6;
			}
		to_skip += verts_surf;
	}

}

void loadBezier(const char *file_name) {
	read_bezier_file(file_name, surfaces);
	bezier_mode = true;
	loadBezierVertsAndNorms();
}

void display( void )
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// based on where the mouse has moved to, construct a transformation matrix:
	// compute new eye and up vector
	GLfloat p = DegreesToRadians*phi;
	GLfloat t = DegreesToRadians*theta;
	eye = point4(r*sin(p)*sin(t), r*cos(p), r*sin(p)*cos(t), 1.0);
	vec4 to_center = normalize(at - eye);
	vec4 to_side = normalize(cross(to_center, vec4(0, 1, 0, 0)));
	up = normalize(vec4(cross(to_side, to_center), 0.0));

	glUniform4fv(view_pos, 1, eye);

	glUniformMatrix4fv(ctm, 1, GL_TRUE, LookAt(eye, at, up));
	glUniformMatrix4fv(ptm, 1, GL_TRUE, Perspective(40, 1.0, 1, 50));

	if (use_checker)
		glUniform1i(checkerboard, checkerboard_coarseness);
	else
		glUniform1i(checkerboard, 0);

	if (bezier_mode && bezier_changed) {
		loadBezierVertsAndNorms();
		glBufferData(GL_ARRAY_BUFFER, 2*sizeof(vec4)*NumVertices, NULL, GL_STATIC_DRAW);

		GLuint loc, loc2;
		loc = glGetAttribLocation(program, "vPosition");
		glEnableVertexAttribArray(loc);
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

		loc2 = glGetAttribLocation(program, "vNorm");
		glEnableVertexAttribArray(loc2);
		glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(vec4)*NumVertices));

		glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(vec4)*NumVertices, vertices );
		glBufferSubData( GL_ARRAY_BUFFER, sizeof(vec4)*NumVertices, sizeof(vec4)*NumVertices, norms );
	}
	bezier_changed = false;

	glDrawArrays(GL_TRIANGLES, 0, NumVertices);

	glutSwapBuffers();
}

int main(int argc, char** argv)
{
	if (argc != 2) {
		cout << "Usage: glrender pathToOBJFile" << endl;
		return 1;
	}

	if (checkIfOBJFileType(argv[1]))
		loadOBJ(argv[1]);
	else
		loadBezier(argv[1]);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);

	glutInitWindowSize(512, 512);
	glutCreateWindow("Rotate OBJ File");

	glutDisplayFunc(display);
	glutMotionFunc(mouse_move_rotate);
	glutKeyboardFunc(mykey);

	glewInit();
	init();
	glEnable(GL_DEPTH_TEST);
	glutMainLoop();
	return 0;
}

