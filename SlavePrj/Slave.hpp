/*
 * Slave.hpp
 *
 *  Created on: 29.04.2014
 *      Author: Art
 */

#ifndef SLAVE_HPP_
#define SLAVE_HPP_

#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdio.h>
#include <iostream>
#include <errno.h>
#include "../InterpolatorImpl/InterpolatorImpl.h"
#include "../ServerPrj/includes/CommonStructs.hpp"

void * Slave(void *argSlaveParam);

typedef struct {
	char serverNodeName[250];
	pid_t pid;
	int chidTasks;
	int chidResults;
} ArgSlaveStruct;

#endif /* SLAVE_HPP_ */
