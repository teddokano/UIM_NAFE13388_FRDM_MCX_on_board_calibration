/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef NAFE_COEFFS_H
#define NAFE_COEFFS_H

#include	"r01lib.h"
#include	"afe/NAFE13388_UIM.h"
#include	<math.h>

typedef struct	_point	{
	double	voltage;
	int32_t	data;
} point;

typedef struct	_ref_points	{
	int		coeff_index;
	point	high;
	point	low;
	int		cal_index;
} ref_points;

void	gain_offset_coeff( NAFE13388_UIM &afe, const ref_points &ref );
void	recalibrate( NAFE13388_UIM &afe, int pga_gain_index, bool use_positive_side = true, int ch_GND = 14, int ch_REF = 15 );

#endif	//	NAFE_COEFFS_H
