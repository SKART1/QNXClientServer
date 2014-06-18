/*
 * ServerPrj.hpp
 *
 *  Created on: 03.06.2014
 *      Author: Art
 */

#ifndef SERVERPRJ_HPP_
#define SERVERPRJ_HPP_

#include <process.h>
#include <sys/neutrino.h>
#include <pthread.h>

#include <stdio.h>  //For cout cerr and so on
#include <errno.h>	//For errno
#include <string.h>  //For strerror

#include <cstdlib>
#include <iostream>

#include <unistd.h> //For sleep

#include <sys/netmgr.h>//for node to name

#include "Structs.hpp"
#include "../SlavePrj/Slave.hpp"



/*Module to parse configuration file of server*/
#include "ParseConfigFile.hpp"

/*Structures that are used for communication with other members*/
#include "includes/CommonStructs.hpp"

/*debug outpur macroses*/
#include "../DebugPrint.hpp"

/*for ham*/
#include <ha/ham.h>


/*-----------------------------------------------------------------------*/
//Prototypes
static void sigusr1Handler(int signo, siginfo_t *info, void *other);
static void sigusr2Handler(int signo,siginfo_t *info, void *other);
void decrementTimeExceedLimits(ServerInternalDynamicData *serverInternalDynamicData);


/*-----------------------------------------------------------------------*/

#endif /* SERVERPRJ_HPP_ */
