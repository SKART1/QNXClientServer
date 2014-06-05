#include "InterpolatorImpl.h"


InterpolatorImpl::InterpolatorImpl(){
};

InterpolatorImpl::InterpolatorImpl(double H, double a, double b,int kvadrantX, int kvadrantY, double startX, double startY) {
	this->H=H;
	this->a=a;
	this->k=b/a;

	U=0;

	this->kvadrantX=kvadrantX;
	this->kvadrantY=kvadrantY;

	X=startX;
	Y=startY;
}


void InterpolatorImpl::setAllNewParametrs(double H, double a, double b,int kvadrantX, int kvadrantY, double startX, double startY) {
	this->H=H;
	this->a=a;
	this->k=a/b;

	U=0;

	this->kvadrantX=kvadrantX;
	this->kvadrantY=kvadrantY;

	X=startX;
	Y=startY;
}



void InterpolatorImpl::getNextPoint(double *outX, double *outY) {
	//Evaluation
	double deltaX=(kvadrantX*H*Y)/(k*a);
	double deltaY=(kvadrantY*H*X)/a;
	U=U+2*(k*k*X*deltaX+Y*deltaY)+k*k*deltaX*deltaX+deltaY*deltaY;
	X=X+deltaX;
	Y=Y+deltaY;

	//Correction
	double psiX=0;
	double psiY=0;

	if(abs(U)>H){
		double R=sqrt((X*X+Y*Y));
		double psiR=-U/a;
		psiX=(psiR*X)/(k*R);
		psiY=(psiR*Y)/(R);
		U=U+2*(k*k*X*psiX+Y*psiY)+k*k*psiX*psiX+psiY*psiY;
		X=X+psiX;
		Y=Y+psiY;
	}
	*outX=X;
	*outY=Y;
}




InterpolatorImpl::~InterpolatorImpl() {
}




