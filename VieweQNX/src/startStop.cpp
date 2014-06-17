/* Y o u r   D e s c r i p t i o n                       */
/*                            AppBuilder Photon Code Lib */
/*                                         Version 2.03  */

/* Standard headers */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//#include <string.h>


/*My standard headers*/
#include <sys/netmgr.h>
#include <string>
#include <iostream>
#include <sys/neutrino.h>
#include <sys/netmgr.h>

/* Local headers */
#include "ablibs.h"
#include "abimport.h"
#include "proto.h"

/*My local headers*/
#include "../../ServPrj/includes/CommonStructs.hpp"

#define BUFF_SIZE 5000
#define PORTION 10;

int regime=0;
PtWidget_t ** lines;



/*-----------------------------------------------------------------------*/
int ParseServerInfoFile(std::string pathToInfoFile, char * serverNodeName,  pid_t *servPID, int *servCHID){
	FILE *filePointer = NULL;

	int result=0;


	if((filePointer=fopen(pathToInfoFile.c_str(),"r"))==NULL){
		std::cerr<<"[ERROR]: Can not server info file to "<<pathToInfoFile.c_str()<<std::endl;
		return -1;
	}
	else{
		result=result+fscanf(filePointer, "SERVER_NODE_NAME: %s\n", serverNodeName);
		result=result+fscanf(filePointer, "SERVER_PID: %d\n", servPID);
		fscanf(filePointer, "CHID_FOR_CLIENT: %d\t\n", servCHID);
		fscanf(filePointer, "CHID_TASKS_FOR_SLAVES: %d\t\n", servCHID);
		fscanf(filePointer, "CHID_RESULTS_FOR_SLAVES: %d\t\n", servCHID);
		result=result+fscanf(filePointer, "CHID_FOR_VIEWERS: %d\n", servCHID);
		fclose(filePointer);
		if(result!=3){
			std::cerr<<"[ERROR]: Wrong server info format"<<std::endl;
			return -1;
		}

	}
	return 0;
}
/*-----------------------------------------------------------------------*/


