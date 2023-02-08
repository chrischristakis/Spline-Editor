#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/GLU.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Maths.h"

const int LINE_SEGMENTS = 200;  // Line segments per bezier curve in spline

int main() {
	int width = 1000, height = 1000;

	if (!glfwInit()) {
		std::cout << "Failed to initialize GLFW" << std::endl;
		return -1;
	}
	GLFWwindow *window = glfwCreateWindow(width, height, "Assignment 3", NULL, NULL);

	if (!window) {
		std::cout << "Failed to initialize window" << std::endl;
		return -1;
	}

	glfwMakeContextCurrent(window);

	if (glewInit() != GLEW_OK) {
		std::cout << "Failed to initialize GLEW" << std::endl;
		return -1;
	}

	glViewport(0, 0, width, height);
	
	Point p1(-0.75f, -0.5f);
	Point p2(0.75f,  0.0f);
	Point c1(-0.2f, -0.85f);
	Point c2(0.65f, -0.6f);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		glClearColor(0.06, 0.06, 0.06, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		glLineWidth(2.0f);
		glBegin(GL_LINE_STRIP);
			glColor3f(1, 1, 1);
			for (int i = 0; i < LINE_SEGMENTS; i++) {
				float x = cubic_bezier(p1.x, p2.x, c1.x, c2.x, i/(float)LINE_SEGMENTS);
				float y = cubic_bezier(p1.y, p2.y, c1.y, c2.y, i/(float)LINE_SEGMENTS);
				glVertex2f(x, y);
			}
			glVertex2f(p2.x, p2.y);  // We need to complete the line by just connecting to the last point.
		glEnd();

		glPointSize(10.0f);
		glBegin(GL_POINTS);
			glColor3f(0, 0.3f, 1);
			glVertex2f(p1.x, p1.y);
			glVertex2f(p2.x, p2.y);
			glColor3f(1, 0.2f, 0);
			glVertex2f(c1.x, c1.y);
			glVertex2f(c2.x, c2.y);
		glEnd();

		glfwSwapBuffers(window);
	}
	glfwTerminate();

	return 1;
}