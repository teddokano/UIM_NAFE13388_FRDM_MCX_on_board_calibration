/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"afe/NAFE13388_UIM.h"
#include	"coeffs.h"

using enum	NAFE13388_UIM::Register24;
using enum	NAFE13388_UIM::Register16;

void gain_offset_coeff( NAFE13388_UIM &afe, const ref_points &ref )
{
	constexpr double	pga1x_voltage		= 5.0;
	constexpr int		adc_resolution		= 24;
	constexpr double	pga_gain_setting	= 0.2;

	constexpr double	fullscale_voltage	= pga1x_voltage / pga_gain_setting;

	double	fullscale_data		= pow( 2, (adc_resolution - 1) );
	double	ref_data_span		= ref.high.data		- ref.low.data;
	double	ref_voltage_span	= ref.high.voltage	- ref.low.voltage;
	
	double	dv_slope			= ref_data_span / ref_voltage_span;
	double	custom_gain			= dv_slope * (fullscale_voltage / fullscale_data);
	double	custom_offset		= (dv_slope * ref.low.voltage - ref.low.data) / custom_gain;
	
	int32_t	gain_coeff_cal		= afe.reg( GAIN_COEFF0   + ref.cal_index );
	int32_t	offsset_coeff_cal	= afe.reg( OFFSET_COEFF0 + ref.cal_index );
	int32_t	gain_coeff_new		= round( gain_coeff_cal * custom_gain );
	int32_t	offset_coeff_new	= custom_offset - offsset_coeff_cal;

#if 0
	printf( "ref_point_high = %8ld @%6.3lf\r\n", ref.high.data, ref.high.voltage );
	printf( "ref_point_low  = %8ld @%6.3lf\r\n", ref.low.data,  ref.low.voltage  );
	printf( "gain_coeff_new   = %8ld\r\n", gain_coeff_new   );
	printf( "offset_coeff_new = %8ld\r\n", offset_coeff_new );
#endif
	
	afe.reg( GAIN_COEFF0   + ref.coeff_index, gain_coeff_new   );
	afe.reg( OFFSET_COEFF0 + ref.coeff_index, offset_coeff_new );
}

using	ch_setting_t	= const uint16_t[ 4 ];
using 	raw_t			= NAFE13388_UIM::raw_t;

void recalibrate( NAFE13388_UIM &afe, int pga_gain_index, bool use_positive_side, int ch_GND, int ch_REF )
{
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
	
	constexpr	auto	delay_to_read_adc	= 1.1;

	raw_t	data_REF	= afe.read<raw_t>( ch_REF, delay_to_read_adc );
	raw_t	data_GND	= afe.read<raw_t>( ch_GND, delay_to_read_adc );

	constexpr double	pga_gain[]	= { 0.2, 0.4, 0.8, 1, 2, 4, 8, 16 };

	const double	fullscale_voltage	= 5.00 / pga_gain[ pga_gain_index ];
	const double	calibrated_gain		= pow( 2, 23 ) * (reference_source_voltage / fullscale_voltage) / (double)(data_REF - data_GND);

#if 0	
	printf( "data_REF = %8ld\r\n", data_REF );
	printf( "data_GND = %8ld\r\n", data_GND  );
	printf( "gain adjustment = %8lf (%lfdB)\r\n", calibrated_gain, 20 * log10( calibrated_gain ) );
#endif
	
	const double	current_gain_coeff_value	= (double)afe.reg( GAIN_COEFF0 + pga_gain_index );
	const uint32_t	current_offset_coeff_value	= afe.reg( OFFSET_COEFF0 + pga_gain_index );

	afe.reg( GAIN_COEFF0   + pga_gain_index, (uint32_t)(current_gain_coeff_value * calibrated_gain) );
	afe.reg( OFFSET_COEFF0 + pga_gain_index, current_offset_coeff_value + data_GND );

	const uint16_t	channel_disabling	= (0x1 << ch_GND) | (0x1 << ch_REF);
	afe.bit_op( CH_CONFIG4, ~channel_disabling, ~channel_disabling );
}

