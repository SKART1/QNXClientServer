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
#include <sys/neutrino.h>
#include <sys/netmgr.h>

/* Local headers */
#include "ablibs.h"
#include "abimport.h"
#include "proto.h"

/*My local headers*/
#include "../../ServerPrj/includes/CommonStructs.hpp"

int regime=0;
PtWidget_t * lines;


int
startStop( PtWidget_t *widget, ApInfo_t *apinfo, PtCallbackInfo_t *cbinfo )

	{

	widget = widget, apinfo = apinfo, cbinfo = cbinfo;

	PtArg_t  args[5];

	//PtLine

	PhPoint_t phPointOriginMy;
	phPointOriginMy.x=125;
	phPointOriginMy.y=0;

	PhPoint_t phPointMy[2];

	phPointMy[0].x=0;
	phPointMy[0].y=0;

	phPointMy[1].x=1000;
	phPointMy[1].y=1000;


	char serverNodeName[250];
	int serverNodeNumber;

	int serverPID;

	int serverCHID;

	int coid;




	char taskName[250];
	int taskNumber;

	/*http://127.0.0.1:50484/help/index.jsp?topic=%2Fcom.qnx.doc.photon_prog_guide%2Fres_code.html*/

	if(regime==0){
		bool success;

		PtSetArg( args, Pt_ARG_TEXT_STRING, 0, 0 );
		PtGetResources( ABW_NodeNameInput, 1, args );
		strcpy(serverNodeName, (char*) (args[0].value));
		serverNodeNumber=netmgr_strtond(serverNodeName, NULL);

		printf("[INFO]: Node name: %s and serverNodeNumber: %i\n",serverNodeName,serverNodeNumber);



		PtSetArg( args, Pt_ARG_TEXT_STRING, 0, 0 );
		PtGetResources( ABW_NodeNameInput, 1, args );
		strcpy(taskName, (char*) (args[0].value));
		taskNumber=atoi((char*) (args[0].value));

		printf("[INFO]: Test: %s\n and number: %i",taskName, taskNumber);

/*
		PtSetArg(&args[0],Pt_ARG_ORIGIN, &phPointOriginMy,1);
		PtSetResources(ABW_PtMyLine, 1, args);


		PtSetArg(&args[0],Pt_ARG_POINTS, phPointMy,2);
		PtSetResources(ABW_PtMyLine, 1, args);
*/
		if(serverNodeNumber==-1 || taskNumber<0){
			printf("[ERROR]: serverNodeNumber or  taskNumber incorrect");
			//goto deinit;
		}
		std::string pathToServInfo=std::string(serverNodeName)+std::string("/tmp/serv.serv");

		FILE *fp;
		if((fp=fopen(pathToServInfo.c_str(),"r"))==NULL){
			printf("[ERROR]: fopen serv on path: %s\n",pathToServInfo);
			goto deinit;
		};
		fscanf(fp,"SERVER_NODE_NAME: %i\n",&serverPID);
		if(fscanf(fp,"SERVER_PID: %i\n",&serverPID)!=0){
			printf("[ERROR]: Failed reading server PID\n");
			goto deinit;
		}
		printf("[INFO]: Server PID: %i\n",serverPID);

		if(fscanf(fp,"CHID_FOR_CLIENT: %i\n",&serverCHID)!=0){
			printf("[ERROR]: Failed reading server PID\n");
			goto deinit;
		};
		printf("[INFO]: Server CHID for client: %i\n",serverCHID);

		if((coid=ConnectAttach(serverNodeNumber, serverPID, serverCHID,NULL,NULL))==-1){
			goto deinit;
		};

		//MsgSendv(coid,);
		TaskResultCommonStruct taskResultStruct;
		taskResultStruct.taskResultPairOfDots=new TaskResultPairOfDots[10000];


		int number=1;
		iov_t iovSend;
		iov_t iovReceive[2];

		SETIOV(&iovSend, &(number), sizeof(int));

		SETIOV(iovReceive+0, &(taskResultStruct.taskResultCommonStructHeader), sizeof(TaskResultCommonStructHeader));
		SETIOV(iovReceive+1, taskResultStruct.taskResultPairOfDots, 10000*sizeof(TaskResultPairOfDots));

		MsgSendv(coid,&iovSend, 1, iovReceive, 2);



		for(int i=0; i<taskResultStruct.taskResultCommonStructHeader.numberOfDotsCoordinates; i++){
			printf("[INFO]: Server CHID for client X: %i, Y: %i\n",taskResultStruct.taskResultPairOfDots[i].xResult, taskResultStruct.taskResultPairOfDots[i].yResult);
		}


		PtSetArg(&args[0], Pt_ARG_TEXT_STRING,"Stop", 0);
		PtSetResources(ABW_StartStopButton, 1, args);

		regime=1;
	}
	else{
		regime=0;
		PtSetArg(&args[0], Pt_ARG_TEXT_STRING,"Start", 0);
		PtSetResources(ABW_StartStopButton, 1, args);
	}
	return( Pt_CONTINUE );

deinit:


	//aboutServerInfoStruct.nd=netmgr_strtond(aboutServerInfoStruct.serverNodeName, NULL);

	/* eliminate 'unreferenced' warnings */

	return( Pt_CONTINUE );

	}

