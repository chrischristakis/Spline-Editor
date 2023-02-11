#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/GLU.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include "Maths.h"

GLFWwindow *window = nullptr;

const int LINE_SEGMENTS = 200;  // Line segments per bezier curve in spline
const float POINT_WIDTH = 11.0f;

int width = 1000, height = 1000;
std::vector<Node*> nodes;

float mx = 0, my = 0; // Mouse position

// For user interaction
Node *grabbedNode = nullptr;
Handle *grabbedHandle = nullptr;

void addNode(float x, float y) {
	Node *node = new Node(x, y);

	// If its our first node, we need to start with 1 handle.
	if (nodes.size() == 0) {
		node->addHandle1(x, y + 50);
		nodes.push_back(node);
		return;
	}
	
	// If we only have 1 node so far, we dont want to overwrite its handle1. Skip to else block.
	if (nodes.size() != 1) {
		double distToFront = euclid_dist(*node, *nodes[0]);
		double distToBack = euclid_dist(*node, *nodes.back());
		
		// Figure out which endpoint the new node is closer to, does it go to front or back of the list?
		if (distToFront <= distToBack) {
			// If we place at front, we need to give it a handle1 and give old front node a handle2.
			// Why? Because I chose to render each bezier iteration using handle1 as control 1 and handle2 as control 2,
			// so this 1->2 structure must be followed.
			node->addHandle1(x, y + 50);

			Node *prevEndpoint = nodes[0];
			
			// Make sure new handle is colinear to the old one!
			Point colinear = get_colinear_point(*prevEndpoint, prevEndpoint->handle1);
			prevEndpoint->addHandle2(colinear.x, colinear.y);

			// Now put it at the front of the list
			nodes.insert(nodes.begin(), node);
		}
		else {
			// If we place at back, we need to give old back a handle 1 and new node a handle2.
			node->addHandle2(x, y + 50);

			Node *prevEndpoint = nodes.back();
			Point colinear = get_colinear_point(*prevEndpoint, prevEndpoint->handle2);
			prevEndpoint->addHandle1(colinear.x, colinear.y);

			// Add to the back of the list instead!
			nodes.push_back(node);
		}
	}
	else {
		// We just make a new node since there's only 1 so far, and give it a handle2 to draw our first bezier.
		node->addHandle2(x, y + 50);
		nodes.push_back(node);
	}
}

// Move a point to a point (locX, locY) and constrain it to the window.
void moveAndBind(Point &point, float locX, float locY) {
	point.x = locX;
	point.y = locY;

	// Bind points to the window so they don't go outside the window (With 5px of padding)
	point.x = std::max(5.0f, std::min(point.x, (float)width-5));
	point.y = std::max(5.0f, std::min(point.y, (float)height-5));
}

void reset() {
	// Incase the user is holding a node while resetting, lets not use a hanging pointer.
	grabbedNode = nullptr;
	grabbedHandle = nullptr;

	for (Node *n : nodes)
		delete n;
	nodes.clear();
}

void update() {
	//KEY INPUT:
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		reset();
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	//VIRTUAL EVENTS:
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

// To be run each frame, actually render our spline here.
void render() {
	glClearColor(23/255.0f, 30/255.0f, 36/255.0f, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	// Don't render if there's nothing to render.
	if (nodes.size() == 0) {
		glfwSwapBuffers(window);
		return;
	}

	/* DRAW BEZIER LINES */
	glLineWidth(3.4f);
	glBegin(GL_LINE_STRIP);
	glColor3f(1, 1, 1);

	// Loop through each node and draw the bezier curve between it and the next node, using their handles.
	for (int i = 0; i < nodes.size() - 1; i++) {
		Node &current = *nodes[i];
		Node &next = *nodes[i + 1];

		// Draw line between nodes
		for (int i = 0; i < LINE_SEGMENTS; i++) {
			double x = cubic_bezier(current.x, next.x, current.handle1.x, next.handle2.x,
				i / (float)LINE_SEGMENTS);
			double y = cubic_bezier(current.y, next.y, current.handle1.y, next.handle2.y,
				i / (float)LINE_SEGMENTS);

			glVertex2f((float)x, (float)y);
		}
	}
	// We need to complete the line by just connecting it to the last node.
	if (!nodes.empty())
		glVertex2f(nodes.back()->x, nodes.back()->y);
	glEnd();

	/* DRAW HANDLE LINES */
	glEnable(GL_LINE_STIPPLE);
	glLineWidth(2.0f);
	glLineStipple(8, 0xAAAA);  // Used to dot lines
	glBegin(GL_LINES);
	glColor3f(0.4f, 0.4f, 0.4f);
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
	glColor3ub(54, 255, 221);
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

/* GLFW CALLBACKS */
void framebuffer_size_callback(GLFWwindow* window, int w, int h)
{
	width = w; height = h;

	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, 0, height, -1, 1);

	// Re-render on the new resolution.
	render();
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	mx = (float)xpos;
	my = (float)(height - ypos); // Since window is using a left handed system, translate it to our opengl right handed system.
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {

		//First, check if our mouse intersects any of the nodes/handles.
		bool noNodesTouched = true;
		for (int i = 0; i < nodes.size(); i++) {
			Node *current = nodes[i];

			// If our mouse is bounded by the node, then we are 'grabbing' it until we release our mouse.
			if (mx >= current->x - POINT_WIDTH && mx <= current->x + POINT_WIDTH &&
				my >= current->y - POINT_WIDTH && my <= current->y + POINT_WIDTH) {
				grabbedNode = current;
				noNodesTouched = false;
				break;  // Break so we dont check other handles/nodes if we're already grabbing one!
			}

			// Also check handles
			if (current->hasHandle1)
				if (mx >= current->handle1.x - POINT_WIDTH && mx <= current->handle1.x + POINT_WIDTH &&
					my >= current->handle1.y - POINT_WIDTH && my <= current->handle1.y + POINT_WIDTH) {
					grabbedHandle = &(current->handle1);
					noNodesTouched = false;
					break;
				}
			if (current->hasHandle2)
				if (mx >= current->handle2.x - POINT_WIDTH && mx <= current->handle2.x + POINT_WIDTH &&
					my >= current->handle2.y - POINT_WIDTH && my <= current->handle2.y + POINT_WIDTH) {
					grabbedHandle = &(current->handle2);
					noNodesTouched = false;
					break;
				}
		}

		// If no nodes interacted with, then lets make one.
		if (noNodesTouched)
			addNode(mx, my);
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		grabbedNode = nullptr;
		grabbedHandle = nullptr;
	}

}
/* ------------- */

int main(int argc, char **argv) {

	// If the user specifies window dimensions, use them
	if (argc == 3) {
		width = std::atoi(argv[1]);
		height = std::atoi(argv[2]);

		if (width <= 0 || height <= 0) {
			std::cout << "Bad arguments, provide width, height as integers" << std::endl;
			return -1;
		}
	}

	if (!glfwInit()) {
		std::cout << "Failed to initialize GLFW" << std::endl;
		return -1;
	}
	window = glfwCreateWindow(width, height, "Spline creator", NULL, NULL);

	if (!window) {
		std::cout << "Failed to initialize window" << std::endl;
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

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

	// Ortho projection matrix with screen bounds.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, 0, height, -1, 1);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// Handle game updates before rendering
		update();
		render();	
	}

	// Cleanup
	reset();
	glfwTerminate();

	return 1;
}