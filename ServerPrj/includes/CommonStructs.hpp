#ifndef COMMON_STRUCTS_HPP_
#define COMMON_STRUCTS_HPP_

#include <vector>

typedef struct{
	double H;
	double a;
	double b;
	double kvadrantX;
	double kvadrantY;
	double startX;
	double startY;
	long long exceedsInNanosecds;
	long numberOfNeededPoints;
	int taskID;
} TaskCommonStruct;

typedef struct{
	int taskID;
	int numberOfDotsCoordinates;

	long long exceedsInNanosecds;

	bool taskExceeded;
	bool serverBusy;
} TaskResultCommonStructHeader;


typedef  struct{
	double xResult;
	double yResult;
} TaskResultPairOfDots;

typedef struct{
	TaskResultCommonStructHeader taskResultCommonStructHeader;
	TaskResultPairOfDots *taskResultPairOfDots;
} TaskResultCommonStruct;


typedef struct{
	TaskResultCommonStruct taskResultCommonStruct;
	bool exist;
} ViewerResultCommonStruct;



#endif /* COMMON_STRUCTS_HPP_ */
