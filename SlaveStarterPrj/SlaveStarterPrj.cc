#include <cstdlib>
#include <iostream>
#include <stdio.h>

#include "../SlavePrj/Slave.hpp"
#include "../ServPrj/ServerPrj.hpp"

#include  "../DebugPrint.hpp"

int ParseServerInfoFile(std::string pathToInfoFile, ArgSlaveStruct *argSlaveStruct){
	FILE *filePointer;
	int result=0;
	if((filePointer=fopen(pathToInfoFile.c_str(),"r"))==NULL){
		DEBUG_CRITICALL_PRINT("ERROR", "Can not server info file to "<<pathToInfoFile);
		return -1;
	}
	else{
		result=result+fscanf(filePointer, "SERVER_NODE_NAME: %s\n", argSlaveStruct->serverNodeName);
		result=result+fscanf(filePointer, "SERVER_PID: %d\n", &argSlaveStruct->pid);
		fscanf(filePointer, "CHID_FOR_CLIENT: %d\n",  &argSlaveStruct->chidTasks);
		result=result+fscanf(filePointer, "CHID_TASKS_FOR_SLAVES: %d\n", &argSlaveStruct->chidTasks);
		result=result+fscanf(filePointer, "CHID_RESULTS_FOR_SLAVES: %d\n", &argSlaveStruct->chidResults);
		fclose(filePointer);
		if(result!=4){
			DEBUG_CRITICALL_PRINT("ERROR", "Wrong server info format");
			return -1;
		}
		return 0;
	}


}

int main(int argc, char *argv[]) {
	/*Slaves*/
	ArgSlaveStruct *argSlaveStruct = new ArgSlaveStruct ;
	std::string pathToInfoFile="serv.serv";

	if(argc>1){
		pathToInfoFile = argv[1];
	}

	if(ParseServerInfoFile(pathToInfoFile, argSlaveStruct)==-1){
		return NULL;
	};


	Slave(argSlaveStruct);

	return EXIT_SUCCESS;
}
