#ifndef INTERPOLATORIMPL_H_
#define INTERPOLATORIMPL_H_
#include <vector>


#include <math.h>//For  sqrt

class InterpolatorImpl {
public:
	InterpolatorImpl();
	~InterpolatorImpl();
	InterpolatorImpl(double H, double a, double b,int kvadrantX, int kvadrantY, double startX, double startY);
	void setAllNewParametrs(double H, double a, double b,int kvadrantX, int kvadrantY, double startX, double startY);
	void getNextPoint(double *outX, double *outY );

private:
	int H;
	double a;
	double b;
	double k;

	double X;
	double Y;

	double U;

	int kvadrantX;
	int kvadrantY;
};

#endif /* INTERPOLATORIMPL_H_ */
