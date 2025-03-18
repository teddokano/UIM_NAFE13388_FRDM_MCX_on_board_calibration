/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef NAFE_PRINTOUTPUT_H
#define NAFE_PRINTOUTPUT_H

#include	<stdio.h>
#include	<inttypes.h>

/** PrintOutput class
 *	
 *  @class PrintOutput
 *
 *	PrintOutput is a class to support output text to screen and file. 
 *	This supports color and modifier control by color() method.
 */

class	PrintOutput
{
public:
	
	/** Color identifiers */
	enum class Color : uint16_t {
		black		= 0x001,
		red			= 0x002,
		green		= 0x003,
		yellow		= 0x004,
		blue		= 0x005,
		magenta		= 0x006,
		cyan		= 0x007,
		gray		= 0x008,
		none		= 0x009,
		bg_black	= 0x010,
		bg_red		= 0x020,
		bg_green	= 0x030,
		bg_yellow	= 0x040,
		bg_blue		= 0x050,
		bg_magenta	= 0x060,
		bg_cyan		= 0x070,
		bg_gray		= 0x080,
		bg_none		= 0x090,
		bold		= 0x200,
		italic		= 0x400,
		underline	= 0x500,
		blink		= 0x600,
		swap		= 0x800,
		normal		= 0x900,
	};
	
	/** Create a PrintOutput instance
	 *
	 * @param file_name default is nullptr to disabling file out
	 * @param file_ext file name extention
	 * @param time_info "true" for putinf date and time information in the file name
	 */
	PrintOutput( const char *file_name = nullptr, const char *file_ext = "csv", bool time_info = true );

	/** Printf as normal printf
	 */
	void	printf( const char *format, ... );

	/** Screen output : Text output for screen only
	 *
	 * @param s string to output
	 * @param file_output true, if file output is needed
	 */
	void	screen( const char *s, bool file_output = false );

	/** Screen output : Text output for screen only
	 *
	 * @param c color and/or midifier
	 * @param file_output true, if file output is needed
	 */
	void	color( Color c, bool file_output = false );

private:
	friend Color operator+( const Color& lhs, const Color& rhs )
	{
		return Color( static_cast<uint16_t>( lhs ) + static_cast<uint16_t>( rhs ) );
	}

	friend Color operator+( const Color& lhs, const int n )
	{
		return Color( static_cast<uint16_t>( lhs ) + n );
	}

	friend Color operator+( const int n, const Color& rhs )
	{
		return Color( n + static_cast<uint16_t>( rhs ) );
	}

	FILE	*fp;
	
	static const char	*code_format[];
};

#endif	//	NAFE_PRINTOUTPUT_H
