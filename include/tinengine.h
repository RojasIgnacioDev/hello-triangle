#pragma once
struct Color {
	float r, g, b, a;
	Color(float red, float green, float blue, float alpha) :
		r(red), g(green), b(blue), a(alpha) {};
};

struct Vertex {
	float x, y, z;
	Color color;
	Vertex(float xPosition, float yPosition, float zPosition, Color color) :
		x(xPosition), y(yPosition), z(zPosition), color(color) {};
};