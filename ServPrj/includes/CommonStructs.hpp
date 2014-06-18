#ifndef COMMON_STRUCTS_HPP_
#define COMMON_STRUCTS_HPP_

#define ID_MEANS_NEW_TASK 0

/*-----------------------------------------------------------*/
/*Task from client*/
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
/*----------------------------------------------------*/


/*----------------------------------------------------*/
/*Result to client*/
typedef enum{
	OK,
	TASK_IS_PARTICALLY_DONE,
	TASK_IS_DONE,

	QUEU_IS_FULL,
	NO_SUCH_TASK,

	PREVIOUS_SUBTASK_HAVE_NOT_BEEN_DONE_YET,

	TASK_EXCEEDED,
} ServerToClientAnswers;

typedef struct{
	int taskID;

	unsigned long offsetOfResults;
	unsigned long numberOfDotsInCurrentPortion;

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
/*----------------------------------------------------*/


/*----------------------------------------------------*/
/*Viewer interest*/
typedef struct{
	struct{
		unsigned long offsetOfWantedDots;
		unsigned long numberOfWantedDots;
	};
	int taskID;
}ViewerTaskInterest;
/*----------------------------------------------------*/

/*----------------------------------------------------*/
/*Result to viewer*/
typedef enum{
	VIEWER_OK,
	VIEWER_TASK_IS_PARTICALLY_DONE,
	VIEWER_NO_SUCH_TASK,

	VIEWER_TASK_IS_NOT_DONE,

} ServerToViewerAnswer;


typedef struct{
	ServerToViewerAnswer answer;
	unsigned long totalNumberOfDots;
	TaskResultCommonStruct taskResultCommonStruct;
} ViewerResultCommonStruct;


/*----------------------------------------------------*/

#endif /* COMMON_STRUCTS_HPP_ */
