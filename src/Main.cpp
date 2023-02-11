#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/GLU.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include "Maths.h"

const int LINE_SEGMENTS = 200;  // Line segments per bezier curve in spline
const float POINT_WIDTH = 11.0f;

int width = 1000, height = 1000;
std::vector<Node*> nodes;

double mx = 0, my = 0; // Mouse position

// For user interaction
Node *grabbedNode = nullptr;
Handle *grabbedHandle = nullptr;

void addNode(float x, float y) {
	Node *node = new Node(x, y);

	if (nodes.size() == 0) {
		node->addHandle1(x, y + 50);
		nodes.push_back(node);
		return;
	}
	
	// If we only have 1 node so far, it already has a handle 1. We don't want to overwrite that.
	if (nodes.size() != 1) {
		Node &prev = *nodes.back();
		prev.addHandle1(prev.x, prev.y - 50);
	}
	node->addHandle2(x, y + 50);

	nodes.push_back(node);
}

// Move a point to a point (locX, locY) and constrain it to the window.
void moveAndBind(Point &point, double locX, double locY) {
	point.x = locX;
	point.y = locY;

	// Bind points to the window so they don't go outside the window (With 5px of padding)
	point.x = std::max(5.0f, std::min(point.x, (float)width-5));
	point.y = std::max(5.0f, std::min(point.y, (float)height-5));
}

/* GLFW CALLBACKS */
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	mx = xpos;
	my = height - ypos; // Since window is using a left handed system, translate it to our opengl right handed system.
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		for (int i = 0; i < nodes.size(); i++) {
			Node *current = nodes[i];

			// If our mouse is bounded by the node, then we are 'grabbing' it until we release our mouse.
			if (mx >= current->x - POINT_WIDTH && mx <= current->x + POINT_WIDTH &&
				my >= current->y - POINT_WIDTH && my <= current->y + POINT_WIDTH) {
				grabbedNode = current;
				break;  // Break so we dont check other handles/nodes if we're already grabbing one!
			}

			// Also check handles
			if (current->hasHandle1)
				if (mx >= current->handle1.x - POINT_WIDTH && mx <= current->handle1.x + POINT_WIDTH &&
					my >= current->handle1.y - POINT_WIDTH && my <= current->handle1.y + POINT_WIDTH) {
					grabbedHandle = &(current->handle1);
					break;
				}
			if (current->hasHandle2)
				if (mx >= current->handle2.x - POINT_WIDTH && mx <= current->handle2.x + POINT_WIDTH &&
					my >= current->handle2.y - POINT_WIDTH && my <= current->handle2.y + POINT_WIDTH) {
					grabbedHandle = &(current->handle2);
					break;
				}
		}
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		grabbedNode = nullptr;
		grabbedHandle = nullptr;
	}

}
/* ------------- */

void update() {
	// If a node is grabbed, set it equal to our mouse's position
	if (grabbedNode) {
		float prevX = grabbedNode->x;
		float prevY = grabbedNode->y;
		moveAndBind(*grabbedNode, mx, my);

		// Offset our handles by the amount we offset our node.
		if (grabbedNode->hasHandle1)
			grabbedNode->handle1.move(grabbedNode->x - prevX, grabbedNode->y - prevY);
		if (grabbedNode->hasHandle2)
			grabbedNode->handle2.move(grabbedNode->x - prevX, grabbedNode->y - prevY);
	}
	else if (grabbedHandle) {
		moveAndBind(*grabbedHandle, mx, my);

		// Now, we just offset the other handle and make it colinear to this handle, mirrored at the node.
		if (grabbedHandle->type == Handle::Type::HANDLE1 && grabbedHandle->parent->hasHandle2) {
			Point p = get_colinear_point(*(grabbedHandle->parent), *grabbedHandle);
			grabbedHandle->parent->handle2.set(p.x, p.y);
		}
		if (grabbedHandle->type == Handle::Type::HANDLE2 && grabbedHandle->parent->hasHandle1) {
			Point p = get_colinear_point(*(grabbedHandle->parent), *grabbedHandle);
			grabbedHandle->parent->handle1.set(p.x, p.y);
		}
	}
}

