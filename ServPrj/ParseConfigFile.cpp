/*
 * ParseConfigFile.cpp
 *
 *  Created on: 03.06.2014
 *      Author: Art
 */
#include <ParseConfigFile.hpp>


/*-----------------------------------------------------------------------*/
int ParseConfigFile(std::string pathToConfig, ServerConfigs *serverConfigs){
	if(pathToConfig.length()==0){
		pathToConfig=std::string(CONFIG_FILE_PATH);
	}
	FILE *filePointer = NULL;
	filePointer = fopen((pathToConfig.c_str()), "r");

	if (filePointer == NULL) {
		perror("[ERROR]: Can not open file");
		return -1;
	}


	int scanned=0;
	int totalNumberOfParametrs=6;
	do{
		scanned=0;

		scanned=scanned+fscanf(filePointer, "QUANTUM_NANOSEC: %llu\n\t", &(serverConfigs->quantumNanosec));
		scanned=scanned+fscanf(filePointer, "TIMEOUT_NANOSEC: %llu\n\t", &(serverConfigs->timeoutNanosec));

		scanned=scanned+fscanf(filePointer, "TIMER_EXCEEDED_PERIOD: %llu\n\t", &(serverConfigs->timerExceededPeriod));

		scanned=scanned+fscanf(filePointer, "NUMBER_OF_SLAVES: %i\n\t", &(serverConfigs->numberOfSlaves));
		scanned=scanned+fscanf(filePointer, "MAX_TASK_QUEU_SIZE: %i\n\t",  &(serverConfigs->maxTaskQueueSize));
		scanned=scanned+fscanf(filePointer, "SERVER_INFO_FILE_NAME: %s\n\t",  serverConfigs->severInfoFileName);
		totalNumberOfParametrs=totalNumberOfParametrs-scanned;
	}
	while(scanned>0);


	fclose(filePointer);

	if(totalNumberOfParametrs!=6){
		return -1;
	}
	return 0;
}
/*-----------------------------------------------------------------------*/
