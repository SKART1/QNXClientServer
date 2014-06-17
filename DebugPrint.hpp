/*
 * DebugPrint.hpp
 *
 *  Created on: 09.06.2014
 *      Author: Art
 */

#ifndef DEBUGPRINT_HPP_
#define DEBUGPRINT_HPP_


#define DEBUG_PRINT_ALLOWED
#ifdef DEBUG_PRINT_ALLOWED
	#define DEBUG_PRINT(typeInfoError, message)\
		std::cerr<<"["<<typeInfoError<<"]"<<message<<std::endl;
#else
	#define DEBUG_PRINT(typeInfoError, message)
#endif



#define DEBUG_CRITICALL_ALLOWED
#ifdef DEBUG_CRITICALL_ALLOWED
	#define DEBUG_CRITICALL_PRINT(typeInfoError, message)\
		std::cerr<<"["<<typeInfoError<<"]"<<message<<std::endl;
#else
	#define DEBUG_CRITICALL_PRINT(typeInfoError, message)
#endif



#define DEBUG_VERBOSE_ALLOWED
#ifdef DEBUG_VERBOSE_ALLOWED
	#define DEBUG_VERBOSE_PRINT(typeInfoError, message)\
		std::cerr<<"["<<typeInfoError<<"]"<<message<<std::endl;
#else
	#define DEBUG_VERBOSE_PRINT(typeInfoError, message)
#endif



#endif /* DEBUGPRINT_HPP_ */
