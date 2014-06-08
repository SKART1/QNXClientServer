#include "ServerPrj.hpp"

ServerInternalDynamicData serverInternalDynamicData;

/*------------------------------------------------------------------------------------*/
//Timer for quantum
static void sigusr1Handler(int signo,siginfo_t *info, void *other) {
	std::cerr<<"In handler 1"<<std::endl;
	serverInternalDynamicData.quantEndedRecievingTasksFromClients=true;
	serverInternalDynamicData.quantEndedGivingTasksToSlaves=true;
	serverInternalDynamicData.quantEndedGettingResultsFromSlaves=true;
	serverInternalDynamicData.quantEndedViewer=true;
}
/*------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------*/
//Timer for exceeded
static void sigusr2Handler(int signo,siginfo_t *info, void *other) {
	std::cerr<<"In handler 2"<<std::endl;
	//for(std::map  <int, TaskResultCommonStruct>::iterator iteratorQueue=serverInternalDynamicData.serverResultsStruct.begin(); iteratorQueue!=serverInternalDynamicData.serverResultsStruct.end(); iteratorQueue++){
		//iteratorQueue
	//}
}
/*------------------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
int InformationToFile(ServerInternalStaticData *serverInternalData, ServerConfigs *serverConfigs){
	FILE * filePointer;
	filePointer = fopen(serverConfigs->severInfoFileName, "w");
	if (filePointer == NULL) {
		perror("[ERROR]: fopen:");
		return -1;
	}
	else{
		fprintf(filePointer, "SERVER_NODE_NAME: %s\n", serverInternalData->serverNodeName);
		fprintf(filePointer, "SERVER_PID: %d\n", serverInternalData->servPid);
		fprintf(filePointer, "CHID_FOR_CLIENT: %d\n", serverInternalData->chidClient);
		fprintf(filePointer, "CHID_TASKS_FOR_SLAVES: %d\n", serverInternalData->chidSlaveTask);
		fprintf(filePointer, "CHID_RESULTS_FOR_SLAVES: %d\n", serverInternalData->chidSlaveResult);
		fprintf(filePointer, "CHID_FOR_VIEWERS: %d\n", serverInternalData->chidViewer);
		fclose(filePointer);
		return 0;
	}
}
/*-----------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
int initialize(ServerInternalStaticData *serverInternalData, ServerConfigs *serverConfigs, ServerInternalDynamicData *serverInternalDynamicData){
	/*Info about server*/
	serverInternalData->servPid=getpid();

	if(netmgr_ndtostr(ND2S_DIR_SHOW, 0,  serverInternalData->serverNodeName,250)==-1){
		perror("[ERROR]: Node name to node number");
		return -1;
	};


	/*Channels*/
	if((serverInternalData->chidClient= ChannelCreate(NULL))==-1){
		printf("[ERROR]: %d error creating channel for clients: %s\n",errno, strerror(errno));
		return -1;
	}
	if((serverInternalData->chidSlaveTask= ChannelCreate(NULL))==-1){
		printf("[ERROR]: %d error creating channel for slave tasks: %s\n",errno, strerror(errno));
		return -1;
	}
	if((serverInternalData->chidSlaveResult= ChannelCreate(NULL))==-1){
		printf("[ERROR]: %d error creating channel for slave results: %s\n",errno, strerror(errno));
		return -1;
	}
	if((serverInternalData->chidViewer= ChannelCreate(NULL))==-1){
		printf("[ERROR]: %d error creating channel for viewer: %s\n",errno, strerror(errno));
		return -1;
	}


	/*Slaves*/
	ArgSlaveStruct *argSlaveStruct = new ArgSlaveStruct ;
	pthread_attr_t threadAttr;
	unsigned int errorNumber=0;

	strcpy(argSlaveStruct->serverNodeName,serverInternalData->serverNodeName);
	argSlaveStruct->pid=serverInternalData->servPid;
	argSlaveStruct->chidTasks=serverInternalData->chidSlaveTask;
	argSlaveStruct->chidResults=serverInternalData->chidSlaveResult;

	pthread_attr_init(&threadAttr);
	pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);

	serverInternalData->tid=new pthread_t[serverConfigs->numberOfSlaves];
	memset(serverInternalData->tid, 0, serverConfigs->numberOfSlaves*sizeof(pthread_t));

	for(unsigned int i=0; i<serverConfigs->numberOfSlaves; i++){
		if(pthread_create(&(serverInternalData->tid[i]), &threadAttr, &Slave, argSlaveStruct)!=EOK)
		{
			errorNumber++;
			i--;
		};
		if(errorNumber>serverConfigs->numberOfSlaves){
			return -1;
		};
	};

	/*Initializing server state*/
	serverInternalDynamicData->quantEndedRecievingTasksFromClients=false;
	serverInternalDynamicData->quantEndedGivingTasksToSlaves=false;
	serverInternalDynamicData->quantEndedGettingResultsFromSlaves=false;
	serverInternalDynamicData->quantEndedViewer=false;
	serverInternalDynamicData->uniqueTaskID=1;

	return 0;
}
/*-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/
void releaseResources(ServerInternalStaticData *serverInternalData, ServerConfigs *serverConfigs){
	for(unsigned int i=0; i<serverConfigs->numberOfSlaves; i++){
		if(serverInternalData->tid[i]!=0){
			pthread_abort(serverInternalData->tid[i]);
		};
	};
}
/*-----------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
inline TaskResultServerStruct*  findTaskForward(TaskResultServerStruct* placeToStart,int taskID){
	TaskResultServerStruct* currentResultServerStructInServer=placeToStart;
	if(placeToStart==NULL){
		return NULL;
	}
	while(currentResultServerStructInServer!=NULL){
		if(currentResultServerStructInServer->taskResultCommonStruct.taskResultCommonStructHeader.taskID==taskID){
			break;
		}
		currentResultServerStructInServer=currentResultServerStructInServer->nextResultServerStruct;
	}
	return currentResultServerStructInServer;
}
/*-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/

#define QUANTUM_EXCEEDED_SIGNAL_CODE  SI_MINAVAIL

/*-----------------------------------------------------------------------*/
int inline startSingleSignalTimer(timer_t *timerDescriptor, unsigned long long length,int code){
	sigevent sigeventObject;
	itimerspec timeDescriptorStruct;
	//Create with pulse with code QUANTUM_EXCEEDED
	SIGEV_SIGNAL_CODE_INIT(&sigeventObject, SIGUSR1,NULL,QUANTUM_EXCEEDED_SIGNAL_CODE);

	//Create timer
	/*if(timer_create( CLOCK_REALTIME, &sigeventObject ,timerDescriptor)==-1){
		perror("[ERROR]: timer_create");
		return -1;
	}*/
	//Set timer mode
	//Fire every...
	timeDescriptorStruct.it_value.tv_sec = length/1000000000LL;
	timeDescriptorStruct.it_value.tv_nsec= length%1000000000LL;

	//Single. Means not repeat
	timeDescriptorStruct.it_interval.tv_sec =  0;
	timeDescriptorStruct.it_interval.tv_nsec = 0;

	//SetTimer
	//timer_settime(*timerDescriptor, NULL , &timeDescriptorStruct, NULL);

	return 0;
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
int stopSingleSignalTimer(timer_t *timerDescriptor){
	//itimerspec timeDescriptorStruct;
	//memset(&timeDescriptorStruct,0, sizeof(itimerspec));
	//timer_settime(*timerDescriptor, NULL , &timeDescriptorStruct, NULL);
	//timer_delete(*timerDescriptor);
	return 0;
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
typedef enum{
	QUANTUM_ENDED,
	TIMEOUT_RECIEVE_REPLY,

	NOTHING_TO_DO,

	TOO_MUCH_TASKS,
	CRITICAL_INTERNAL_ERROR,
	NON_CRITICAL_INTERNAL_ERROR,
} StateReturns;
/*-----------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
StateReturns ReceivingTasks(ServerConfigs *serverConfigs, ServerInternalStaticData *serverInternalStaticData){
	std::cerr<<"[ReceivingTasks]: Entering receivingTasks"<<std::endl;
	TaskServerStruct taskServerStruct;
	TaskResultCommonStructHeader quickAnswerToCLient;
	timer_t timerDescriptorQuantum;

	_msg_info msgInfo;

	struct sigevent event;

	int recieveID=-1;

	StateReturns returnCode=QUANTUM_ENDED;

	//Set new type
	if(startSingleSignalTimer(&timerDescriptorQuantum, serverConfigs->quantumNanosec, serverInternalStaticData->chidClient)==-1){
		return CRITICAL_INTERNAL_ERROR;
	}
	serverInternalDynamicData.quantEndedRecievingTasksFromClients=false;

	while(serverInternalDynamicData.quantEndedRecievingTasksFromClients==false){
		/*Receiving tasks*/
		//Setting timeout
		SIGEV_UNBLOCK_INIT (&event);
		if(TimerTimeout(CLOCK_REALTIME,_NTO_TIMEOUT_RECEIVE | _NTO_TIMEOUT_SEND, &event, &serverConfigs->timeoutNanosec, NULL)==-1){
			perror("[ERROR]: TimerTimeout creating");
			returnCode=CRITICAL_INTERNAL_ERROR;
			break;
		};
		//Receiving tasks
		recieveID=MsgReceive(serverInternalStaticData->chidClient, &taskServerStruct.taskCommonStruct, sizeof(TaskCommonStruct), &msgInfo);
		//Timeout checking
		if (recieveID < 0 && errno==ETIMEDOUT) {
			std::cerr<<"[ReceivingTasks]: Timeout"<<std::endl;
			returnCode=TIMEOUT_RECIEVE_REPLY;
			break;
		}
		//Quantum ended checking (SIGUSR1) or "Exceeded" were updated in SIGUSR1
		else if(recieveID < 0 && errno==EINTR){
			std::cerr<<"[ReceivingTasks]: SIGUSR1 or SIGUSR2"<<std::endl;
			continue;
		}
		//Pulse?! Error?! WTF?!
		else if(recieveID<=0){
			perror("[ERROR]: Wrong request");
			continue;
		}
		//OK - new task. Try to add to queue
		else{
			/*New task*/
			if(taskServerStruct.taskCommonStruct.taskID==0){
				/*Too much tasks*/
				if(serverInternalDynamicData.serverTaskQueueStruct.size()>serverConfigs->maxTaskQueueSize){
					quickAnswerToCLient.serverToClientAnswers=QUEU_IS_FULL;
					MsgReply(recieveID, NULL,&quickAnswerToCLient, sizeof(TaskResultCommonStructHeader));
					std::cerr<<"[ReceivingTasks]: Message queue is full"<<std::endl;
					returnCode=TOO_MUCH_TASKS;
					break;
				}
				/*Not too much - add task to queue*/
				else{
					taskServerStruct.taskCommonStruct.taskID=serverInternalDynamicData.uniqueTaskID++;
					taskServerStruct.receiveClientID=recieveID;
					serverInternalDynamicData.serverTaskQueueStruct.insert(std::make_pair(taskServerStruct.taskCommonStruct.taskID, taskServerStruct));
					std::cerr<<"[ReceivingTasks]: Task added to queue"<<std::endl;
					continue;
				}
			}
			/*Already in queue*/
			else{
				/*Let`s find it*/
				std::map  <int, TaskServerStruct>::iterator taskOnServerIterator=serverInternalDynamicData.serverTaskQueueStruct.find(taskServerStruct.taskCommonStruct.taskID);
				TaskResultServerStruct* taskResultServerStruct=findTaskForward(serverInternalDynamicData.taskResultServerStructFirst, taskServerStruct.taskCommonStruct.taskID);
				/*Neither in queue nor in tasks.... Means client lie!! Client is a pie!!*/
				if(taskOnServerIterator==serverInternalDynamicData.serverTaskQueueStruct.end() && taskResultServerStruct==NULL){
					quickAnswerToCLient.serverToClientAnswers=NO_SUCH_TASK;
					MsgReply(recieveID, NULL,&quickAnswerToCLient, sizeof(TaskResultCommonStructHeader));
					std::cerr<<"[ReceivingTasks]: No such task!"<<std::endl;
					continue;
				}
				/*Previous part is not yet done*/
				else if (taskOnServerIterator!=serverInternalDynamicData.serverTaskQueueStruct.end()){
					quickAnswerToCLient.serverToClientAnswers=PREVIOUS_SUBTASK_HAVE_NOT_BEEN_DONE_YET;
					MsgReply(recieveID, NULL,&quickAnswerToCLient, sizeof(TaskResultCommonStructHeader));
					std::cerr<<"[ReceivingTasks]: Previous task is only in queue!"<<std::endl;
					continue;
				}
				else if (taskResultServerStruct!=NULL){
					/*Checking if previous task have already been done*/
					if(taskResultServerStruct->receiveClientID!=-1){
						quickAnswerToCLient.serverToClientAnswers=PREVIOUS_SUBTASK_HAVE_NOT_BEEN_DONE_YET;
						MsgReply(recieveID, NULL,&quickAnswerToCLient, sizeof(TaskResultCommonStructHeader));
						std::cerr<<"[ReceivingTasks]: Previous task not yet done!"<<std::endl;
						continue;
					}
					else{
						taskResultServerStruct->taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults=taskServerStruct.taskCommonStruct.offsetOfWantedDots;
						taskResultServerStruct->taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion=taskServerStruct.taskCommonStruct.numberOfWantedDots;
						taskResultServerStruct->receiveClientID=recieveID;

						/*Work is already done*/
						if(taskResultServerStruct->lastCompletedDot>=(taskServerStruct.taskCommonStruct.offsetOfWantedDots+taskServerStruct.taskCommonStruct.numberOfWantedDots)){
							MsgWrite(taskResultServerStruct->receiveClientID, &(taskResultServerStruct->taskResultCommonStruct.taskResultCommonStructHeader), sizeof(TaskResultCommonStructHeader),0);
							MsgWrite(taskResultServerStruct->receiveClientID,
									&taskResultServerStruct->taskResultCommonStruct.taskResultPairOfDots[taskResultServerStruct->taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults],
									sizeof(TaskResultPairOfDots)*taskResultServerStruct->taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion,
									sizeof(TaskResultCommonStructHeader));
							MsgReply(taskResultServerStruct->receiveClientID, NULL, NULL,NULL);
							taskResultServerStruct->receiveClientID=-1;
							continue;
						}
					}

				}
			}
		}
	}

	/*Switch off timer*/
	std::cerr<<"[INFO]: End receivingTasks "<<returnCode<<std::endl;
	stopSingleSignalTimer(&timerDescriptorQuantum);
	return returnCode;
}
/*-----------------------------------------------------------------------*/



