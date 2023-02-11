#ifndef MATHS_H
#define MATHS_H
#include <cmath>

/* Header only implementation where maths and util functions will be for this project. */

struct Node;

struct Point {
	float x, y;

	void set(float x, float y) {
		this->x = x;
		this->y = y;
	}

	void move(float x, float y) {
		this->x += x;
		this->y += y;
	}

	Point(float x, float y): x(x), y(y) {}
	Point() = default;
};

struct Handle : public Point {
	Node *parent = nullptr;

	enum class Type {
		HANDLE1,
		HANDLE2
	} type;

	Handle(float x, float y, Node *parent): Point(x, y), parent(parent) {}
};

struct Node : public Point {
	bool hasHandle1 = false, hasHandle2 = false;
	Handle handle1, handle2;

	Node(float x, float y): Point(x, y), handle1(0, 0, this), handle2(0, 0, this) {}

	void addHandle1(float x, float y) {
		handle1 = Handle(x, y, this);
		handle1.type = Handle::Type::HANDLE1;
		hasHandle1 = true;
	}

	void addHandle2(float x, float y) {
		handle2 = Handle(x, y, this);
		handle2.type = Handle::Type::HANDLE2;
		hasHandle2 = true;
	}
};

// Returns the value lying on the cubic bezier curve
// t:[0, 1]
double cubic_bezier(float a, float b, float c1, float c2, float t) {
	return pow(1 - t, 3) * a + 3 * (1 - t) * (1 - t) * t * c1 +
		3 * (1 - t) * t * t * c2 + pow(t, 3) * b;
}

double euclid_dist(const Point &p1, const Point &p2) {
	return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
}

// To find a colinear point we literally just negate the x and y components between the root and point, then 
// offset the root by these negated distances.
Point get_colinear_point(const Point &root, const Point &point) {
	float px = point.x - root.x;
	float py = point.y - root.y;
	return Point(root.x-px, root.y-py);
}

#endif