int
startStop( PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo )

	{

	widget = widget, apinfo = apinfo, cbinfo = cbinfo;

	PtArg_t  args[5];

	//PtLine
	PhPoint_t phPointOriginMy;
	phPointOriginMy.x=150;
	phPointOriginMy.y=0;


	PhPoint_t phPointMy[2];



	if(regime==0){
		char pathToServerInfoFile[250];
		char serverNodeName[250];
		int serverNodeNumber;
		int serverPID;
		int serverCHID;
		int coid;
		int taskNumber;
		unsigned long currentDot;

		iov_t iovSend;
		iov_t iovReceive[3];

		/*Get path to server info file*/
		PtSetArg( args, Pt_ARG_TEXT_STRING, 0, 0 );
		PtGetResources( ABW_NodeNameInput, 1, args );
		strcpy(pathToServerInfoFile, (char*) (args[0].value));

		/*Get task number*/
		PtSetArg( args, Pt_ARG_TEXT_STRING, 0, 0 );
		PtGetResources( ABW_WorkNumberInput, 1, args );
		taskNumber=atoi((char*) (args[0].value));

		ParseServerInfoFile(pathToServerInfoFile, serverNodeName, &serverPID, &serverCHID);
		serverNodeNumber=netmgr_strtond(serverNodeName, NULL);

		if(serverNodeNumber==-1 || taskNumber<0){
			printf("[ERROR]: serverNodeNumber or  taskNumber incorrect");
			return -1;
		};


		if((coid=ConnectAttach(serverNodeNumber, serverPID, serverCHID,NULL,NULL))==-1){
			perror("[ERROR]: Connect attach");
			return -1;
		};


		ViewerResultCommonStruct viewerResultCommonStruct;
		viewerResultCommonStruct.taskResultCommonStruct.taskResultPairOfDots=new TaskResultPairOfDots[BUFF_SIZE];

		ViewerTaskInterest viewerTaskInterest;
		viewerTaskInterest.taskID=taskNumber;
		viewerTaskInterest.offsetOfWantedDots=0;
		viewerTaskInterest.numberOfWantedDots=BUFF_SIZE;

		lines=new PtWidget_t*[BUFF_SIZE];
		memset(lines, NULL, sizeof(PtWidget_t*)*BUFF_SIZE);
		for(int i=0; i<BUFF_SIZE;i++){
			lines[i]=NULL;
		}
		currentDot=0;
		viewerResultCommonStruct.totalNumberOfDots=1;
		int currentLine;
		currentLine=0;
		while(viewerResultCommonStruct.totalNumberOfDots>currentDot && viewerResultCommonStruct.answer!=VIEWER_NO_SUCH_TASK){
			viewerTaskInterest.offsetOfWantedDots=currentDot;
			viewerTaskInterest.numberOfWantedDots=BUFF_SIZE;

			SETIOV(&iovSend, &(viewerTaskInterest), sizeof(ViewerTaskInterest));

			SETIOV(iovReceive+0, &(viewerResultCommonStruct.answer), sizeof(ServerToViewerAnswer));
			SETIOV(iovReceive+1, &(viewerResultCommonStruct.taskResultCommonStruct.taskResultCommonStructHeader), sizeof(TaskResultCommonStructHeader));
			SETIOV(iovReceive+2, &viewerResultCommonStruct.taskResultCommonStruct.taskResultPairOfDots[currentDot], (BUFF_SIZE-currentDot)*sizeof(TaskResultPairOfDots));

			if(MsgSendv(coid,&iovSend, 1, iovReceive, 3)==-1){
				perror("[EROR]: Message send!");
				goto deinit;
			};
			switch(viewerResultCommonStruct.answer){
				case VIEWER_NO_SUCH_TASK:
					PtSetArg(&args[0], Pt_ARG_TEXT_STRING,"VIEWER_NO_SUCH_TASK", 0);
					PtSetResources(ABW_PtLabelStatus, 1, args);
					goto deinit;
					break;
				case VIEWER_TASK_IS_NOT_DONE:
					PtSetArg(&args[0], Pt_ARG_TEXT_STRING,"TASK_IN_QUEU", 0);
					PtSetResources(ABW_PtLabelStatus, 1, args);
					usleep(5000);
					break;
				case VIEWER_OK:
				case VIEWER_TASK_IS_PARTICALLY_DONE:
					PtSetArg(&args[0], Pt_ARG_TEXT_STRING,"VIEWER_OK", 0);
					PtSetResources(ABW_PtLabelStatus, 1, args);
					for(unsigned int i=(viewerTaskInterest.offsetOfWantedDots); i<(viewerResultCommonStruct.taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsInCurrentPortion); i++){
						if(i==0){
							i=1;
						}
						phPointMy[0].x=viewerResultCommonStruct.taskResultCommonStruct.taskResultPairOfDots[i-1].xResult;
						phPointMy[0].y=-1*viewerResultCommonStruct.taskResultCommonStruct.taskResultPairOfDots[i-1].yResult;

						phPointMy[1].x=viewerResultCommonStruct.taskResultCommonStruct.taskResultPairOfDots[i].xResult;
						phPointMy[1].y=-1*viewerResultCommonStruct.taskResultCommonStruct.taskResultPairOfDots[i].yResult;
						PtSetArg(&args[0],Pt_ARG_POINTS, phPointMy,2);


						PtSetArg(&args[0],Pt_ARG_POINTS, phPointMy,2);
						PtSetArg(&args[1],Pt_ARG_ORIGIN, &phPointOriginMy,1);
						if(viewerResultCommonStruct.taskResultCommonStruct.taskResultPairOfDots[i-1].resultExceeded==true || viewerResultCommonStruct.taskResultCommonStruct.taskResultPairOfDots[i].resultExceeded==true){
							PtSetArg(&args[2],Pt_ARG_COLOR,0x00FF0000,0);
						}
						else{
							PtSetArg(&args[2],Pt_ARG_COLOR,0x0000FF00,0);
						}


						lines[currentLine]=PtCreateWidget(PtLine, Pt_DEFAULT_PARENT,3, args);
						PtRealizeWidget(lines[currentLine]);
						currentLine++;
						phPointOriginMy.x=500;
						phPointOriginMy.y=350;
						PtSetArg(&args[0],Pt_ARG_ORIGIN, &phPointOriginMy,1);
						PtSetResources(lines[i-1], 1, args);



						printf("[INFO]: NUmber: %d X: %f, Y: %f\n",i,viewerResultCommonStruct.taskResultCommonStruct.taskResultPairOfDots[i].xResult, viewerResultCommonStruct.taskResultCommonStruct.taskResultPairOfDots[i].yResult);
						//break;
					}
					currentDot=currentDot+viewerResultCommonStruct.taskResultCommonStruct.taskResultCommonStructHeader.numberOfDotsInCurrentPortion;
					viewerTaskInterest.offsetOfWantedDots=currentDot;
					viewerTaskInterest.numberOfWantedDots=viewerTaskInterest.numberOfWantedDots-currentDot;
					break;
			}


		}
		PtSetArg(&args[0], Pt_ARG_TEXT_STRING,"Stop", 0);
		PtSetResources(ABW_StartStopButton, 1, args);
		delete [] viewerResultCommonStruct.taskResultCommonStruct.taskResultPairOfDots;
		regime=1;
		return( Pt_CONTINUE );

	deinit:
		for(int i=0; i<BUFF_SIZE;i++){
			if(lines[i]!=NULL){
				PtDestroyWidget(lines[i]);
			}
		}
		delete [] lines;
		delete [] viewerResultCommonStruct.taskResultCommonStruct.taskResultPairOfDots;
		return( Pt_CONTINUE );
	}
	else{
		regime=0;
		for(int i=0; i<BUFF_SIZE;i++){
			if(lines[i]!=NULL){
				PtDestroyWidget(lines[i]);
			}
		}
		delete [] lines;
		PtSetArg(&args[0], Pt_ARG_TEXT_STRING,"Start", 0);
		PtSetResources(ABW_StartStopButton, 1, args);
		return( Pt_CONTINUE );
	}



	//aboutServerInfoStruct.nd=netmgr_strtond(aboutServerInfoStruct.serverNodeName, NULL);

	/* eliminate 'unreferenced' warnings */

	return( Pt_CONTINUE );
	}


