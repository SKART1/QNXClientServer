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

#include "../ServPrj/includes/CommonStructs.hpp"
#include "../DebugPrint.hpp"


/*-----------------------------------------------------------------------*/
int ParseServerInfoFile(std::string pathToInfoFile, char * serverNodeName,  pid_t *servPID, int *servCHID){
	FILE *filePointer = NULL;

	int result=0;


	if((filePointer=fopen(pathToInfoFile.c_str(),"r"))==NULL){
		DEBUG_CRITICALL_PRINT("ERROR", "[ERROR]: Can not server info file to "<<pathToInfoFile.c_str());
		return -1;
	}
	else{
		result=result+fscanf(filePointer, "SERVER_NODE_NAME: %s\n", serverNodeName);
		result=result+fscanf(filePointer, "SERVER_PID: %d\n", servPID);
		result=result+fscanf(filePointer, "CHID_FOR_CLIENT: %d\n", servCHID);
	}
	fclose(filePointer);
	if(result!=3){
		DEBUG_CRITICALL_PRINT("ERROR", "Wrong server info format");
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

#ifdef DEBUG_PRINT_ALLOWED
	const char separator    = ' ';
	const int numWidth      = 8;
#endif

	std::string pathTemp;

	TaskCommonStruct taskStruct;

	TaskResultCommonStruct taskResultStruct;

	iov_t iovSend;
	iov_t iovReceive[2];

	pathTemp="serv.serv";
	if(argc>1){
		pathTemp=std::string(argv[1]);
	}

	if(ParseServerInfoFile(pathTemp, serverNodeName,  &servPID, &servCHID)==-1){
		return -1;
	};

	if((nd=netmgr_strtond(serverNodeName, NULL))==-1){
		std::cerr<<"[ERROR]: Can not resolve QNet host "<<serverNodeName<<std::endl;
		return -1;
	}

	if ((coid = ConnectAttach(nd, servPID, servCHID,	NULL, NULL))==-1) {
		std::cerr << "[ERROR]: " << errno << "can not attach from client to server channel because of:"<< strerror(errno);
		return -1;
	}
	
	unsigned int totalNumberOfDots=atoi(argv[4]);



	taskStruct.H=1;
	taskStruct.a=atof(argv[2]);
	taskStruct.b=atof(argv[3]);
	if(taskStruct.a<taskStruct.b){
		double temp=taskStruct.b;
		taskStruct.b=taskStruct.a;
		taskStruct.a=temp;
	}
	taskStruct.kvadrantX=+1;
	taskStruct.kvadrantY=-1;
	taskStruct.startX=0;
	taskStruct.startY=taskStruct.b;
	taskStruct.totalNumberOfDots=totalNumberOfDots;
	taskStruct.portionSize=atoi(argv[5]);;
	taskStruct.exceedsInNanosecds=100000000000LL;
	taskStruct.taskID=0;




	taskResultStruct.taskResultPairOfDots=new TaskResultPairOfDots[taskStruct.totalNumberOfDots];


	SETIOV(&iovSend, &(taskStruct), sizeof(TaskCommonStruct));

	SETIOV(iovReceive+0, &(taskResultStruct.taskResultCommonStructHeader), sizeof(TaskResultCommonStructHeader));
	SETIOV(iovReceive+1, &taskResultStruct.taskResultPairOfDots[0], taskStruct.portionSize*sizeof(TaskResultPairOfDots));

	if(MsgSendv(coid,&iovSend, 1, iovReceive, 2)==-1){
		perror("[ERROR]: MsgSend: ");
		goto deinit;

	};
	switch(taskResultStruct.taskResultCommonStructHeader.serverToClientAnswers){
		case QUEU_IS_FULL:
			DEBUG_CRITICALL_PRINT("ERROR", "QUEU IS FULL");
			break;
		case NO_SUCH_TASK:
			DEBUG_CRITICALL_PRINT("ERROR", "NO_SUCH_TASK");
			break;
		case PREVIOUS_SUBTASK_HAVE_NOT_BEEN_DONE_YET:
			DEBUG_CRITICALL_PRINT("ERROR", "PREVIOUS_SUBTASK_HAVE_NOT_BEEN_DONE_YET");
			break;
		default:
			DEBUG_PRINT("INFO", "Answer is: "<<taskResultStruct.taskResultCommonStructHeader.serverToClientAnswers);
			break;
	}

#ifdef DEBUG_PRINT_ALLOWED
	for(unsigned int i=0; i<taskStruct.portionSize; i++){
		std::cerr <<" Dot:"<<std::left<<std::fixed << std::setprecision(3) << std::setw(numWidth) << std::setfill(separator) <<i;
		std::cerr <<" X:"<<std::left<<std::fixed << std::setprecision(3) << std::setw(numWidth) << std::setfill(separator) <<taskResultStruct.taskResultPairOfDots[i].xResult;
		std::cerr <<" Y:"<< std::left<< std::fixed << std::setprecision(3) << std::setw(numWidth) << std::setfill(separator) <<taskResultStruct.taskResultPairOfDots[i].yResult;
		std::cerr << std::endl;
	}
#endif

	unsigned int pairsDone;
	pairsDone=taskStruct.portionSize;
	taskStruct.taskID=taskResultStruct.taskResultCommonStructHeader.taskID;
	while(pairsDone<totalNumberOfDots){
		taskStruct.offsetOfWantedDots=pairsDone;
		taskStruct.numberOfWantedDots=taskStruct.portionSize;

		SETIOV(&iovSend, &(taskStruct), sizeof(TaskCommonStruct));

		SETIOV(iovReceive+0, &(taskResultStruct.taskResultCommonStructHeader), sizeof(TaskResultCommonStructHeader));
		SETIOV(iovReceive+1, &taskResultStruct.taskResultPairOfDots[pairsDone], taskStruct.portionSize*sizeof(TaskResultPairOfDots));

		MsgSendv(coid,&iovSend, 1, iovReceive, 2);

#ifdef DEBUG_PRINT_ALLOWED
		for(unsigned int i=taskResultStruct.taskResultCommonStructHeader.offsetOfResults; i<(taskResultStruct.taskResultCommonStructHeader.offsetOfResults+taskResultStruct.taskResultCommonStructHeader.numberOfDotsInCurrentPortion); i++){
			std::cerr <<" Dot:"<<std::left<<std::fixed << std::setprecision(3) << std::setw(numWidth) << std::setfill(separator) <<i;
			std::cerr <<" X:"<<std::left<<std::fixed << std::setprecision(3) << std::setw(numWidth) << std::setfill(separator) <<taskResultStruct.taskResultPairOfDots[i].xResult;
			std::cerr <<" Y:"<< std::left<< std::fixed << std::setprecision(3) << std::setw(numWidth) << std::setfill(separator) <<taskResultStruct.taskResultPairOfDots[i].yResult;
			std::cerr<<std::endl;
		}
#endif
		pairsDone=pairsDone+taskResultStruct.taskResultCommonStructHeader.numberOfDotsInCurrentPortion;
	}

	DEBUG_PRINT("INFO", "Finished");

deinit:
	delete[] taskResultStruct.taskResultPairOfDots;
	return EXIT_SUCCESS;
}
/*-----------------------------------------------------------------------*/
