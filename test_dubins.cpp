
// this program tests the online dubins code library
#include<stdio.h>
#include<iostream>
#include<vector>
#include "dubins.h"
#include "fssimplewindow.h"
#include <math.h>
using namespace std;



struct state {
	double x;
	double y;
	double angle;

	state(double x, double y, double angle) {
		this->x = x;
		this->y = y;
		this->angle = angle;
	}
};

vector<state> path_points;


int printConfiguration(double q[3], double x, void* user_data) {
	printf("%f, %f, %f, %f\n", q[0], q[1], q[2], x);
	state temp(q[0], q[1], q[2]);
	path_points.push_back(temp);
	return 0;
}

void Drawcar(double x, double y, double angle) {
	glColor3ub(255, 0, 0);
	glBegin(GL_LINE_LOOP);
	int scale = 1;
	glVertex2f((x + (8) * cos(angle) - (4) * sin(angle)) * scale, ((y + (8) * sin(angle) + (4) * cos(angle)) * scale));
	glVertex2f((x + (-8) * cos(angle) - (4) * sin(angle)) * scale, ((y + (-8) * sin(angle) + (4) * cos(angle)) * scale));
	glVertex2f((x + (-8) * cos(angle) - (-4) * sin(angle)) * scale, (y + (-8) * sin(angle) + (-4) * cos(angle)) * scale);
	glVertex2f((x + (8) * cos(angle) - (-4) * sin(angle)) * scale, ((y + (+8) * sin(angle) + (-4) * cos(angle)) * scale));
	glEnd();
}

void Drawpath(vector<state> path_points) {
	glColor3ub(0, 0, 0);
	glBegin(GL_LINE_STRIP);
	for (int i = 0; i < path_points.size(); i++) {
		glVertex2f(path_points[i].x*4, path_points[i].y*4);
	}
	glEnd();
	

}


int main()
{
	bool terminate = false;
	double q0[] = { 100,200,-4.71 };
	double q1[] = { 350,350,-1.57 };
	double turning_radius = 22.0;
	DubinsPath path;
	dubins_shortest_path(&path, q0, q1, turning_radius);
	
	dubins_path_sample_many(&path, 2, printConfiguration, NULL);
	FsOpenWindow(0, 0, 1600, 1600, 1);

	while (!terminate) {
		FsPollDevice();
		int key = FsInkey();
		if (key == FSKEY_ESC) {
			terminate = true;
		}
		/*for (int i = 0; i < path_points.size(); i++) {
			Drawcar(path_points[i].x, path_points[i].y, path_points[i].angle);
			
		}*/
		Drawpath(path_points);
		glFlush();
		FsSwapBuffers();
		FsSleep(100);
	
	}
	return 0;
}