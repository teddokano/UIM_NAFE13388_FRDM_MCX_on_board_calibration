/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef NAFE_PRINTOUTPUT_H
#define NAFE_PRINTOUTPUT_H

#include	<stdio.h>

class	PrintOutput
{
public:
	PrintOutput( const char *file_name, const char *file_ext = "csv", bool time_info = true );
	void	printf( const char *format, ... );
	void	screen( const char *s );
private:
	FILE	*fp;
};

#endif	//	NAFE_PRINTOUTPUT_H
