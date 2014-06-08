#include <cstdlib>
#include <iostream>
#include <stdio.h>

#include "../SlavePrj/Slave.hpp"
#include "../ServerPrj/ServerPrj.hpp"

int ParseServerInfoFile(std::string pathToInfoFile, ArgSlaveStruct *argSlaveStruct){
	FILE *filePointer;
	int result=0;
	if((filePointer=fopen(pathToInfoFile.c_str(),"r"))==NULL){
		std::cerr<<"[ERROR]: Can not server info file to "<<pathToInfoFile<<std::endl;
		return -1;
	}
	else{
		result=result+fscanf(filePointer, "SERVER_NODE_NAME: %d\n", argSlaveStruct->serverNodeName);
		result=result+fscanf(filePointer, "SERVER_PID: %d\n", &argSlaveStruct->pid);
		fscanf(filePointer, "CHID_FOR_CLIENT: %:d\n");
		result=result+fscanf(filePointer, "CHID_TASKS_FOR_SLAVES: %d\n", &argSlaveStruct->chidTasks);
		result=result+fscanf(filePointer, "CHID_RESULTS_FOR_SLAVES: %d\n", &argSlaveStruct->chidResults);
		fclose(filePointer);
		if(result!=4){
			std::cerr<<"[ERROR]: Wrong server info format"<<std::endl;
			return -1;
		}
		return 0;
	}


}



int main(int argc, char *argv[]) {
	/*Slaves*/
	ArgSlaveStruct *argSlaveStruct = new ArgSlaveStruct ;

	if(argc<2){
		std::cerr<<"[INFO]: No path given. Using default serv.serv"<<std::endl;
		ParseServerInfoFile("serv.serv", argSlaveStruct);
	}
	else{
		ParseServerInfoFile(argv[1], argSlaveStruct);
	}


	Slave(argSlaveStruct);



	return EXIT_SUCCESS;
}
