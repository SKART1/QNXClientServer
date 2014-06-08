#include <cstdlib>
#include <iostream>
#include <unistd.h>

#include <sys/neutrino.h>
#include <sys/netmgr.h>

#include <stdio.h>  //For cerr cerr and so on
#include <errno.h>	//For errno
#include <string.h>  //For strerror
#include <stdlib.h>
#include <iomanip>

#include "../ServerPrj/includes/CommonStructs.hpp"



/*-----------------------------------------------------------------------*/
int ParseServerInfoFile(std::string pathToInfoFile, char * serverNodeName,  pid_t *servPID, int *servCHID){
	FILE *filePointer = NULL;

	int result=0;


	if((filePointer=fopen(pathToInfoFile.c_str(),"r"))==NULL){
		std::cerr<<"[ERROR]: Can not server info file to "<<pathToInfoFile.c_str()<<std::endl;
		return -1;
	}
	else{
		result=result+fscanf(filePointer, "SERVER_NODE_NAME: %s\n", serverNodeName);
		result=result+fscanf(filePointer, "SERVER_PID: %d\n", servPID);
		result=result+fscanf(filePointer, "CHID_FOR_CLIENT: %d\n", servCHID);
	}
	fclose(filePointer);
	if(result!=3){
		std::cerr<<"[ERROR]: Wrong server info format"<<std::endl;
		return -1;
	}
	return 0;
}
/*-----------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
int main(int argc, char *argv[]) {
	sleep(2); //Let server start and be prepared!

	pid_t servPID;
	int servCHID;
	char serverNodeName[250];

	int nd=-1;
	int coid=-1;

	const char separator    = ' ';
	const int numWidth      = 8;


	if(argc<2){
		std::cerr<<"[INFO]: No path given. Using default serv.serv"<<std::endl;
		ParseServerInfoFile("serv.serv", serverNodeName,  &servPID, &servCHID);
	}
	else{
		ParseServerInfoFile(argv[1], serverNodeName,  &servPID, &servCHID);
	}




	if((nd=netmgr_strtond(serverNodeName, NULL))==-1){
		std::cerr<<"[ERROR]: Can not resolve QNet host "<<serverNodeName<<std::endl;
	}



	if ((coid = ConnectAttach(nd, servPID, servCHID,	NULL, NULL))==-1) {
		std::cerr << "[ERROR]: " << errno << "can not attach from client to server channel because of:"<< strerror(errno);
		return NULL;
	}
	
	unsigned int totalNumberOfDots=atoi(argv[4]);


	TaskCommonStruct taskStruct;
	taskStruct.H=1;
	taskStruct.a=atof(argv[2]);
	taskStruct.b=atof(argv[3]);
	if(taskStruct.a<taskStruct.b){
		double temp=taskStruct.b;
		taskStruct.b=taskStruct.a;
		taskStruct.a=taskStruct.b;
	}
	taskStruct.kvadrantX=+1;
	taskStruct.kvadrantY=-1;
	taskStruct.startX=0;
	taskStruct.startY=taskStruct.b;
	taskStruct.totalNumberOfDots=totalNumberOfDots;
	taskStruct.portionSize=atoi(argv[5]);;
	taskStruct.exceedsInNanosecds=100000000000LL;
	taskStruct.taskID=0;





	TaskResultCommonStruct taskResultStruct;
	taskResultStruct.taskResultPairOfDots=new TaskResultPairOfDots[taskStruct.totalNumberOfDots];
	iov_t iovSend;
	iov_t iovReceive[2];

	SETIOV(&iovSend, &(taskStruct), sizeof(TaskCommonStruct));

	SETIOV(iovReceive+0, &(taskResultStruct.taskResultCommonStructHeader), sizeof(TaskResultCommonStructHeader));
	SETIOV(iovReceive+1, &taskResultStruct.taskResultPairOfDots[0], taskStruct.portionSize*sizeof(TaskResultPairOfDots));

	MsgSendv(coid,&iovSend, 1, iovReceive, 2);
	switch(taskResultStruct.taskResultCommonStructHeader.serverToClientAnswers){
		case QUEU_IS_FULL:
			std::cerr<<"QUEU IS FULL"<<std::endl;
			break;
		case NO_SUCH_TASK:
			std::cerr<<"NO_SUCH_TASK"<<std::endl;
			break;
		case PREVIOUS_SUBTASK_HAVE_NOT_BEEN_DONE_YET:
			std::cerr<<"PREVIOUS_SUBTASK_HAVE_NOT_BEEN_DONE_YET"<<std::endl;
			break;
		default:
			std::cerr<<"Answer is: "<<taskResultStruct.taskResultCommonStructHeader.serverToClientAnswers<<std::endl;
			break;
	}

	for(unsigned int i=0; i<taskStruct.portionSize; i++){
		std::cerr <<" Dot:"<<std::left<<std::fixed << std::setprecision(3) << std::setw(numWidth) << std::setfill(separator) <<i;
		std::cerr <<" X:"<<std::left<<std::fixed << std::setprecision(3) << std::setw(numWidth) << std::setfill(separator) <<taskResultStruct.taskResultPairOfDots[i].xResult;
		std::cerr <<" Y:"<< std::left<< std::fixed << std::setprecision(3) << std::setw(numWidth) << std::setfill(separator) <<taskResultStruct.taskResultPairOfDots[i].yResult;
		std::cerr << std::endl;
		//std::cerr<<"Dot "<<i<< " X: "<<taskResultStruct.taskResultPairOfDots[i].xResult<<" Y: "<<taskResultStruct.taskResultPairOfDots[i].yResult<<std::endl;
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
			std::cerr <<" Dot:"<<std::left<<std::fixed << std::setprecision(3) << std::setw(numWidth) << std::setfill(separator) <<i;
			std::cerr <<" X:"<<std::left<<std::fixed << std::setprecision(3) << std::setw(numWidth) << std::setfill(separator) <<taskResultStruct.taskResultPairOfDots[i].xResult;
			std::cerr <<" Y:"<< std::left<< std::fixed << std::setprecision(3) << std::setw(numWidth) << std::setfill(separator) <<taskResultStruct.taskResultPairOfDots[i].yResult;
			std::cerr<<std::endl;
			//std::cerr<<"Dot "<<i<<" X: "<<taskResultStruct.taskResultPairOfDots[i].xResult<<" Y: "<<taskResultStruct.taskResultPairOfDots[i].yResult<<std::endl;
			//taskResultStruct.xVector.pop_back();
			//taskResultStruct.yVector.pop_back();
		}


		pairsDone=pairsDone+taskResultStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion;


		//std::cerr<<taskResultStruct.xVector.size()<<std::endl;



	}
	delete[] taskResultStruct.taskResultPairOfDots;

	std::cerr<<"Finished"<<std::endl;

	return EXIT_SUCCESS;
}
/*-----------------------------------------------------------------------*/
