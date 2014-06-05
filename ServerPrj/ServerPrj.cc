#include "ServerPrj.hpp"

ServerInternalDynamicData serverInternalDynamicData;

/*------------------------------------------------------------------------------------*/
//Timer for quantum
static void sigusr1Handler(int signo,siginfo_t *info, void *other) {
	std::cout<<"In handler"<<std::endl;
	serverInternalDynamicData.quantEndedRecievingTasksFromClients=true;
	serverInternalDynamicData.quantEndedGivingTasksToSlaves=true;
	serverInternalDynamicData.quantEndedGettingResultsFromSlaves=true;
	serverInternalDynamicData.quantEndedViewer=true;
}
/*------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------*/
//Timer for exceeded
static void sigusr2Handler(int signo,siginfo_t *info, void *other) {
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
		fprintf(filePointer, "SERVER_NODE_NAME: %d\n", serverInternalData->serverNodeName);
		fprintf(filePointer, "SERVER_PID: %d\n", serverInternalData->servPid);
		fprintf(filePointer, "CHID_FOR_CLIENT: %d\n", serverInternalData->chidClient);
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
int main(int argc, char *argv[]) {
	ServerInternalStaticData serverInternalData;
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
	if(initialize(&serverInternalData,&serverConfigs, &serverInternalDynamicData)==-1){
		releaseResources(&serverInternalData, &serverConfigs);
		return -1;
	}
	if(InformationToFile(&serverInternalData, &serverConfigs)==-1){
		releaseResources(&serverInternalData, &serverConfigs);
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
		releaseResources(&serverInternalData, &serverConfigs);
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
	if(timer_create( CLOCK_REALTIME, &sigeventObject ,&timerDescriptor)==-1){
		perror("[ERROR]: timer_create");
		releaseResources(&serverInternalData, &serverConfigs);
		return -1;
	}
	//Set timer mode
	//Fire every...
	timeDescriptorStruct.it_value.tv_sec = serverConfigs.timerExceededPeriod/1000000000LL;
	timeDescriptorStruct.it_value.tv_nsec= serverConfigs.timerExceededPeriod%1000000000LL;

	//Single. Means not repeat
	timeDescriptorStruct.it_interval.tv_sec =  serverConfigs.timerExceededPeriod/1000000000LL;
	timeDescriptorStruct.it_interval.tv_nsec = serverConfigs.timerExceededPeriod%1000000000LL;

	//Set new type
	//timer_settime(timerDescriptor, NULL , &timeDescriptorStruct, NULL);








	int recieveID=0;
	uint64_t        timeout;
	struct sigevent event;





	/*Main cycle*/
	while(1){




ReceivingTasksState:
		if(serverInternalDynamicData.quantEndedRecievingTasksFromClients==true){
			serverInternalDynamicData.quantEndedGettingResultsFromSlaves=false;
			goto GettingResultsFromSlaves;
		}
		else{
			/*Receiving tasks*/
			//Setting timeout
			SIGEV_UNBLOCK_INIT (&event);
			if(TimerTimeout(CLOCK_REALTIME,_NTO_TIMEOUT_RECEIVE | _NTO_TIMEOUT_SEND, &event, &serverConfigs.timeoutNanosec, NULL)==-1){
				perror("[ERROR]: TimerTimeout creating");
				releaseResources(&serverInternalData, &serverConfigs);
				return EXIT_FAILURE;
			};
			//Receiving tasks
			recieveID=MsgReceive(serverInternalData.chidClient, &taskCommonStruct, sizeof(TaskCommonStruct), NULL);
			//Timeout checking
			if (recieveID < 0 && errno==ETIMEDOUT) {
				serverInternalDynamicData.quantEndedGettingResultsFromSlaves=false;
				goto GettingResultsFromSlaves;
			}
			//Timeout of another Timer
			else if(recieveID < 0 && errno==EINTR){
				goto ReceivingTasksState;
			}
			//Pulse?! Error?! WTF?!
			else if(recieveID<=0){
				perror("[ERROR]: Wrong request");
				std::cout<<"[ERROR]: Wrong request"<<std::endl;
				releaseResources(&serverInternalData, &serverConfigs);
				return EXIT_FAILURE;
			}
			//OK - new task. Try to add to queue
			else{
				/*Too much tasks*/
				if(serverInternalDynamicData.serverTaskQueueStruct.size()>serverConfigs.maxTaskQueueSize){
					TaskResultCommonStruct taskResultCommonStructTemp;
					taskResultCommonStructTemp.taskResultCommonStructHeader.serverBusy=true;
					MsgReply(recieveID, NULL,&taskResultCommonStructTemp, sizeof(TaskResultCommonStructHeader));
					serverInternalDynamicData.quantEndedRecievingTasksFromClients=false;
					goto GettingResultsFromSlaves;
				}
				/*Not too much - add task to queue*/
				else{
					taskCommonStruct.taskID=recieveID;
					serverInternalDynamicData.serverTaskQueueStruct.insert(std::make_pair(recieveID, taskCommonStruct));
					goto ReceivingTasksState;
				}
			}
		}


GettingResultsFromSlaves:
		if(serverInternalDynamicData.quantEndedGettingResultsFromSlaves==true){
			//serverInternalDynamicData.quantEndedGettingResultsFromSlaves=false;
			serverInternalDynamicData.quantEndedGivingTasksToSlaves=false;
			goto GivingTasksToSlavesState;
		}
		else{
			SIGEV_UNBLOCK_INIT (&event);
			timeout = serverConfigs.timeoutNanosec;
			if(TimerTimeout(CLOCK_REALTIME, _NTO_TIMEOUT_RECEIVE | _NTO_TIMEOUT_SEND, &event, &timeout, NULL)==-1){
				perror("[ERROR]: TimerTimeout creating");
				releaseResources(&serverInternalData, &serverConfigs);
				return EXIT_FAILURE;
			};
			//Receiving results
			//TaskResultCommonStruct taskResultCommonStruct;
			//char array[10];
			TaskResultServerStruct taskResultCommonStructInSlave;
			struct _msg_info info;
			recieveID=MsgReceive(serverInternalData.chidSlaveResult, &(taskResultCommonStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader), sizeof(TaskResultCommonStructHeader), &info);
			//Timeout
			if (recieveID<0 &&  errno== ETIMEDOUT  ) {
				serverInternalDynamicData.quantEndedGivingTasksToSlaves=false;
				goto GivingTasksToSlavesState;
			}
			//Timeout of another Timer
			else if(recieveID < 0 && errno==EINTR){
				goto GettingResultsFromSlaves;
			}
			//Not timeout
			else{
				//Find the task structure in which to store everything
				TaskResultServerStruct* currentResultServerStructInServer=serverInternalDynamicData.taskResultServerStructFirst;
				while(currentResultServerStructInServer!=NULL){
					if(currentResultServerStructInServer->taskResultCommonStruct.taskResultCommonStructHeader.taskID==taskResultCommonStructInSlave.taskResultCommonStruct.taskResultCommonStructHeader.taskID){
						break;
					}
					currentResultServerStructInServer=currentResultServerStructInServer->nextResultServerStruct;
				}

				if(currentResultServerStructInServer==NULL){
					std::cerr<<"[ERROR]: Slave returned work which was not given to him!"<<std::endl;
					return -1;
				}

				MsgRead(recieveID,currentResultServerStructInServer->taskResultCommonStruct.taskResultPairOfDots,info.srcmsglen-sizeof(TaskResultCommonStructHeader),sizeof(TaskResultCommonStructHeader));


				int a=10;

				MsgWrite(currentResultServerStructInServer->taskResultCommonStruct.taskResultCommonStructHeader.taskID, &(currentResultServerStructInServer->taskResultCommonStruct.taskResultCommonStructHeader), sizeof(TaskResultCommonStructHeader),0);
				MsgWrite(currentResultServerStructInServer->taskResultCommonStruct.taskResultCommonStructHeader.taskID, currentResultServerStructInServer->taskResultCommonStruct.taskResultPairOfDots,sizeof(TaskResultPairOfDots)*currentResultServerStructInServer->taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsCoordinates,sizeof(TaskResultCommonStructHeader));
				MsgReply(currentResultServerStructInServer->taskResultCommonStruct.taskResultCommonStructHeader.taskID, NULL, NULL,NULL);
				//MsgReply(,NULL,&(currentResultServerStructInServer->taskResultCommonStruct),info.srcmsglen);
				goto GettingResultsFromSlaves;
			}
		}


GivingTasksToSlavesState:
		if(serverInternalDynamicData.quantEndedGivingTasksToSlaves==true){
			//serverInternalDynamicData.quantEndedGivingTasksToSlaves=false;
			serverInternalDynamicData.quantEndedViewer=false;
			goto GivingResultsToViewersState;
		}
		else{
			if(serverInternalDynamicData.serverTaskQueueStruct.size()>0){
				SIGEV_UNBLOCK_INIT (&event);
				timeout = serverConfigs.timeoutNanosec;
				if(TimerTimeout(CLOCK_REALTIME, _NTO_TIMEOUT_RECEIVE | _NTO_TIMEOUT_SEND, &event, &timeout, NULL)==-1){
					perror("[ERROR]: TimerTimeout creating");
					releaseResources(&serverInternalData, &serverConfigs);
					return EXIT_FAILURE;
				};
				//Receiving
				//TaskResultCommonStruct taskResultCommonStruct;
				recieveID=MsgReceive(serverInternalData.chidSlaveTask, NULL, NULL, NULL);
				//Timeout - no free Slaves - this is bad news....
				if (recieveID<0 &&  errno== ETIMEDOUT) {
					goto GivingTasksToSlavesState;
				}
				//Timeout of another Timer
				else if(recieveID < 0 && errno==EINTR){
					goto GivingTasksToSlavesState;
				}
				//Not a timeout
				else{
					std::map  <int, TaskCommonStruct>::iterator itr=serverInternalDynamicData.serverTaskQueueStruct.begin();

					serverInternalDynamicData.taskResultServerStructLastExistingEmpty->nextResultServerStruct=new TaskResultServerStruct();
					serverInternalDynamicData.taskResultServerStructLastExistingEmpty->nextResultServerStruct->previousResultServerStruct=serverInternalDynamicData.taskResultServerStructLastExistingEmpty;
					serverInternalDynamicData.taskResultServerStructLastExistingEmpty->nextResultServerStruct->nextResultServerStruct=NULL;

					serverInternalDynamicData.taskResultServerStructLastExistingEmpty->taskResultCommonStruct.taskResultCommonStructHeader.taskID=itr->first;
					serverInternalDynamicData.taskResultServerStructLastExistingEmpty->taskResultCommonStruct.taskResultCommonStructHeader.serverBusy=false;
					serverInternalDynamicData.taskResultServerStructLastExistingEmpty->taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsCoordinates=(itr->second.numberOfNeededPoints);
					serverInternalDynamicData.taskResultServerStructLastExistingEmpty->taskResultCommonStruct.taskResultCommonStructHeader.exceedsInNanosecds=itr->second.exceedsInNanosecds;
					if(serverInternalDynamicData.taskResultServerStructLastExistingEmpty->taskResultCommonStruct.taskResultCommonStructHeader.exceedsInNanosecds<0){
						serverInternalDynamicData.taskResultServerStructLastExistingEmpty->taskResultCommonStruct.taskResultCommonStructHeader.taskExceeded=true;
					}
					else{
						serverInternalDynamicData.taskResultServerStructLastExistingEmpty->taskResultCommonStruct.taskResultCommonStructHeader.taskExceeded=false;
					}

					serverInternalDynamicData.taskResultServerStructLastExistingEmpty->taskResultCommonStruct.taskResultPairOfDots=new TaskResultPairOfDots[(*itr).second.numberOfNeededPoints];


					serverInternalDynamicData.serverTaskQueueStruct.erase(itr);

					MsgReply(recieveID, NULL,&(taskCommonStruct), sizeof(taskCommonStruct));

					serverInternalDynamicData.taskResultServerStructLastExistingEmpty->nextResultServerStruct=serverInternalDynamicData.taskResultServerStructLastExistingEmpty->nextResultServerStruct->nextResultServerStruct;

					goto GivingTasksToSlavesState;
				}
			}
			else{
				serverInternalDynamicData.quantEndedViewer=false;
				goto GivingResultsToViewersState;
			}
		}





GivingResultsToViewersState:
		goto ReceivingTasksState;
		if(1/*serverInternalDynamicData.quantEndedViewer==true*/){
			serverInternalDynamicData.quantEndedGettingResultsFromSlaves=false;
			goto GettingResultsFromSlaves;
		}
		else{
			int viewerTaskInterest=-1;
			/*What task viewer wants*/
			//Setting timeout
			SIGEV_UNBLOCK_INIT (&event);
			timeout = serverConfigs.timeoutNanosec;
			if(TimerTimeout(CLOCK_REALTIME, _NTO_TIMEOUT_RECEIVE | _NTO_TIMEOUT_SEND, &event, &timeout, NULL)==-1){
				perror("[ERROR]: TimerTimeout creating");
				releaseResources(&serverInternalData, &serverConfigs);
				return EXIT_FAILURE;
			};
			//Receiving tasks
			recieveID=MsgReceive(serverInternalData.chidViewer, &viewerTaskInterest, sizeof(viewerTaskInterest), NULL);
			//Timeout
			if (recieveID<0 &&  errno== ETIMEDOUT) {
				serverInternalDynamicData.quantEndedRecievingTasksFromClients=false;
				goto ReceivingTasksState;
			}
			//Timeout of another Timer
			else if(recieveID < 0 && errno==EINTR){
				goto GivingResultsToViewersState;
			}
			//Not timeout
			else{
				/*Find if this task has ever exist*/
				ViewerResultCommonStruct viewerResultCommonStruct;

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
			}
		}
	}











	return EXIT_SUCCESS;
}
