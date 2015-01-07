#ifndef BEZIER_H_
#define BEZIER_H_

#include <vector>
#include "amath.h"
using namespace std;

/* I am choosing to use my own point structure, although I could very
 * well use vec3.
 */
struct point {
	double x,y,z;
	point() {}
	point(double x, double y, double z) {
		this->x = x;
		this->y = y;
		this->z = z;
	}

	point& operator+= (const point& a) {
		x += a.x;
		y += a.y;
		z += a.z;
		return *this;
	}

	point operator+ (const point& a) {
		return point(x + a.x, y + a.y, z + a.z);
	}

	point operator- (const point& a) {
		return point(x - a.x, y - a.y, z - a.z);
	}

	point operator* (double a) {
		return point(x*a, y*a, z*a);
	}
};

typedef point vect;

class bezier_surf {
private:
	vector < vector<point> > controls;
	int u_deg;
	int v_deg;

	vector<point> getControlColumn(int u) {
		vector<point> ret;
		for (int i = 0; i <= v_deg; i++)
			ret.push_back(controls[i][u]);
		return ret;
	}

	vector<point> getControlRow(int v) {
		return controls[v];
	}

public:
	bezier_surf(vector<double> p, int u, int v) {
		u_deg = u;
		v_deg = v;
		for (int j = 0; j <= v; j++) {
			vector<point> row;
			for (int i = 0; i <= u; i++) {
				int l = 3*i + 3*(u_deg+1)*j;
				row.push_back(point(p[l], p[l+1], p[l+2]));
			}
			controls.push_back(row);
		}
	}

	int degree_u() {
		return u_deg;
	}

	int degree_v() {
		return v_deg;
	}

	void print() {
		cout << "=============================" << endl;
		cout << "Surface : " << u_deg << " " << v_deg << endl;
		for (int j = 0; j <= v_deg; j++) {
			for (int i = 0; i <= u_deg; i++) {
				point pt = controls[v_deg-j][i];
				cout << pt.x << " " << pt.y << " " << pt.z << "\t\t";
			}
			cout << endl;
		}
	}

	int getUSamples(int samples) {
		return u_deg*samples;
	}

	int getVSamples(int samples) {
		return v_deg*samples;
	}

	/* samples refers to the number of samples, with samples = 1
	 * That is, # samples in u = u_deg * samples and like wise for v
	 */
	bool sample(int samples, vector<vec4> &vertices, vector<vec4> &normals) {
		if (samples <= 1)
			return false;

		int u_sam = u_deg * samples;
		int v_sam = v_deg * samples;
		double u_spac = 1.0/(u_sam-1);
		double v_spac = 1.0/(v_sam-1);

		int v_c = 0;
		double v = 0;
		while (v_c < v_sam) {
			double u = 0;
			int u_c = 0;
			while (u_c < u_sam) {
				vec4 point;
				vec4 norm;
				evaluate(u, v, point, norm);
				vertices.push_back(point);
				normals.push_back(norm);
				u += u_spac;
				u_c++;
			}
			v += v_spac;
			v_c++;
		}
		return true;
	}

	bool evaluate(double u, double v, vec4 &pt, vec4 &norm) {

		vector<point> u_controls;

		vect garbageTan;
		for (int i = 0; i <= v_deg; i++) {
			point newControl;
			eval_bez(getControlRow(i), u_deg, u, newControl, garbageTan);
			u_controls.push_back(newControl);
		}

		point sample;
		vect v_tan;
		eval_bez(u_controls, v_deg, 1-v, sample, v_tan); // 1-v because v starts from bottom

		vector<point> v_controls;
		for (int i = 0; i <= u_deg; i++) {
			point newControl;
			eval_bez(getControlColumn(i), v_deg, 1-v, newControl, garbageTan);
			v_controls.push_back(newControl);
		}

		point copySample;
		vect u_tan;
		eval_bez(v_controls, u_deg, u, copySample, u_tan);

		vec3 u_t = vec3(u_tan.x, u_tan.y, u_tan.z);
		vec3 v_t = vec3(v_tan.x, v_tan.y, v_tan.z);
		vec3 n_t = normalize(cross(u_t, v_t));
		norm = vec4(n_t.x, n_t.y, n_t.z, 0.0);
		pt = vec4(sample.x, sample.y, sample.z, 1.0);
		return true;
	}

	/* cur should be the last thing computed
	 * last stores the value that should be overwritten by cur
	 * The value immediately behind that is the value we need to subtract
	 * from last
	 * 1
	 * 2 5
	 * 3 6 8
	 * 4 7 9 10
	 * 8 is workingArray[degree-1], last = 9, cur is 10
	 */
	static void eval_bez(const vector<point> controlpoints, int degree, double t,
														point &pnt, vect &tangent) {
		point *workingArray = new point[degree+1];
		for (int i = 0; i <= degree; i++)
			workingArray[i] = controlpoints[i];

		point last, cur;
		int i, j;
		for (j = 0; j < degree; j++) {
			last = cur = workingArray[j];
			for (i = j+1; i <= degree; i++) {
				workingArray[i-1] = cur;
				cur = workingArray[i]*t + last*(1-t);
				last = workingArray[i];
			}
			workingArray[i-1] = cur;
		}
		pnt = cur;
		tangent = last - workingArray[degree-1];
		delete workingArray;
	}
};

#endif /* BEZIER_H_ */
