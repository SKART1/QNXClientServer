#include "ServerPrj.hpp"



#define QUANTUM_EXCEEDED_SIGNAL_CODE  SI_MINAVAIL

/*----------------------------------------------------------------------*/
enum StateReturns{
	QUANTUM_ENDED,
	TIMEOUT_RECIEVE_REPLY,

	NOTHING_TO_DO,

	TOO_MUCH_TASKS,
	CRITICAL_INTERNAL_ERROR,
	NON_CRITICAL_INTERNAL_ERROR,
};
/*-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/
enum StateMachine{
	ReceivingTasksState,
	GettingResultsFromSlavesState,
	GivingTasksToSlavesFirstTimeState,
};
/*-----------------------------------------------------------------------*/


/*------------------------------------------------------------------------------------*/
//Timer for quantum
static void sigusr1Handler(int signo, siginfo_t *info, void *other) {
	DEBUG_VERBOSE_PRINT("INFO", "In SIGUSR1 handler");
	ServerInternalDynamicData *serverInternalDynamicData=(ServerInternalDynamicData *)info->si_value.sival_ptr;

	serverInternalDynamicData->quantEndedRecievingTasksFromClients=true;
	serverInternalDynamicData->quantEndedGivingTasksToSlaves=true;
	serverInternalDynamicData->quantEndedGettingResultsFromSlaves=true;
	serverInternalDynamicData->quantEndedViewer=true;
}
/*------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------*/
//Timer for exceeded
static void sigusr2Handler(int signo,siginfo_t *info, void *other) {
	DEBUG_VERBOSE_PRINT("INFO", "In SIGUSR2 handler");

	/*TaskResultCommonStructHeader quickAnswerToCLient;
	quickAnswerToCLient.numberOfDotsInCurrentPortion=0;
	quickAnswerToCLient.offsetOfResults=0;
	quickAnswerToCLient.serverToClientAnswers=TASK_EXCEEDED;
*/
	ServerInternalDynamicData *serverInternalDynamicData=(ServerInternalDynamicData *)info->si_value.sival_ptr;

	/*Let`s check queue*/
/*	for(std::map  <int, TaskServerStruct>::iterator iteratorQueue=serverInternalDynamicData->serverTaskQueueStruct.begin(); iteratorQueue!=serverInternalDynamicData->serverTaskQueueStruct.end(); iteratorQueue++){
		if(iteratorQueue->second.taskCommonStruct.exceedsInNanosecds>0){
			iteratorQueue->second.taskCommonStruct.exceedsInNanosecds=iteratorQueue->second.taskCommonStruct.exceedsInNanosecds-serverInternalDynamicData->timerExceededPeriod;

		};
		if(iteratorQueue->second.taskCommonStruct.exceedsInNanosecds<0){
			quickAnswerToCLient.taskID=iteratorQueue->second.receiveClientID;
			MsgReply(iteratorQueue->second.receiveClientID, NULL, &quickAnswerToCLient, sizeof(TaskResultCommonStructHeader));
			iteratorQueue->second.receiveClientID=-1;
		};
	}*/


	TaskResultServerStruct* currentResultServerStructInServer=serverInternalDynamicData->taskResultServerStructFirst;

	/*Let`s check task that are performed now*/
	/*while(currentResultServerStructInServer!=NULL){
		if(currentResultServerStructInServer->taskServerStruct.taskCommonStruct.exceedsInNanosecds>0){
			currentResultServerStructInServer->taskServerStruct.taskCommonStruct.exceedsInNanosecds=currentResultServerStructInServer->taskServerStruct.taskCommonStruct.exceedsInNanosecds-serverInternalDynamicData->timerExceededPeriod;
		};
		if(currentResultServerStructInServer->taskServerStruct.taskCommonStruct.exceedsInNanosecds<0){
			if(	currentResultServerStructInServer->taskServerStruct.receiveClientID!=-1){
				quickAnswerToCLient.serverToClientAnswers=TASK_EXCEEDED;
				MsgReply(currentResultServerStructInServer->taskServerStruct.receiveClientID, NULL, &quickAnswerToCLient, sizeof(TaskResultCommonStructHeader));
				currentResultServerStructInServer->taskServerStruct.receiveClientID=-1;
			}
		};
		currentResultServerStructInServer=currentResultServerStructInServer->nextResultServerStruct;
	}*/
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
	/*Start ham*/
	/*1). Ham connect*/
	if(ham_connect(NULL)==-1){
		perror("[ERROR]: Ham connect");
	};

	ham_entity_t *testEntity;
	testEntity=ham_attach_self("SERVER", NULL, 0, 0, NULL);


	ham_condition_t *testCondition;
	char conditionName[]="SERVER_DIED";
	if((testCondition=ham_condition(testEntity, CONDDEATH, conditionName, /*NULL*/ HREARMAFTERRESTART))==NULL){
		perror("[ERROR]: Ham condition");

	};

	ham_action_t *testAction;
	char actionName[]="RESTART_SERVER";
	if((testAction=ham_action_restart( testCondition,	actionName, "/tmp/ServPrj_g 1>1.txt 2>2.txt" /*"/tmp/test.sh"*/, /*NULL*/HREARMAFTERRESTART))==NULL){
		perror("[ERROR]: Ham condition");
	};


	if(ham_disconnect(NULL)==-1){
		perror("[ERROR]: Ham disconnect");
	};




	timer_t timerDescriptor;
	sigevent sigeventObject;
	itimerspec timeDescriptorStruct;
	struct sigaction act;


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

	serverInternalData->slaveThreadID=new pthread_t[serverConfigs->numberOfSlaves];
	memset((void *)serverInternalData->slaveThreadID, 0, serverConfigs->numberOfSlaves*sizeof(pthread_t));

	for(unsigned int i=0; i<serverConfigs->numberOfSlaves; i++){
		if(pthread_create(&(serverInternalData->slaveThreadID[i]), &threadAttr, &Slave, argSlaveStruct)!=EOK)
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
	serverInternalDynamicData->uniqueTaskIDGenerator=1;

	/*Create first empty structure to hold results*/
	serverInternalDynamicData->taskResultServerStructFirst= new TaskResultServerStruct;
	serverInternalDynamicData->taskResultServerStructLastExistingEmpty= serverInternalDynamicData->taskResultServerStructFirst;


	//Register POSIX signal handler for quantum
	act.sa_sigaction   = &sigusr1Handler;
	act.sa_flags = 0;
	if (sigaction(SIGUSR1, &act, 0) < 0) {
		perror("[ERROR]: sigaction registering");
		return -1;
	}

	//Register POSIX signal handler for exceeded
	act.sa_sigaction   = &sigusr2Handler;
	act.sa_flags = 0;
	if (sigaction(SIGUSR2, &act, 0) < 0) {
		perror("[ERROR]: sigaction registering");
		return -1;
	}

	//Create structure with signal (SIGUSR2).
	SIGEV_SIGNAL_CODE_INIT(&sigeventObject, SIGUSR2, serverInternalDynamicData, SI_MINAVAIL);

	//Create timer
	if(timer_create( CLOCK_REALTIME, &sigeventObject ,&timerDescriptor)==-1){
		perror("[ERROR]: timer_create");
		return -1;
	}
	//Set timer mode
	//Fire every...
	timeDescriptorStruct.it_value.tv_sec = serverConfigs->timerExceededPeriod/1000000000LL;
	timeDescriptorStruct.it_value.tv_nsec= serverConfigs->timerExceededPeriod%1000000000LL;

	//Single. Means not repeat
	timeDescriptorStruct.it_interval.tv_sec =  serverConfigs->timerExceededPeriod/1000000000LL;
	timeDescriptorStruct.it_interval.tv_nsec = serverConfigs->timerExceededPeriod%1000000000LL;

	//Set new type
	if(timer_settime(timerDescriptor, NULL , &timeDescriptorStruct, NULL)==-1){
		perror("[ERROR]: timer_settime");
		return -1;
	};


	return 0;
}
/*-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/
void releaseResources(ServerInternalStaticData *serverInternalData, ServerConfigs *serverConfigs){
	for(unsigned int i=0; i<serverConfigs->numberOfSlaves; i++){
		if(serverInternalData->slaveThreadID[i]!=0){
			pthread_abort(serverInternalData->slaveThreadID[i]);
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
int inline startSingleSignalTimer(ServerInternalDynamicData *serverInternalDynamicData,timer_t *timerDescriptor, unsigned long long length,int code){
	sigevent sigeventObject;
	itimerspec timeDescriptorStruct;
	//Create with pulse with code QUANTUM_EXCEEDED
	SIGEV_SIGNAL_CODE_INIT(&sigeventObject, SIGUSR1,serverInternalDynamicData,QUANTUM_EXCEEDED_SIGNAL_CODE);

	//Create timer
	if(timer_create( CLOCK_REALTIME, &sigeventObject ,timerDescriptor)==-1){
		perror("[ERROR]: timer_create");
		return -1;
	}
	//Set timer mode
	//Fire every...
	timeDescriptorStruct.it_value.tv_sec = length/1000000000LL;
	timeDescriptorStruct.it_value.tv_nsec= length%1000000000LL;

	//Single. Means not repeat
	timeDescriptorStruct.it_interval.tv_sec =  0;
	timeDescriptorStruct.it_interval.tv_nsec = 0;

	//SetTimer
	if(timer_settime(*timerDescriptor, NULL , &timeDescriptorStruct, NULL)==-1){;
		perror("[ERROR]: timer_settime");
		return -1;
	};

	return 0;
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
int stopSingleSignalTimer(timer_t *timerDescriptor){
	itimerspec timeDescriptorStruct;
	memset(&timeDescriptorStruct,0, sizeof(itimerspec));
	timer_settime(*timerDescriptor, NULL , &timeDescriptorStruct, NULL);
	timer_delete(*timerDescriptor);
	return 0;
}
/*----------------------------------------------------------------------*/

void blockSigUsr2(){
	sigset_t  sigSet;
	sigemptyset( &sigSet );
	sigaddset( &sigSet, SIGUSR2);
	if(sigprocmask(SIG_BLOCK,  &sigSet, NULL)==-1){
		perror("[ERROR]: Blocking sigpromask");
	}

}

void unblockSigUsr2(){
	sigset_t  sigSet;
	sigemptyset( &sigSet );
	sigaddset( &sigSet, SIGUSR2);
	if(sigprocmask(SIG_UNBLOCK,  &sigSet, NULL)==-1){
		perror("[ERROR]: Blocking sigpromask");
	}
}

/*-----------------------------------------------------------------------*/
StateReturns ReceivingTasks(ServerInternalDynamicData *serverInternalDynamicData, ServerConfigs *serverConfigs, ServerInternalStaticData *serverInternalStaticData){
	std::cerr<<"[ReceivingTasks]: Entering receivingTasks"<<std::endl;
	TaskServerStruct taskServerStruct;
	TaskResultCommonStructHeader quickAnswerToCLient;
	timer_t timerDescriptorQuantum;

	_msg_info msgInfo;

	struct sigevent event;

	int recieveID=-1;

	StateReturns returnCode=QUANTUM_ENDED;




	//Set new type
	if(startSingleSignalTimer(serverInternalDynamicData,&timerDescriptorQuantum, serverConfigs->quantumNanosec, serverInternalStaticData->chidClient)==-1){
		return CRITICAL_INTERNAL_ERROR;
	}
	serverInternalDynamicData->quantEndedRecievingTasksFromClients=false;

	while(serverInternalDynamicData->quantEndedRecievingTasksFromClients==false){
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
			DEBUG_VERBOSE_PRINT("ReceivingTasks","Timeout");
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
			if(taskServerStruct.taskCommonStruct.taskID==ID_MEANS_NEW_TASK){
				/*Too much tasks*/
				if(serverInternalDynamicData->serverTaskQueueStruct.size()>serverConfigs->maxTaskQueueSize){
					quickAnswerToCLient.serverToClientAnswers=QUEU_IS_FULL;
					MsgReply(recieveID, NULL,&quickAnswerToCLient, sizeof(TaskResultCommonStructHeader));
					DEBUG_PRINT("ReceivingTasks", "Message queue is full");
					returnCode=TOO_MUCH_TASKS;
					break;
				}
				/*Not too much - add task to queue*/
				else{
					taskServerStruct.taskCommonStruct.taskID=serverInternalDynamicData->uniqueTaskIDGenerator++;
					taskServerStruct.receiveClientID=recieveID;
					serverInternalDynamicData->serverTaskQueueStruct.insert(std::make_pair(taskServerStruct.taskCommonStruct.taskID, taskServerStruct));
					DEBUG_PRINT("ReceivingTasks", "Task added to queue");
					continue;
				}
			}
			/*Already in queue*/
			else{
				/*Let`s find it*/
				std::map  <int, TaskServerStruct>::iterator taskOnServerIterator=serverInternalDynamicData->serverTaskQueueStruct.find(taskServerStruct.taskCommonStruct.taskID);
				TaskResultServerStruct* taskResultServerStruct=findTaskForward(serverInternalDynamicData->taskResultServerStructFirst, taskServerStruct.taskCommonStruct.taskID);
				/*Neither in queue nor in tasks.... Means client lie!! Client is a pie!!*/
				if(taskOnServerIterator==serverInternalDynamicData->serverTaskQueueStruct.end() && taskResultServerStruct==NULL){
					quickAnswerToCLient.serverToClientAnswers=NO_SUCH_TASK;
					MsgReply(recieveID, NULL,&quickAnswerToCLient, sizeof(TaskResultCommonStructHeader));
					DEBUG_PRINT("[ReceivingTasks]","No such task");
				}
				/*Previous part is not yet done*/
				else if (taskOnServerIterator!=serverInternalDynamicData->serverTaskQueueStruct.end()){
					quickAnswerToCLient.serverToClientAnswers=PREVIOUS_SUBTASK_HAVE_NOT_BEEN_DONE_YET;
					MsgReply(recieveID, NULL,&quickAnswerToCLient, sizeof(TaskResultCommonStructHeader));
					DEBUG_PRINT("ReceivingTasks","Previous task is only in queue!");
				}
				else if (taskResultServerStruct!=NULL){
					/*Checking if previous task have already been done*/
					if(taskResultServerStruct->taskServerStruct.receiveClientID!=-1){
						quickAnswerToCLient.serverToClientAnswers=PREVIOUS_SUBTASK_HAVE_NOT_BEEN_DONE_YET;
						MsgReply(recieveID, NULL,&quickAnswerToCLient, sizeof(TaskResultCommonStructHeader));
						std::cerr<<"[ReceivingTasks]: Previous task not yet done!"<<std::endl;
					}
					else{
						taskResultServerStruct->taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults=taskServerStruct.taskCommonStruct.offsetOfWantedDots;
						taskResultServerStruct->taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsInCurrentPortion=taskServerStruct.taskCommonStruct.numberOfWantedDots;
						taskResultServerStruct->taskServerStruct.receiveClientID=recieveID;

						/*Work is already done*/
						blockSigUsr2();
						if(taskResultServerStruct->lastCompletedDot>=(taskServerStruct.taskCommonStruct.offsetOfWantedDots+taskServerStruct.taskCommonStruct.numberOfWantedDots)){
							MsgWrite(taskResultServerStruct->taskServerStruct.receiveClientID, &(taskResultServerStruct->taskResultCommonStruct.taskResultCommonStructHeader), sizeof(TaskResultCommonStructHeader),0);
							MsgWrite(taskResultServerStruct->taskServerStruct.receiveClientID,
									&taskResultServerStruct->taskResultCommonStruct.taskResultPairOfDots[taskResultServerStruct->taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults],
									sizeof(TaskResultPairOfDots)*taskResultServerStruct->taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsInCurrentPortion,
									sizeof(TaskResultCommonStructHeader));
							MsgReply(taskResultServerStruct->taskServerStruct.receiveClientID, NULL, NULL,NULL);
							taskResultServerStruct->taskServerStruct.receiveClientID=-1;
						}
						unblockSigUsr2();
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
StateReturns GivingTasksToSlavesFirstTime(ServerInternalDynamicData *serverInternalDynamicData,ServerConfigs *serverConfigs, ServerInternalStaticData *serverInternalStaticData){
	std::cerr<<"[GivingTasksToSlavesFirstTime]: Entering GivingTasksToSlavesFirstTime"<<std::endl;
	TaskCommonStruct taskCommonStruct;

	_msg_info msgInfo;
	_pulse pulseBuff;

	timer_t timerDescriptorQuantum;
	struct sigevent event;

	int recieveID=-1;

	StateReturns returnCode=QUANTUM_ENDED;

	for(std::map  <int, TaskServerStruct>::iterator iteratorQueue=serverInternalDynamicData->serverTaskQueueStruct.begin(); iteratorQueue!=serverInternalDynamicData->serverTaskQueueStruct.end(); iteratorQueue++){
		serverInternalDynamicData->serverTaskQueueStruct.erase(iteratorQueue);
	};

	if(serverInternalDynamicData->serverTaskQueueStruct.size()>0){
		if(startSingleSignalTimer(serverInternalDynamicData,&timerDescriptorQuantum, serverConfigs->quantumNanosec, serverInternalStaticData->chidClient)==-1){
			return CRITICAL_INTERNAL_ERROR;
		}
		serverInternalDynamicData->quantEndedGivingTasksToSlaves=false;

		while(serverInternalDynamicData->quantEndedGivingTasksToSlaves==false){
			SIGEV_UNBLOCK_INIT (&event);
			if(TimerTimeout(CLOCK_REALTIME, _NTO_TIMEOUT_RECEIVE | _NTO_TIMEOUT_SEND, &event, &serverConfigs->timeoutNanosec, NULL)==-1){
				perror("[ERROR]: TimerTimeout creating");
				returnCode=CRITICAL_INTERNAL_ERROR;
				break;
			};
			//Receiving
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
				std::map  <int, TaskServerStruct>::iterator itr=serverInternalDynamicData->serverTaskQueueStruct.begin();
				if(itr->second.taskCommonStruct.exceedsInNanosecds<0){
					serverInternalDynamicData->serverTaskQueueStruct.erase(itr);
					continue;
				}
				serverInternalDynamicData->taskResultServerStructLastExistingEmpty->nextResultServerStruct=new TaskResultServerStruct();
				serverInternalDynamicData->taskResultServerStructLastExistingEmpty->nextResultServerStruct->previousResultServerStruct=serverInternalDynamicData->taskResultServerStructLastExistingEmpty;
				serverInternalDynamicData->taskResultServerStructLastExistingEmpty->nextResultServerStruct->nextResultServerStruct=NULL;


				serverInternalDynamicData->taskResultServerStructLastExistingEmpty->taskResultCommonStruct.taskResultCommonStructHeader.taskID=itr->first;

				serverInternalDynamicData->taskResultServerStructLastExistingEmpty->taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults=0;
				serverInternalDynamicData->taskResultServerStructLastExistingEmpty->taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsInCurrentPortion=0;

				serverInternalDynamicData->taskResultServerStructLastExistingEmpty->taskResultCommonStruct.taskResultPairOfDots=new TaskResultPairOfDots[(*itr).second.taskCommonStruct.totalNumberOfDots];

				blockSigUsr2();
				serverInternalDynamicData->taskResultServerStructLastExistingEmpty->taskServerStruct=itr->second;

				taskCommonStruct=itr->second.taskCommonStruct;
				serverInternalDynamicData->serverTaskQueueStruct.erase(itr);
				unblockSigUsr2();

				MsgReply(recieveID, NULL,&(taskCommonStruct), sizeof(TaskCommonStruct));
				DEBUG_PRINT("[GivingTasksToSlavesFirstTime]:","Task is send to Slave");
				serverInternalDynamicData->taskResultServerStructLastExistingEmpty=serverInternalDynamicData->taskResultServerStructLastExistingEmpty->nextResultServerStruct;
				continue;

			}
		}
		/*Switch off timer*/
		DEBUG_VERBOSE_PRINT("[GivingTasksToSlavesFirstTime]:","Exiting GivingTasksToSlavesFirstTime ");
		stopSingleSignalTimer(&timerDescriptorQuantum);
		return returnCode;
	}
	else{
		returnCode=NOTHING_TO_DO;
		return returnCode;
	}
}
/*-----------------------------------------------------------------------*/




/*-----------------------------------------------------------------------*/
StateReturns GettingResultsFromSlaves(ServerInternalDynamicData *serverInternalDynamicData, ServerConfigs *serverConfigs, ServerInternalStaticData *serverInternalStaticData){
	std::cerr<<"[INFO]: GettingResultsFromSlaves "<<std::endl;
	TaskResultServerStruct taskResultServerStructInSlave;
	//TaskResultCommonStructHeader quickAnswerToCLient;
	_msg_info msgInfo;
	timer_t timerDescriptorQuantum;

	struct sigevent event;

	int recieveID=-1;

	StateReturns returnCode=QUANTUM_ENDED;

	//Set new type
	if(startSingleSignalTimer(serverInternalDynamicData, &timerDescriptorQuantum, serverConfigs->quantumNanosec, serverInternalStaticData->chidClient)==-1){
		return CRITICAL_INTERNAL_ERROR;
	}
	serverInternalDynamicData->quantEndedGettingResultsFromSlaves=false;

	while(serverInternalDynamicData->quantEndedGettingResultsFromSlaves==false){
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
			TaskResultServerStruct* resultServerStructInServer=findTaskForward(serverInternalDynamicData->taskResultServerStructFirst, taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.taskID);

			if(resultServerStructInServer==NULL){
				std::cerr<<"[ERROR]: Slave returned work which was not given to him!"<<std::endl;
				returnCode=NON_CRITICAL_INTERNAL_ERROR;
				continue;
			}

			/*Slave has done current portion*/
			//resultServerStructInServer->taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults=taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults;
			//resultServerStructInServer->taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsInCurrentPortion=taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsInCurrentPortion;



			MsgRead(recieveID,
					&resultServerStructInServer->taskResultCommonStruct.taskResultPairOfDots[taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults],
					taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsInCurrentPortion*sizeof(TaskResultPairOfDots),
					sizeof(TaskResultCommonStructHeader));

			MsgReply(recieveID, NULL, NULL,NULL);

			resultServerStructInServer->lastCompletedDot=taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults+taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsInCurrentPortion;
			resultServerStructInServer->taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsInCurrentPortion=taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsInCurrentPortion;

			/*Data is out of date - so no need to send to server (already sent answer in exceeded (SIGUSR2) handler)*/
			if(resultServerStructInServer->taskServerStruct.taskCommonStruct.exceedsInNanosecds<0){
				for(unsigned int i=0; i<taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsInCurrentPortion; i++){
					resultServerStructInServer->taskResultCommonStruct.taskResultPairOfDots[taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults+i].resultExceeded=true;
				}
			}
			/*Data is not out of date*/
			else{
				for(unsigned int i=0; i<taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsInCurrentPortion; i++){
					resultServerStructInServer->taskResultCommonStruct.taskResultPairOfDots[taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults+i].resultExceeded=false;
				}
			}


			/*We may answer to client immediately*/
			blockSigUsr2();
			if(resultServerStructInServer->lastCompletedDot>=(resultServerStructInServer->taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults+resultServerStructInServer->taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsInCurrentPortion)
					&& resultServerStructInServer->taskServerStruct.receiveClientID!=-1){
				MsgWrite(resultServerStructInServer->taskServerStruct.receiveClientID, &(resultServerStructInServer->taskResultCommonStruct.taskResultCommonStructHeader), sizeof(TaskResultCommonStructHeader),0);
				MsgWrite(resultServerStructInServer->taskServerStruct.receiveClientID,
						&resultServerStructInServer->taskResultCommonStruct.taskResultPairOfDots[taskResultServerStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults],
						sizeof(TaskResultPairOfDots)*resultServerStructInServer->taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsInCurrentPortion,
						sizeof(TaskResultCommonStructHeader));
				MsgReply(resultServerStructInServer->taskServerStruct.receiveClientID, NULL, NULL,NULL);
				resultServerStructInServer->taskServerStruct.receiveClientID=-1;

			}
			unblockSigUsr2();
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
StateReturns ReceivingViewerQuery(ServerInternalDynamicData *serverInternalDynamicData,ServerConfigs *serverConfigs, ServerInternalStaticData *serverInternalStaticData){
	std::cerr<<"[ReceivingViewerQuery]: Entering ReceivingViewerQuery"<<std::endl;
	ViewerTaskInterest viewerTaskInterest;
	ViewerResultCommonStruct viewerResultCommonStruct;
	timer_t timerDescriptorQuantum;

	_msg_info msgInfo;

	struct sigevent event;

	int recieveID=-1;

	StateReturns returnCode=QUANTUM_ENDED;

	//Set new type
	if(startSingleSignalTimer(serverInternalDynamicData, &timerDescriptorQuantum, serverConfigs->quantumNanosec, serverInternalStaticData->chidClient)==-1){
		return CRITICAL_INTERNAL_ERROR;
	}
	serverInternalDynamicData->quantEndedRecievingTasksFromClients=false;

	while(serverInternalDynamicData->quantEndedRecievingTasksFromClients==false){
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
			std::map  <int, TaskServerStruct>::iterator taskOnServerIterator=serverInternalDynamicData->serverTaskQueueStruct.find(viewerTaskInterest.taskID);
			TaskResultServerStruct* taskResultServerStruct=findTaskForward(serverInternalDynamicData->taskResultServerStructFirst, viewerTaskInterest.taskID);
			/*Neither in queue nor in tasks.... Means client lie!! Client is a pie!!*/
			if(taskOnServerIterator==serverInternalDynamicData->serverTaskQueueStruct.end() && taskResultServerStruct==NULL){
				viewerResultCommonStruct.answer=VIEWER_NO_SUCH_TASK;
				MsgReply(recieveID, NULL,&viewerResultCommonStruct, sizeof(ViewerResultCommonStruct));
				std::cerr<<"[ReceivingViewerQuery]: No such task!"<<std::endl;
				continue;
			}
			/*Previous part is not yet done*/
			else if (taskOnServerIterator!=serverInternalDynamicData->serverTaskQueueStruct.end()){
				viewerResultCommonStruct.answer=VIEWER_TASK_IS_NOT_DONE;
				MsgReply(recieveID, NULL,&viewerResultCommonStruct, sizeof(ViewerResultCommonStruct));
				std::cerr<<"[ReceivingViewerQuery]: Previous task is only in queue!"<<std::endl;
				continue;
			}
			else if (taskResultServerStruct!=NULL){
				viewerResultCommonStruct.taskResultCommonStruct.taskResultCommonStructHeader=taskResultServerStruct->taskResultCommonStruct.taskResultCommonStructHeader;
				viewerResultCommonStruct.taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults=viewerTaskInterest.offsetOfWantedDots;
				viewerResultCommonStruct.totalNumberOfDots=taskResultServerStruct->taskServerStruct.taskCommonStruct.totalNumberOfDots;
				if(taskResultServerStruct->lastCompletedDot>=(viewerTaskInterest.offsetOfWantedDots+viewerTaskInterest.numberOfWantedDots)){
					viewerResultCommonStruct.answer=VIEWER_OK;
					viewerResultCommonStruct.taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsInCurrentPortion=viewerTaskInterest.numberOfWantedDots;
				}
				else if(taskResultServerStruct->lastCompletedDot>(viewerTaskInterest.offsetOfWantedDots)){
					viewerResultCommonStruct.answer=VIEWER_TASK_IS_PARTICALLY_DONE;
					viewerResultCommonStruct.taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsInCurrentPortion=(taskResultServerStruct->lastCompletedDot- viewerTaskInterest.offsetOfWantedDots);
				}
				else{
					viewerResultCommonStruct.answer=VIEWER_TASK_IS_NOT_DONE;
					MsgReply(recieveID, NULL,&viewerResultCommonStruct, sizeof(ViewerResultCommonStruct));
					std::cerr<<"[ReceivingViewerQuery]: Previous task is only in queue!"<<std::endl;
					continue;
				}

				/*Work is already done*/
				MsgWrite(recieveID,  &viewerResultCommonStruct.answer, sizeof(ServerToViewerAnswer),0);
				MsgWrite(recieveID, &(viewerResultCommonStruct.taskResultCommonStruct.taskResultCommonStructHeader), sizeof(TaskResultCommonStructHeader),sizeof(ServerToViewerAnswer));
				MsgWrite(recieveID,
						&taskResultServerStruct->taskResultCommonStruct.taskResultPairOfDots[viewerResultCommonStruct.taskResultCommonStruct.taskResultCommonStructHeader.offsetOfResults],
						sizeof(TaskResultPairOfDots)*viewerResultCommonStruct.taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsInCurrentPortion,
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












/*-----------------------------------------------------------------------*/
int main(int argc, char *argv[]) {
	ServerInternalDynamicData serverInternalDynamicData;
	ServerInternalStaticData serverInternalStaticData;
	ServerConfigs serverConfigs;


	std::string pathToConfigFile=std::string(argv[0]);
    pathToConfigFile=pathToConfigFile.substr(0, pathToConfigFile.find_last_of('/')+1)+std::string(CONFIG_FILE_PATH);

    if(ParseConfigFile(pathToConfigFile,&serverConfigs)==-1){
			return EXIT_FAILURE;
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






	//Create structure with signal (SIGUSR1).
	//SIGEV_SIGNAL_CODE_INIT(&sigeventObject, SIGUSR1, NULL, SI_MINAVAIL);

	//Create timer
	//if(timer_create( CLOCK_REALTIME, &sigeventObject ,&timerDescriptor)==-1){
	//	perror("[ERROR]: timer_create");
	//	releaseResources(&serverInternalStaticData, &serverConfigs);
	//	return -1;
	//}
	//Set timer mode
	//Fire every...
	//timeDescriptorStruct.it_value.tv_sec = serverConfigs.quantumNanosec/1000000000LL;
	//timeDescriptorStruct.it_value.tv_nsec= serverConfigs.quantumNanosec%1000000000LL;

	//Single. Means not repeat
	//timeDescriptorStruct.it_interval.tv_sec =  serverConfigs.quantumNanosec/1000000000LL;
	//timeDescriptorStruct.it_interval.tv_nsec = serverConfigs.quantumNanosec%1000000000LL;

	//Set new type
	//timer_settime(timerDescriptor, NULL , &timeDescriptorStruct, NULL);



	//Register POSIX signal handler




	//StateReturns stateReturns;




	/*Main cycle*/
	while(1){
		ReceivingTasks(&serverInternalDynamicData, &serverConfigs, &serverInternalStaticData);
		GettingResultsFromSlaves(&serverInternalDynamicData,&serverConfigs, &serverInternalStaticData);
		GivingTasksToSlavesFirstTime(&serverInternalDynamicData,&serverConfigs, &serverInternalStaticData);
		ReceivingViewerQuery(&serverInternalDynamicData, &serverConfigs, &serverInternalStaticData);
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
