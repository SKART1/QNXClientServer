#include <cstdlib>
#include <iostream>
#include <unistd.h>

#include <sys/neutrino.h>
#include <sys/netmgr.h>

#include <stdio.h>  //For cerr cerr and so on
#include <errno.h>	//For errno
#include <string.h>  //For strerror

#include <iomanip>

#include "../ServerPrj/includes/CommonStructs.hpp"


int main(int argc, char *argv[]) {
	sleep(2); //Let server start and be prepared!
	FILE *fp;
	fp=fopen("serv.serv", "r");

	pid_t servPID;
	int servCHID;
	char dummy[100];
	fscanf(fp,"%s", &dummy);
	fscanf(fp,"%d", &servPID);
	fscanf(fp,"%s", &dummy);
	fscanf(fp,"%d", &servPID);
	fscanf(fp,"%s", &dummy);
	fscanf(fp,"%d", &servCHID);

	//std::cerr<<servPID<<servCHID<<std::endl;


	int coid=-1;
	if ((coid = ConnectAttach(0, servPID, servCHID,	NULL, NULL))==-1) {
		std::cerr << "[ERROR]: " << errno << "can not attach from client to server channel because of:"<< strerror(errno);
		return NULL;
	}
	
	int totalNumberOfDots=100;


	TaskCommonStruct taskStruct;
	taskStruct.H=1;
	taskStruct.a=200;
	taskStruct.b=100;
	taskStruct.kvadrantX=+1;
	taskStruct.kvadrantY=-1;
	taskStruct.startX=0;
	taskStruct.startY=taskStruct.b;
	taskStruct.totalNumberOfDots=totalNumberOfDots;
	taskStruct.portionSize=10;
	taskStruct.exceedsInNanosecds=100000000000LL;



	taskStruct.taskID=0;
	//taskStruct.rcvid=-1;


	double buff[100];

	TaskResultCommonStruct taskResultStruct;
	taskResultStruct.taskResultPairOfDots=new TaskResultPairOfDots[taskStruct.totalNumberOfDots];
	iov_t iovSend;
	iov_t iovReceive[2];

	SETIOV(&iovSend, &(taskStruct), sizeof(TaskCommonStruct));

	SETIOV(iovReceive+0, &(taskResultStruct.taskResultCommonStructHeader), sizeof(TaskResultCommonStructHeader));
	SETIOV(iovReceive+1, &taskResultStruct.taskResultPairOfDots[0], taskStruct.portionSize*sizeof(TaskResultPairOfDots));

	MsgSendv(coid,&iovSend, 1, iovReceive, 2);

	for(unsigned int i=0; i<taskStruct.portionSize; i++){
		//std::cerr << std::left<<std::fixed << std::setprecision(3) << std::setw(numWidth) << std::setfill(separator) << taskResultStruct.xVector.back();
		//std::cerr << std::left<< std::fixed << std::setprecision(3) << std::setw(numWidth) << std::setfill(separator) << taskResultStruct.yVector.back();
		//std::cerr << std::endl;
		std::cerr<<"Dot "<<i<< " X: "<<taskResultStruct.taskResultPairOfDots[i].xResult<<" Y: "<<taskResultStruct.taskResultPairOfDots[i].yResult<<std::endl;
		//taskResultStruct.xVector.pop_back();
		//taskResultStruct.yVector.pop_back();
	}


	unsigned int pairsDone=taskStruct.portionSize;
	taskStruct.taskID=taskResultStruct.taskResultCommonStructHeader.taskID;
	while(pairsDone<totalNumberOfDots){
		taskStruct.offsetOfWantedDots=pairsDone;
		taskStruct.numberOfWantedDots=taskStruct.portionSize;

		SETIOV(&iovSend, &(taskStruct), sizeof(TaskCommonStruct));

		SETIOV(iovReceive+0, &(taskResultStruct.taskResultCommonStructHeader), sizeof(TaskResultCommonStructHeader));
		SETIOV(iovReceive+1, &taskResultStruct.taskResultPairOfDots[pairsDone], taskStruct.portionSize*sizeof(TaskResultPairOfDots));

		MsgSendv(coid,&iovSend, 1, iovReceive, 2);

		for(unsigned int i=taskResultStruct.taskResultCommonStructHeader.offsetOfResults; i<(taskResultStruct.taskResultCommonStructHeader.offsetOfResults+taskResultStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion); i++){
			//std::cerr << std::left<<std::fixed << std::setprecision(3) << std::setw(numWidth) << std::setfill(separator) << taskResultStruct.xVector.back();
			//std::cerr << std::left<< std::fixed << std::setprecision(3) << std::setw(numWidth) << std::setfill(separator) << taskResultStruct.yVector.back();
			//std::cerr << std::endl;
			std::cerr<<"Dot "<<i<<" X: "<<taskResultStruct.taskResultPairOfDots[i].xResult<<" Y: "<<taskResultStruct.taskResultPairOfDots[i].yResult<<std::endl;
			//taskResultStruct.xVector.pop_back();
			//taskResultStruct.yVector.pop_back();
		}


		pairsDone=pairsDone+taskResultStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion;

		const char separator    = ' ';
		const int numWidth      = 10;
		//std::cerr<<taskResultStruct.xVector.size()<<std::endl;



	}
	delete[] taskResultStruct.taskResultPairOfDots;

	std::cerr<<"Finished"<<std::endl;

	return EXIT_SUCCESS;
}
