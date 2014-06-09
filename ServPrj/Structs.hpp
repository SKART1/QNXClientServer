/*
 * Structs.hpp
 *
 *  Created on: 29.04.2014
 *      Author: Art
 */

#ifndef STRUCTS_HPP_
#define STRUCTS_HPP_

#include <stdint.h> //For uint32_t
#include <process.h> //For pid_t

#include <map> //for map

#include "includes/CommonStructs.hpp"







typedef struct TaskResultServerStruct TaskResultServerStruct;

struct TaskResultServerStruct{
	TaskResultCommonStruct taskResultCommonStruct;
	int receiveClientID;
	unsigned long lastCompletedDot;
	TaskResultServerStruct* nextResultServerStruct;
	TaskResultServerStruct* previousResultServerStruct;
};

typedef struct{
	TaskCommonStruct taskCommonStruct;
	int receiveClientID;
}TaskServerStruct;




typedef struct {
	std::map  <int, TaskServerStruct> serverTaskQueueStruct;
	TaskResultServerStruct *taskResultServerStructFirst;
	TaskResultServerStruct *taskResultServerStructLastExistingEmpty;

	unsigned int numberOfTasksInQueue;

	bool quantEndedRecievingTasksFromClients;
	bool quantEndedGivingTasksToSlaves;
	bool quantEndedGettingResultsFromSlaves;
	bool quantEndedViewer;

	unsigned long uniqueTaskID;
} ServerInternalDynamicData;


typedef struct {
	char serverNodeName[250];
	pid_t servPid;
	int chidClient;
	int chidSlaveTask;
	int chidSlaveResult;
	int chidViewer;
	pthread_t *tid;
} ServerInternalStaticData;

typedef struct{
	char severInfoFileName[250];
	unsigned long long int quantumNanosec;
	unsigned long long int timeoutNanosec;
	unsigned long long int timerExceededPeriod;

	unsigned int numberOfSlaves;
	unsigned  int maxTaskQueueSize;
}ServerConfigs;


#endif /* STRUCTS_HPP_ */
