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


	union{
		struct{
			unsigned long offsetOfWantedDots;
			unsigned long numberOfWantedDots;
		};
		struct{
			unsigned long totalNumberOfDots;
			unsigned long portionSize;
		};
	};

	unsigned int taskID;
} TaskCommonStruct;

typedef enum{
	OK,
	TASK_IS_PARTICALLY_DONE,
	TASK_IS_DONE,

	QUEU_IS_FULL,
	NO_SUCH_TASK,


	PREVIOUS_SUBTASK_HAVE_NOT_BEEN_DONE_YET,




	TASK_ECEEDED,
} ServerToClientAnswers;

typedef struct{
	int taskID;

	unsigned long offsetOfResults;
	unsigned long numberOfDotsEvaluatedInCurrentPortion;


	long long exceedsInNanosecds;

	bool taskExceeded;
	ServerToClientAnswers serverToClientAnswers;
} TaskResultCommonStructHeader;


typedef  struct{
	double xResult;
	double yResult;
	bool resultExceeded;
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