/*-----------------------------------------------------------------------*/
StateReturns GivingTasksToSlavesFirstTime(ServerConfigs *serverConfigs, ServerInternalStaticData *serverInternalStaticData){
	std::cerr<<"[GivingTasksToSlavesFirstTime]: Entering GivingTasksToSlavesFirstTime"<<std::endl;
	TaskCommonStruct taskCommonStruct;

	_msg_info msgInfo;
	_pulse pulseBuff;

	timer_t timerDescriptorQuantum;
	struct sigevent event;

	int recieveID=-1;

	StateReturns returnCode=QUANTUM_ENDED;

	if(startSingleSignalTimer(&timerDescriptorQuantum, serverConfigs->quantumNanosec, serverInternalStaticData->chidClient)==-1){
		return CRITICAL_INTERNAL_ERROR;
	}
	serverInternalDynamicData.quantEndedGivingTasksToSlaves=false;

	while(serverInternalDynamicData.quantEndedGivingTasksToSlaves==false){
		if(serverInternalDynamicData.serverTaskQueueStruct.size()>0){
			SIGEV_UNBLOCK_INIT (&event);
			if(TimerTimeout(CLOCK_REALTIME, _NTO_TIMEOUT_RECEIVE | _NTO_TIMEOUT_SEND, &event, &serverConfigs->timeoutNanosec, NULL)==-1){
				perror("[ERROR]: TimerTimeout creating");
				returnCode=CRITICAL_INTERNAL_ERROR;
				break;
			};
			//Receiving
			//TaskResultCommonStruct taskResultCommonStruct;
			recieveID=MsgReceive(serverInternalStaticData->chidSlaveTask, &pulseBuff, sizeof(_pulse), &msgInfo);
			//Timeout - no free Slaves - this is bad news....
			if (recieveID<0 &&  errno== ETIMEDOUT) {
				returnCode=TIMEOUT_RECIEVE_REPLY;
				break;
			}
			//SIGUSR1 or SIGUSR2
			else if(recieveID < 0 && errno==EINTR){
				continue;
			}
			//Not a timeout
			else{
				std::map  <int, TaskServerStruct>::iterator itr=serverInternalDynamicData.serverTaskQueueStruct.begin();

				serverInternalDynamicData.taskResultServerStructLastExistingEmpty->nextResultServerStruct=new TaskResultServerStruct();
				serverInternalDynamicData.taskResultServerStructLastExistingEmpty->nextResultServerStruct->previousResultServerStruct=serverInternalDynamicData.taskResultServerStructLastExistingEmpty;
				serverInternalDynamicData.taskResultServerStructLastExistingEmpty->nextResultServerStruct->nextResultServerStruct=NULL;


				serverInternalDynamicData.taskResultServerStructLastExistingEmpty->taskResultCommonStruct.taskResultCommonStructHeader.taskID=itr->first;
				serverInternalDynamicData.taskResultServerStructLastExistingEmpty->taskResultCommonStruct.taskResultCommonStructHeader.exceedsInNanosecds=itr->second.taskCommonStruct.exceedsInNanosecds;
				serverInternalDynamicData.taskResultServerStructLastExistingEmpty->receiveClientID=itr->second.receiveClientID;

				serverInternalDynamicData.taskResultServerStructLastExistingEmpty->taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults=0;
				serverInternalDynamicData.taskResultServerStructLastExistingEmpty->taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion=itr->second.taskCommonStruct.portionSize;
				serverInternalDynamicData.taskResultServerStructLastExistingEmpty->taskResultCommonStruct.taskResultCommonStructHeader.totalNumberOfDots=itr->second.taskCommonStruct.totalNumberOfDots;


				serverInternalDynamicData.taskResultServerStructLastExistingEmpty->taskResultCommonStruct.taskResultPairOfDots=new TaskResultPairOfDots[(*itr).second.taskCommonStruct.totalNumberOfDots];

				taskCommonStruct=itr->second.taskCommonStruct;
				serverInternalDynamicData.serverTaskQueueStruct.erase(itr);

				MsgReply(recieveID, NULL,&(taskCommonStruct), sizeof(TaskCommonStruct));
				std::cerr<<"[GivingTasksToSlavesFirstTime]: Task is send to Slave"<<std::endl;
				serverInternalDynamicData.taskResultServerStructLastExistingEmpty=serverInternalDynamicData.taskResultServerStructLastExistingEmpty->nextResultServerStruct;
				continue;

			}
		}
		else{
			returnCode=NOTHING_TO_DO;
			break;
		}
	}

	/*Switch off timer*/
	std::cerr<<"[GivingTasksToSlavesFirstTime]: Exiting GivingTasksToSlavesFirstTime "<<returnCode<<std::endl;
	stopSingleSignalTimer(&timerDescriptorQuantum);
	return returnCode;
}
/*-----------------------------------------------------------------------*/



