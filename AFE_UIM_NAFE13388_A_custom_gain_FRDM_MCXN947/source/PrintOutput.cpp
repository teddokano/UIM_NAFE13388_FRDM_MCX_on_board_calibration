/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"PrintOutput.h"

#include	<time.h>
#include	<stdarg.h>

constexpr auto time_offset_hour	= 9;
constexpr auto time_offset_min	= 0;


PrintOutput::PrintOutput( const char *file_name, const char *file_ext, bool time_info )
{
	constexpr int	filename_length	= 256;
	char			s[ 100 ];
	char			filename[ filename_length ];

	if ( !file_name )
		return;

	if ( time_info )
	{
		time_t		current_time;
		struct tm	*tmp;
		
		current_time	 = time( NULL );
		tmp				 = localtime( &current_time );
		tmp->tm_hour	+= time_offset_hour;
		tmp->tm_min		+= time_offset_min;
		current_time	 = mktime( tmp );

		strftime( s, 100, "%Y-%m-%d_%a_%H-%M-%S", localtime( &current_time ) );
		sprintf( filename, "%s_%s.%s", file_name, s, file_ext );

		printf( "log file name : %s\r\n", filename );
	}
	else
	{
		sprintf( filename, "%s.%s", file_name, file_ext );
	}
	
	if ( NULL == (fp	= fopen( filename, "w" )) )
		::printf( "file open error\r\n" );
}

void PrintOutput::printf( const char *format, ... )
{
	constexpr int	char_length	= 256;
	char 			s[ char_length ];

	va_list args;
	va_start( args, format );
	vsnprintf( s, char_length, format, args );
	va_end( args );

	::printf( s );
	
	if ( fp )
		fputs( s, fp );
}

void PrintOutput::screen( const char* s )
{
	::printf( "%s", s );
}
