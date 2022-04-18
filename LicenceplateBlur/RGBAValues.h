#ifndef RGBAVALUES_H
#define RGBAVALUES_H

struct RGBAValues
{
	double r, g, b, a;
	RGBAValues(double v) : r(v), g(v), b(v), a(v) {}

	RGBAValues() : r(0), g(0), b(0), a(0) {}
};

#endif // !RGBAVALUES_H