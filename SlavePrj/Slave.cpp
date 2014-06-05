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
		interpolatorImpl.setAllNewParametrs(taskStruct.H, taskStruct.a, taskStruct.b,taskStruct.kvadrantX,taskStruct.kvadrantY,taskStruct.startX, taskStruct.startY);

		taskResultCommonStruct.taskResultCommonStructHeader.serverBusy=false;
		taskResultCommonStruct.taskResultCommonStructHeader.taskID=taskStruct.taskID;
		taskResultCommonStruct.taskResultPairOfDots=new TaskResultPairOfDots[taskStruct.numberOfNeededPoints];


		taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsCoordinates=0;
		for(int i=0; i<taskStruct.numberOfNeededPoints; i++){
			interpolatorImpl.getNextPoint(&taskResultCommonStruct.taskResultPairOfDots[i].xResult, &taskResultCommonStruct.taskResultPairOfDots[i].yResult);
			taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsCoordinates++;
		}

		int msgSize=sizeof(taskResultCommonStruct);
		if(taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsCoordinates>2){
			msgSize=msgSize+sizeof(TaskResultPairOfDots)*taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsCoordinates;

		}


		iov_t iov[2];

		//int tempSize=sizeof(TaskResultCommonStructHeader);
		//int tempSize2= sizeof(TaskResultPairOfDots)*taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsCoordinates;
		SETIOV(iov+0, &taskResultCommonStruct.taskResultCommonStructHeader, sizeof(TaskResultCommonStructHeader));
		SETIOV(iov+1, taskResultCommonStruct.taskResultPairOfDots, sizeof(TaskResultPairOfDots)*taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsCoordinates);

		MsgSendv(chidResults,iov,2, NULL,NULL);

		delete [] taskResultCommonStruct.taskResultPairOfDots;



	}


	if (ConnectDetach(chidTasks) == -1) {
		perror("[ERROR]: can not detach channel because of:");

		return NULL;
	}
	return NULL;
}

