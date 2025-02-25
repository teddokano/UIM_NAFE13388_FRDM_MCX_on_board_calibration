/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"r01lib.h"
#include	"afe/NAFE13388_UIM.h"
#include	<math.h>
#include	<array>

#include	"coeffs.h"
#include	"PrintOutput.h"

SPI				spi( D11, D12, D13, D10 );	//	MOSI, MISO, SCLK, CS
NAFE13388_UIM	afe( spi );
PrintOutput		out( "test" );
//PrintOutput	out( nullptr );	//use this line to disable file output

using enum	NAFE13388_UIM::Register16;
using enum	NAFE13388_UIM::Register24;
using enum	NAFE13388_UIM::Command;

using 	raw_t			= NAFE13388_UIM::raw_t;

constexpr int	INPUT_GND			= 0x0010;
constexpr int	INPUT_A1P_SINGLE	= 0x1010;

enum CoeffIndex {
	CAL_FOR_PGA_0_2	= 0,
	CAL_NONE		= 8,
	CAL__5V_NONE,
	CAL_10V_NONE,
	CAL__5V_CUSTOM,
	CAL_10V_CUSTOM,
	CAL_1V5V_NONE,
	CAL_1V5V_CUSTOM,
};


using	ch_setting_t	= const uint16_t[ 4 ];

constexpr ch_setting_t	chs[]	= {
	{ INPUT_A1P_SINGLE, (CAL_NONE        << 12) | 0x0084, 0x2900, 0x0000 },
	{ INPUT_A1P_SINGLE, (CAL_FOR_PGA_0_2 << 12) | 0x0084, 0x2900, 0x0000 },
	{ INPUT_A1P_SINGLE, (CAL__5V_NONE    << 12) | 0x0084, 0x2900, 0x0000 },
	{ INPUT_A1P_SINGLE, (CAL__5V_CUSTOM  << 12) | 0x0084, 0x2900, 0x0000 },
	{ INPUT_A1P_SINGLE, (CAL_10V_NONE    << 12) | 0x0084, 0x2900, 0x0000 },
	{ INPUT_A1P_SINGLE, (CAL_10V_CUSTOM  << 12) | 0x0084, 0x2900, 0x0000 },
	{ INPUT_A1P_SINGLE, (CAL_1V5V_NONE   << 12) | 0x0084, 0x2900, 0x0000 },
	{ INPUT_A1P_SINGLE, (CAL_1V5V_CUSTOM << 12) | 0x0084, 0x2900, 0x0000 },
	{ INPUT_GND       , (CAL_NONE        << 12) | 0x0084, 0x2900, 0x0000 },
	{ INPUT_GND       , (CAL_FOR_PGA_0_2 << 12) | 0x0084, 0x2900, 0x0000 },
	{ INPUT_GND       , (CAL__5V_NONE    << 12) | 0x0084, 0x2900, 0x0000 },
	{ INPUT_GND       , (CAL__5V_CUSTOM  << 12) | 0x0084, 0x2900, 0x0000 },
	{ INPUT_GND       , (CAL_10V_NONE    << 12) | 0x0084, 0x2900, 0x0000 },
	{ INPUT_GND       , (CAL_10V_CUSTOM  << 12) | 0x0084, 0x2900, 0x0000 },
};

void	recalibrate( int pga_gain_index, bool use_positive_side = true, int ch_GND = 14, int ch_REF = 15 );
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
	
	for ( auto i = 0; i < 8; i++ )
		recalibrate( i, false );



	raw_t			data;
	long			count		= 0;
	constexpr float read_delay	= 0.01;

	while ( true )
	{
		out.printf( " %8ld, ", count++ );
		
		for ( auto ch = 0; ch < afe.enabled_channels; ch++ )
		{
			data	= afe.read<raw_t>( ch, read_delay );
			out.screen( ch % 2 ? "\033[49m" : "\033[47m" );
			out.printf( " %8ld,", data );
		}
		out.printf( "\r\n" );
		//out.printf( "\n" );

		wait( 0.05 );
	}
}

void recalibrate( int pga_gain_index, bool use_positive_side, int ch_GND, int ch_REF )
{
	out.printf( "  ..on-board calibration is in progress for gain index: %2d\r\n", pga_gain_index );

	constexpr	auto	low_gain_index	= 4;
	uint16_t			reference_source_selection;
	double				reference_source_voltage;

	if ( pga_gain_index <= low_gain_index )
	{
		reference_source_selection	= 0x5;	//	REFH for low gain
		reference_source_voltage	= 2.30;
	}
	else
	{
		reference_source_selection	= 0x6;	//	REFL for high gain
		reference_source_voltage	= 0.20;
	}

	const uint16_t	REF_GND		= 0x0010  | (pga_gain_index << 5);
	const uint16_t	REF_V		= (reference_source_selection << (use_positive_side ? 12 : 8)) | REF_GND;
	const uint16_t	ch_config1	= (pga_gain_index << 12) | 0x00E4;

	const ch_setting_t	refh	= { REF_V,   ch_config1, 0x2900, 0x0000 };
	const ch_setting_t	refg	= { REF_GND, ch_config1, 0x2900, 0x0000 };

	afe.logical_ch_config( ch_REF, refh );
	afe.logical_ch_config( ch_GND, refg );

	raw_t	data_REF	= afe.read<raw_t>( ch_REF, 1.1 );
	raw_t	data_GND	= afe.read<raw_t>( ch_GND,  1.1 );
	out.printf( "data_REF = %8d\r\n", data_REF );
	out.printf( "data_GND = %8d\r\n", data_GND  );

	constexpr double	pga_gain[]	= { 0.2, 0.4, 0.8, 1, 2, 4, 8, 16 };

	const double	fullscale_voltage	= 5.00 / pga_gain[ pga_gain_index ];
	const double	calibrated_gain		= (pow( 2, 23 ) - 1.00) * (reference_source_voltage / fullscale_voltage) / (double)(data_REF - data_GND);

	out.printf( "gain adjustment = %8lf (%lfdB)\r\n", calibrated_gain, 20 * log10( calibrated_gain ) );

	const double	current_gain_coeff_value	= (double)afe.reg( GAIN_COEFF0 + pga_gain_index );
	const uint32_t	current_offset_coeff_value	= afe.reg( OFFSET_COEFF0 + CAL_FOR_PGA_0_2 );

	afe.reg( GAIN_COEFF0   + CAL_FOR_PGA_0_2, (uint32_t)(current_gain_coeff_value * calibrated_gain) );
	afe.reg( OFFSET_COEFF0 + CAL_FOR_PGA_0_2, current_offset_coeff_value + data_GND );
	
	const uint16_t	channel_disabling	= (0x1 << ch_GND) | (0x1 << ch_REF);
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
			table_view( 4, 4, []( int v ){ out.printf( "  0x%04X　@0x%04X", afe.reg( v + CH_CONFIG0 ), v + CH_CONFIG0 ); }, [](){ out.printf( "\r\n" ); } );
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