int main() {

	if (!glfwInit()) {
		std::cout << "Failed to initialize GLFW" << std::endl;
		return -1;
	}
	GLFWwindow *window = glfwCreateWindow(width, height, "Spline creator", NULL, NULL);

	if (!window) {
		std::cout << "Failed to initialize window" << std::endl;
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);

	if (glewInit() != GLEW_OK) {
		std::cout << "Failed to initialize GLEW" << std::endl;
		return -1;
	}

	glViewport(0, 0, width, height);
	glfwWindowHint(GLFW_SAMPLES, 4);  // 4x MSAA
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	addNode(width/4, height/4);
	addNode(width/2+width/4, height/4);
	addNode(width/2, height/2+height/4);
	addNode(width/2-width/4, height/2+height/4);

	// Ortho projection matrix with screen bounds.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, 0, height, -1, 1);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// Handle game updates before rendering
		update();

		// RENDER HERE
		glClearColor(0.04, 0.04, 0.05, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		/* DRAW BEZIER LINES */
		glLineWidth(4.0f);
		glBegin(GL_LINE_STRIP);
			glColor3f(1, 1, 1);

			// Loop through each node and draw the bezier curve between it and the next node, using their handles.
			for (int i = 0; i < nodes.size() - 1; i++) {
				Node &current = *nodes[i];
				Node &next = *nodes[i + 1];
				
				// Draw line between nodes
				for (int i = 0; i < LINE_SEGMENTS; i++) {
					float x = cubic_bezier(current.x, next.x, current.handle1.x, next.handle2.x,
						i / (float)LINE_SEGMENTS);
					float y = cubic_bezier(current.y, next.y, current.handle1.y, next.handle2.y,
						i / (float)LINE_SEGMENTS);

					glVertex2f(x, y);
				}
			}
			// We need to complete the line by just connecting it to the last node.
			if(!nodes.empty())
				glVertex2f(nodes.back()->x, nodes.back()->y);
		glEnd();

		/* DRAW HANDLE LINES */
		glEnable(GL_LINE_STIPPLE);
		glLineWidth(2.0f);
		glLineStipple(8, 0xAAAA);  // Used to dot lines
		glBegin(GL_LINES);
		glColor3f(0.4, 0.4, 0.4);
		for (int i = 0; i < nodes.size(); i++) {
			Node &current = *nodes[i];
			if (current.hasHandle1) {
				glVertex2f(current.handle1.x, current.handle1.y);
				glVertex2f(current.x, current.y);
			}
			if (current.hasHandle2) {
				glVertex2f(current.handle2.x, current.handle2.y);
				glVertex2f(current.x, current.y);
			}
		}
		glEnd();
		glDisable(GL_LINE_STIPPLE);

		/* DRAW NODES */
		glPointSize(POINT_WIDTH);
		glBegin(GL_POINTS);
			glColor3ub(255, 97, 42);
			for (int i = 0; i < nodes.size(); i++) {
				Node &current = *nodes[i];
				glVertex2f(current.x, current.y);
			}
		glEnd();

		/* DRAW HANDLES (Seperate from nodes so they can be round using GL_POINT_SMOOTH) */
		glEnable(GL_POINT_SMOOTH);
		glBegin(GL_POINTS);
			glColor3f(1, 1, 1);
			for (int i = 0; i < nodes.size(); i++) {
				Node &current = *nodes[i];

				if (current.hasHandle1)
					glVertex2f(current.handle1.x, current.handle1.y);
				if (current.hasHandle2)
					glVertex2f(current.handle2.x, current.handle2.y);
			}
		glEnd();
		glDisable(GL_POINT_SMOOTH);

		glfwSwapBuffers(window);
	}
	glfwTerminate();

	return 1;
}