int test=0;
/*-----------------------------------------------------------------------*/
StateReturns GettingResultsFromSlaves(ServerConfigs *serverConfigs, ServerInternalStaticData *serverInternalStaticData){
	std::cerr<<"[INFO]: GettingResultsFromSlaves "<<test++<<std::endl;
	TaskResultServerStruct taskResultServerStructInSlave;
	//TaskResultCommonStructHeader quickAnswerToCLient;
	_msg_info msgInfo;
	timer_t timerDescriptorQuantum;

	struct sigevent event;

	int recieveID=-1;

	StateReturns returnCode=QUANTUM_ENDED;

	//Set new type
	if(startSingleSignalTimer(&timerDescriptorQuantum, serverConfigs->quantumNanosec, serverInternalStaticData->chidClient)==-1){
		return CRITICAL_INTERNAL_ERROR;
	}
	serverInternalDynamicData.quantEndedGettingResultsFromSlaves=false;

	while(serverInternalDynamicData.quantEndedGettingResultsFromSlaves==false){
		SIGEV_UNBLOCK_INIT (&event);
		if(TimerTimeout(CLOCK_REALTIME, _NTO_TIMEOUT_RECEIVE | _NTO_TIMEOUT_SEND, &event, &(serverConfigs->timeoutNanosec), NULL)==-1){
			perror("[ERROR]: TimerTimeout creating");
			return CRITICAL_INTERNAL_ERROR;
		};
		//Receiving results
		recieveID=MsgReceive(serverInternalStaticData->chidSlaveResult, &(taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader), sizeof(TaskResultCommonStructHeader), &msgInfo);

		if (recieveID < 0 && errno==ETIMEDOUT){
			returnCode=TIMEOUT_RECIEVE_REPLY;
			break;
		}
		//SIGUSR1 or SIGUSR2
		else if(recieveID < 0 && errno==EINTR){
			continue;
		}
		//Not timeout
		else{
			//Find the task structure in which to store everything
			TaskResultServerStruct* resultServerStructInServer=findTaskForward(serverInternalDynamicData.taskResultServerStructFirst, taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.taskID);

			if(resultServerStructInServer==NULL){
				std::cerr<<"[ERROR]: Slave returned work which was not given to him!"<<std::endl;
				returnCode=NON_CRITICAL_INTERNAL_ERROR;
				continue;
			}

			/*Slave has done current portion*/
			//resultServerStructInServer->taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults=taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults;
			//resultServerStructInServer->taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion=taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion;



			MsgRead(recieveID,
					&resultServerStructInServer->taskResultCommonStruct.taskResultPairOfDots[taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults],
					taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion*sizeof(TaskResultPairOfDots),
					sizeof(TaskResultCommonStructHeader));

			MsgReply(recieveID, NULL, NULL,NULL);

			resultServerStructInServer->lastCompletedDot=taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults+taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion;


			/*Data is out of date - so no need to send to server (already sent answer in exceeded (SIGUSR2) handler)*/
			if(resultServerStructInServer->taskResultCommonStruct.taskResultCommonStructHeader.exceedsInNanosecds<0){
				for(unsigned int i=0; i<taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion; i++){
					resultServerStructInServer->taskResultCommonStruct.taskResultPairOfDots[taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults+i].resultExceeded=true;
				}
			}
			/*Data is not out of date*/
			else{
				for(unsigned int i=0; i<taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion; i++){
					resultServerStructInServer->taskResultCommonStruct.taskResultPairOfDots[taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults+i].resultExceeded=false;
				}
			}


			/*We may answer to client immediately*/
			if(resultServerStructInServer->lastCompletedDot>=(resultServerStructInServer->taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults+resultServerStructInServer->taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion)
					&& resultServerStructInServer->receiveClientID!=-1){
				MsgWrite(resultServerStructInServer->receiveClientID, &(resultServerStructInServer->taskResultCommonStruct.taskResultCommonStructHeader), sizeof(TaskResultCommonStructHeader),0);
				MsgWrite(resultServerStructInServer->receiveClientID,
						&resultServerStructInServer->taskResultCommonStruct.taskResultPairOfDots[taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults],
						sizeof(TaskResultPairOfDots)*resultServerStructInServer->taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion,
						sizeof(TaskResultCommonStructHeader));
				MsgReply(resultServerStructInServer->receiveClientID, NULL, NULL,NULL);
				resultServerStructInServer->receiveClientID=-1;

			}
			continue;
			/*Or will answer to client later*/
		}
	}
	/*Switch off timer*/
	std::cerr<<"[INFO]: End GettingResultsFromSlaves "<<returnCode<<std::endl;
	stopSingleSignalTimer(&timerDescriptorQuantum);
	return returnCode;
}
/*-----------------------------------------------------------------------*/



