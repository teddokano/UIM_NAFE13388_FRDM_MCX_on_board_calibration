/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"r01lib.h"
#include	"afe/NAFE13388_UIM.h"
#include	<math.h>
#include	<array>

#include	"PrintOutput.h"

SPI				spi( D11, D12, D13, D10 );	//	MOSI, MISO, SCLK, CS
NAFE13388_UIM	afe( spi );
PrintOutput		out( "test" );
//PrintOutput	out( nullptr );	//use this line to disable file output

using enum	NAFE13388_UIM::Register16;
using enum	NAFE13388_UIM::Register24;
using enum	NAFE13388_UIM::Command;

using 	raw_t			= NAFE13388_UIM::raw_t;
using 	microvolt_t		= NAFE13388_UIM::microvolt_t;

constexpr int	CAL_FOR_PGA_0_2		= 0;

constexpr int	INPUT_GND			= 0x0010;
constexpr int	INPUT_A1P_SINGLE	= 0x1010;
constexpr int	INPUT_A1N_SINGLE	= 0x0110;
constexpr int	INPUT_A1__DIFF		= 0x1110;
//constexpr int	ch_config3			= 0x2E09;
//constexpr int	ch_config3			= 0x2E08;
//constexpr int	ch_config3			= 0x2E01;
constexpr int	ch_config3			= 0x0000;

using	ch_setting_t	= const uint16_t[ 4 ];

constexpr ch_setting_t	chs[]	= {
	{ INPUT_GND,        (CAL_FOR_PGA_0_2 << 12) | 0x0084, 0x2900, ch_config3 },
	{ INPUT_A1P_SINGLE, (CAL_FOR_PGA_0_2 << 12) | 0x0084, 0x2900, ch_config3 },
	{ INPUT_A1N_SINGLE, (CAL_FOR_PGA_0_2 << 12) | 0x0084, 0x2900, ch_config3 },
	{ INPUT_A1__DIFF,   (CAL_FOR_PGA_0_2 << 12) | 0x0084, 0x2900, ch_config3 },
};

void	recalibrate( int pga_gain_index, int ch_GND, int ch_REFH );
void	logical_ch_config_view( void );
void	table_view( int size, int cols, std::function<void(int)> view, std::function<void(void)> linefeed = nullptr );


int main( void )
{
	out.printf( "***** Hello, NAFE13388 UIM board! *****\r\n" );
	out.printf( "---   custom gain & offset sample   ---\r\n" );

	spi.frequency( 1000'000 );
	spi.mode( 1 );

	afe.begin();
	
	out.printf( "part number   = %04lX (revision: %01X)\r\n", afe.part_number(), afe.revision_number() );
	out.printf( "serial number = %llX\r\n", afe.serial_number() );
	out.printf( "die temperature = %f℃\r\n", afe.temperature() );
	
	//
	//	logical channels setting
	//

	for ( auto i = 0U; i < sizeof( chs ) / sizeof( ch_setting_t ); i++ )
		afe.logical_ch_config( i, chs[ i ] );

	out.printf( "\r\nenabled logical channel(s) %2d\r\n", afe.enabled_channels );
	logical_ch_config_view();

//	recalibrate( 0, 14, 15 );
	table_view( 32, 4, []( int v ){ out.printf( "  %8ld @ 0x%04X", afe.reg( v + GAIN_COEFF0 ), v + GAIN_COEFF0 ); }, [](){ out.printf( "\r\n" ); });

	out.printf( "\r\ncount, A1P, A1N, A1P - A1N\r\n" );

//	raw_t			data;
	microvolt_t		data;
	long			count		= 0;
	constexpr float read_delay	= 0.01;

	while ( true )
	{
		out.printf( " %8ld, ", count++ );
		
		for ( auto ch = 0; ch < afe.enabled_channels; ch++ )
		{
			data	= afe.read<microvolt_t>( ch, read_delay );
			out.screen( ch % 2 ? "\033[49m" : "\033[47m" );
			out.printf( " %+8.5lf,", data * 0.000001 );
		}
		out.printf( "\r\n" );

		wait( 0.05 );
	}
}

void recalibrate( int pga_gain_index, int ch_GND, int ch_REFH )
{
	out.printf( "  ..on-board calibration is in progress\r\n" );

	const uint16_t	REF_GND	= 0x0010 | (pga_gain_index << 5);
	const uint16_t	REF_H	= 0x5010 | (pga_gain_index << 5);
	const uint16_t	ch_config1	= (pga_gain_index << 12) | 0x00E4;

	const ch_setting_t	refh	= { REF_H,   ch_config1, 0x2900, 0x0000 };
	const ch_setting_t	refg	= { REF_GND, ch_config1, 0x2900, 0x0000 };

	afe.logical_ch_config( ch_REFH, refh );
	afe.logical_ch_config( ch_GND,  refg );

	raw_t	data_REFH	= afe.read<raw_t>( ch_REFH, 1.1 );
	out.printf( "data_REFH = %8d\r\n", data_REFH );

	raw_t	data_GND	= afe.read<raw_t>( ch_GND,  1.1 );
	out.printf( "data_GND  = %8d\r\n", data_GND  );

	const double	calibrated_gain	= (pow( 2, 23 ) - 1.00) * (2.30 / 25.00) / (double)(data_REFH - data_GND);

	out.printf( "gain adjustment = %8lf (%lfdB)\r\n", calibrated_gain, 20 * log10( calibrated_gain ) );

	const double	current_gain_coeff_value	= (double)afe.reg( GAIN_COEFF0 + pga_gain_index );
	const uint32_t	current_offset_coeff_value	= afe.reg( OFFSET_COEFF0 + CAL_FOR_PGA_0_2 );

	afe.reg( GAIN_COEFF0   + CAL_FOR_PGA_0_2, (uint32_t)(current_gain_coeff_value * calibrated_gain) );
	afe.reg( OFFSET_COEFF0 + CAL_FOR_PGA_0_2, current_offset_coeff_value + data_GND );
	
	const uint16_t	channel_disabling	= (0x1 << ch_GND) | (0x1 << ch_REFH);
	afe.bit_op( CH_CONFIG4, ~channel_disabling, ~channel_disabling );
}

void logical_ch_config_view( void )
{
	uint16_t en_ch_bitmap	= afe.reg( CH_CONFIG4 );
	
	for ( auto channel = 0; channel < 16; channel++ )
	{	
		out.printf( "  logical channel %2d : ", channel );

		if ( en_ch_bitmap & (0x1 << channel) )
		{
			afe.command( channel );
			table_view( 4, 4, []( int v ){ out.printf( "  0x%04X: 0x%04X", v + CH_CONFIG0, afe.reg( v + CH_CONFIG0 ) ); }, [](){ out.printf( "\r\n" ); } );
		}
		else
		{
			out.printf(  "  (disabled)\r\n" );
		}
	}
}

void table_view( int length, int cols, std::function<void(int)> value, std::function<void(void)> linefeed )
{
	const auto	raws	= (int)(length + cols - 1) / cols;
	
	for ( auto i = 0; i < raws; i++  )
	{
		if ( i )
		{
			if ( linefeed )
				linefeed();
			else
				printf( "\r\n" );
		}
		
		for ( auto j = 0; j < cols; j++  )
		{
			auto	index	= i + j * raws;
			
			if ( index < length  )
				value( index );
		}
	}
	
	if ( linefeed )
		linefeed();
	else
		printf( "\r\n" );
}
