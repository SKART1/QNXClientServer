/*
 * Slave.cpp
 *
 *  Created on: 29.04.2014
 *      Author: Art
 */




#include "Slave.hpp"


void * Slave(void *argSlaveParam) {
	ArgSlaveStruct argSlave=*(ArgSlaveStruct *) argSlaveParam;
	delete (ArgSlaveStruct *)argSlaveParam;
	int chidTasks = -1;
	int chidResults = -1;



	int nd=netmgr_strtond(argSlave.serverNodeName, NULL);

	if ((chidTasks = ConnectAttach(nd, argSlave.pid, argSlave.chidTasks,	NULL, NULL))==-1) {
		perror("[ERROR]:can not attach channel because of:");
		return NULL;
	}

	if ((chidResults = ConnectAttach(nd, argSlave.pid, argSlave.chidResults,	NULL, NULL))==-1) {
		perror("[ERROR]:can not attach channel because of:");
		return NULL;
	}



	InterpolatorImpl interpolatorImpl;

	TaskCommonStruct taskStruct;

	TaskResultCommonStruct taskResultCommonStruct;

	while (true) {
		MsgSend(chidTasks,NULL,NULL, &taskStruct, sizeof(TaskCommonStruct));
		std::cerr<<"[SLAVE]: Task received"<<std::endl;
		interpolatorImpl.setAllNewParametrs(taskStruct.H, taskStruct.a, taskStruct.b,taskStruct.kvadrantX,taskStruct.kvadrantY,taskStruct.startX, taskStruct.startY);


		taskResultCommonStruct.taskResultCommonStructHeader.taskID=taskStruct.taskID;
		taskResultCommonStruct.taskResultPairOfDots=new TaskResultPairOfDots[taskStruct.totalNumberOfDots];

		taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults=0;
		//taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsCoordinatesEvaluatedInCurrentTask=taskStruct.currentWantedPortionOfDots;



		while((taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults+taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion)<taskStruct.totalNumberOfDots){

			for(unsigned long i=taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults;i<(taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults+taskStruct.portionSize) &&	i<taskStruct.totalNumberOfDots;	i++){
				interpolatorImpl.getNextPoint(&taskResultCommonStruct.taskResultPairOfDots[i].xResult, &taskResultCommonStruct.taskResultPairOfDots[i].yResult);
				taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion++;
			}

			int msgSize=sizeof(taskResultCommonStruct);


			msgSize=msgSize+sizeof(TaskResultPairOfDots)*taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion;




			iov_t iov[2];

			SETIOV(iov+0, &taskResultCommonStruct.taskResultCommonStructHeader, sizeof(TaskResultCommonStructHeader));
			SETIOV(iov+1, &taskResultCommonStruct.taskResultPairOfDots[taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults], sizeof(TaskResultPairOfDots)*taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion);
			std::cerr<<"[SLAVE]: Portion send"<<std::endl;
			MsgSendv(chidResults,iov,2, NULL,NULL);
			taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults=taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults+taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion;
			taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion=0;
		}

		delete [] taskResultCommonStruct.taskResultPairOfDots;
	}

	if (ConnectDetach(chidTasks) == -1) {
		perror("[ERROR]: can not detach channel because of:");
		return NULL;
	}
	return NULL;
}