/*-----------------------------------------------------------------------*/
StateReturns ReceivingViewerQuery(ServerConfigs *serverConfigs, ServerInternalStaticData *serverInternalStaticData){
	std::cerr<<"[ReceivingViewerQuery]: Entering ReceivingViewerQuery"<<std::endl;
	ViewerTaskInterest viewerTaskInterest;
	ViewerResultCommonStruct viewerResultCommonStruct;
	timer_t timerDescriptorQuantum;

	_msg_info msgInfo;

	struct sigevent event;

	int recieveID=-1;

	StateReturns returnCode=QUANTUM_ENDED;

	//Set new type
	if(startSingleSignalTimer(&timerDescriptorQuantum, serverConfigs->quantumNanosec, serverInternalStaticData->chidClient)==-1){
		return CRITICAL_INTERNAL_ERROR;
	}
	serverInternalDynamicData.quantEndedRecievingTasksFromClients=false;

	while(serverInternalDynamicData.quantEndedRecievingTasksFromClients==false){
		/*Receiving tasks*/
		//Setting timeout
		SIGEV_UNBLOCK_INIT (&event);
		if(TimerTimeout(CLOCK_REALTIME,_NTO_TIMEOUT_RECEIVE | _NTO_TIMEOUT_SEND, &event, &serverConfigs->timeoutNanosec, NULL)==-1){
			perror("[ERROR]: TimerTimeout creating");
			returnCode=CRITICAL_INTERNAL_ERROR;
			break;
		};
		//Receiving tasks
		recieveID=MsgReceive(serverInternalStaticData->chidViewer, &viewerTaskInterest, sizeof(ViewerTaskInterest), &msgInfo);
		//recieveID=MsgReceive(serverInternalStaticData->chidClient, &taskServerStruct.taskCommonStruct, sizeof(TaskCommonStruct), &msgInfo);
		//Timeout checking
		if (recieveID < 0 && errno==ETIMEDOUT) {
			std::cerr<<"[ReceivingViewerQuery]: Timeout"<<std::endl;
			returnCode=TIMEOUT_RECIEVE_REPLY;
			break;
		}
		//Quantum ended checking (SIGUSR1) or "Exceeded" were updated in SIGUSR1
		else if(recieveID < 0 && errno==EINTR){
			std::cerr<<"[ReceivingTasks]: SIGUSR1 or SIGUSR2"<<std::endl;
			continue;
		}
		//Pulse?! Error?! WTF?!
		else if(recieveID<=0){
			perror("[ERROR]: Wrong request");
			continue;
		}
		//OK - new task. Try to add to queue
		else{
			/*Let`s find it*/
			std::map  <int, TaskServerStruct>::iterator taskOnServerIterator=serverInternalDynamicData.serverTaskQueueStruct.find(viewerTaskInterest.taskID);
			TaskResultServerStruct* taskResultServerStruct=findTaskForward(serverInternalDynamicData.taskResultServerStructFirst, viewerTaskInterest.taskID);
			/*Neither in queue nor in tasks.... Means client lie!! Client is a pie!!*/
			if(taskOnServerIterator==serverInternalDynamicData.serverTaskQueueStruct.end() && taskResultServerStruct==NULL){
				viewerResultCommonStruct.answer=VIEWER_NO_SUCH_TASK;
				MsgReply(recieveID, NULL,&viewerResultCommonStruct, sizeof(ViewerResultCommonStruct));
				std::cerr<<"[ReceivingViewerQuery]: No such task!"<<std::endl;
				continue;
			}
			/*Previous part is not yet done*/
			else if (taskOnServerIterator!=serverInternalDynamicData.serverTaskQueueStruct.end()){
				viewerResultCommonStruct.answer=VIEWER_TASK_IS_NOT_DONE;
				MsgReply(recieveID, NULL,&viewerResultCommonStruct, sizeof(ViewerResultCommonStruct));
				std::cerr<<"[ReceivingViewerQuery]: Previous task is only in queue!"<<std::endl;
				continue;
			}
			else if (taskResultServerStruct!=NULL){
				viewerResultCommonStruct.taskResultCommonStruct.taskResultCommonStructHeader=taskResultServerStruct->taskResultCommonStruct.taskResultCommonStructHeader;
				viewerResultCommonStruct.taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults=viewerTaskInterest.offsetOfWantedDots;
				if(taskResultServerStruct->lastCompletedDot>=(viewerTaskInterest.offsetOfWantedDots+viewerTaskInterest.numberOfWantedDots)){
					viewerResultCommonStruct.taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion=viewerTaskInterest.numberOfWantedDots;
				}
				else if(taskResultServerStruct->lastCompletedDot>(viewerTaskInterest.offsetOfWantedDots)){
					viewerResultCommonStruct.answer=VIEWER_TASK_IS_PARTICALLY_DONE;
					viewerResultCommonStruct.taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion=(taskResultServerStruct->lastCompletedDot- viewerTaskInterest.offsetOfWantedDots);
				}
				else{
					viewerResultCommonStruct.answer=VIEWER_TASK_IS_NOT_DONE;
					MsgReply(recieveID, NULL,&viewerResultCommonStruct, sizeof(ViewerResultCommonStruct));
					std::cerr<<"[ReceivingViewerQuery]: Previous task is only in queue!"<<std::endl;
					continue;
				}

				//	taskResultServerStruct->taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion=taskServerStruct.taskCommonStruct.numberOfWantedDots;
				//	taskResultServerStruct->receiveClientID=recieveID;

					/*Work is already done*/

				MsgWrite(recieveID,  &viewerResultCommonStruct.answer, sizeof(ServerToViewerAnswer),0);
				MsgWrite(recieveID, &(viewerResultCommonStruct.taskResultCommonStruct.taskResultCommonStructHeader), sizeof(TaskResultCommonStructHeader),sizeof(ServerToViewerAnswer));
				MsgWrite(recieveID,
						&taskResultServerStruct->taskResultCommonStruct.taskResultPairOfDots[viewerResultCommonStruct.taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults],
						sizeof(TaskResultPairOfDots)*viewerResultCommonStruct.taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsEvaluatedInCurrentPortion,
						sizeof(ServerToViewerAnswer)+ sizeof(TaskResultCommonStructHeader));
				MsgReply(recieveID, NULL, NULL,NULL);

				continue;
			}
		}
	}


	/*Switch off timer*/
	std::cerr<<"[ReceivingViewerQuery]: End ReceivingViewerQuery "<<returnCode<<std::endl;
	stopSingleSignalTimer(&timerDescriptorQuantum);
	return returnCode;
}
/*-----------------------------------------------------------------------*/








