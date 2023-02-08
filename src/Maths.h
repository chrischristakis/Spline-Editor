#ifndef MATHS_H
#define MATHS_H
#include <cmath>

/* Header only implementation where maths and util functions will be for this project. */

struct Point {
	float x, y;
	Point(float x, float y): x(x), y(y) {}
};

// Returns the value lying on the cubic bezier curve
// t:[0, 1]
double cubic_bezier(float a, float b, float c1, float c2, float t) {
	return pow(1 - t, 3) * a + 3 * (1 - t) * (1 - t) * t * c1 +
		3 * (1 - t) * t * t * c2 + pow(t, 3) * b;
}

#endif