typedef enum{
	ReceivingTasksState,
	GettingResultsFromSlavesState,
	GivingTasksToSlavesFirstTimeState,
} StateMachine;



/*-----------------------------------------------------------------------*/
int main(int argc, char *argv[]) {
	ServerInternalStaticData serverInternalStaticData;
	TaskCommonStruct taskCommonStruct;
	ServerConfigs serverConfigs;

	serverInternalDynamicData.taskResultServerStructFirst= new TaskResultServerStruct;
	serverInternalDynamicData.taskResultServerStructLastExistingEmpty= serverInternalDynamicData.taskResultServerStructFirst;


	/*Open configuration file*/
	if(argc>1){
		if(ParseConfigFile(argv[1],&serverConfigs)==-1){
			return EXIT_FAILURE;
		}
	}
	else{
		if(ParseConfigFile("",&serverConfigs)==-1){
			return EXIT_FAILURE;
		}
	}


	/*Initialize server - open channel and create threads. Put all necessary info to file*/
	if(initialize(&serverInternalStaticData,&serverConfigs, &serverInternalDynamicData)==-1){
		releaseResources(&serverInternalStaticData, &serverConfigs);
		return -1;
	}
	if(InformationToFile(&serverInternalStaticData, &serverConfigs)==-1){
		releaseResources(&serverInternalStaticData, &serverConfigs);
		return -1;
	}


	//Pre-work - create timers and so on
	timer_t timerDescriptor;
	sigevent sigeventObject;
	itimerspec timeDescriptorStruct;
	struct sigaction act;



	//Register POSIX signal handler
	act.sa_sigaction   = &sigusr1Handler;
	act.sa_flags = 0;
	if (sigaction(SIGUSR1, &act, 0) < 0) {
		perror("[ERROR]: sigaction registering");
		return -1;
	}

	//Create structure with signal (SIGUSR1).
	SIGEV_SIGNAL_CODE_INIT(&sigeventObject, SIGUSR1, NULL, SI_MINAVAIL);

	//Create timer
	if(timer_create( CLOCK_REALTIME, &sigeventObject ,&timerDescriptor)==-1){
		perror("[ERROR]: timer_create");
		releaseResources(&serverInternalStaticData, &serverConfigs);
		return -1;
	}
	//Set timer mode
	//Fire every...
	timeDescriptorStruct.it_value.tv_sec = serverConfigs.quantumNanosec/1000000000LL;
	timeDescriptorStruct.it_value.tv_nsec= serverConfigs.quantumNanosec%1000000000LL;

	//Single. Means not repeat
	timeDescriptorStruct.it_interval.tv_sec =  serverConfigs.quantumNanosec/1000000000LL;
	timeDescriptorStruct.it_interval.tv_nsec = serverConfigs.quantumNanosec%1000000000LL;

	//Set new type
	//timer_settime(timerDescriptor, NULL , &timeDescriptorStruct, NULL);



	//Register POSIX signal handler
	act.sa_sigaction   = &sigusr2Handler;
	act.sa_flags = 0;
	if (sigaction(SIGUSR2, &act, 0) < 0) {
		perror("[ERROR]: sigaction registering");
		return -1;
	}

	//Create structure with signal (SIGUSR1).
	SIGEV_SIGNAL_CODE_INIT(&sigeventObject, SIGUSR2, NULL, SI_MINAVAIL);

	//Create timer
	//if(timer_create( CLOCK_REALTIME, &sigeventObject ,&timerDescriptor)==-1){
		//perror("[ERROR]: timer_create");
		//releaseResources(&serverInternalStaticData, &serverConfigs);
		//return -1;
	//}
	//Set timer mode
	//Fire every...
	//timeDescriptorStruct.it_value.tv_sec = serverConfigs.timerExceededPeriod/1000000000LL;
	//timeDescriptorStruct.it_value.tv_nsec= serverConfigs.timerExceededPeriod%1000000000LL;

	//Single. Means not repeat
	//timeDescriptorStruct.it_interval.tv_sec =  serverConfigs.timerExceededPeriod/1000000000LL;
	//timeDescriptorStruct.it_interval.tv_nsec = serverConfigs.timerExceededPeriod%1000000000LL;

	//Set new type
	//timer_settime(timerDescriptor, NULL , &timeDescriptorStruct, NULL);

	//StateReturns stateReturns;




	/*Main cycle*/
	while(1){
		ReceivingTasks(&serverConfigs, &serverInternalStaticData);
		GettingResultsFromSlaves(&serverConfigs, &serverInternalStaticData);
		GivingTasksToSlavesFirstTime(&serverConfigs, &serverInternalStaticData);
		ReceivingViewerQuery(&serverConfigs, &serverInternalStaticData);
		//if(stateReturns==)


	}
	//int recieveID=0;







	//while(1){





//GivingResultsToViewersState:
//		goto ReceivingTasksState;
//		if(1/*serverInternalDynamicData.quantEndedViewer==true*/){
//			serverInternalDynamicData.quantEndedGettingResultsFromSlaves=false;
//			goto GettingResultsFromSlaves;
//		}
//		else{
//			int viewerTaskInterest=-1;
			/*What task viewer wants*/
			//Setting timeout
//			SIGEV_UNBLOCK_INIT (&event);
//			timeout = serverConfigs.timeoutNanosec;
//			if(TimerTimeout(CLOCK_REALTIME, _NTO_TIMEOUT_RECEIVE | _NTO_TIMEOUT_SEND, &event, &timeout, NULL)==-1){
//				perror("[ERROR]: TimerTimeout creating");
//				releaseResources(&serverInternalStaticData, &serverConfigs);
//				return EXIT_FAILURE;
//			};
			//Receiving tasks
//			recieveID=MsgReceive(serverInternalStaticData.chidViewer, &viewerTaskInterest, sizeof(viewerTaskInterest), NULL);
			//Timeout
//			if (recieveID<0 &&  errno== ETIMEDOUT) {
//				serverInternalDynamicData.quantEndedRecievingTasksFromClients=false;
//				goto ReceivingTasksState;
//			}
			//Timeout of another Timer
//			else if(recieveID < 0 && errno==EINTR){
//				goto GivingResultsToViewersState;
//			}
			//Not timeout
//			else{
				/*Find if this task has ever exist*/
//				ViewerResultCommonStruct viewerResultCommonStruct;

				/*There is no such task - sorry...*/
				/*if(serverResultsStructIterator==serverInternalDynamicData.serverResultsStruct.end()){
					viewerResultCommonStruct.exist=false;
					MsgReply(recieveID, NULL,&viewerResultCommonStruct, sizeof(viewerResultCommonStruct));
					goto GivingResultsToViewersState;
				}*/
				/*Oh, here it is!*/
				/*else{
					viewerResultCommonStruct.exist=true;
					viewerResultCommonStruct.taskResultCommonStruct=serverResultsStructIterator->second;
					MsgReply(recieveID, NULL,&viewerResultCommonStruct, sizeof(viewerResultCommonStruct));
					goto GivingResultsToViewersState;
				}*/
//			}
//		}
//	}



	return EXIT_SUCCESS;
}
