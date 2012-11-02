/*
 * Copyright (C) 2008 J. Creighton, S. Fairhurst, B. Krishnan, L. Santamaria, D. Keppel
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with with program; see the file COPYING. If not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *  MA  02111-1307  USA
 */

#include <math.h>

#include <gsl/gsl_const.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_odeiv.h>

#define LAL_USE_OLD_COMPLEX_STRUCTS
#include <lal/LALSimInspiral.h>
#define LAL_USE_COMPLEX_SHORT_MACROS
#include <lal/LALAdaptiveRungeKutta4.h>
#include <lal/LALComplex.h>
#include <lal/LALConstants.h>
#include <lal/LALStdlib.h>
#include <lal/TimeSeries.h>
#include <lal/Units.h>

#include "LALSimInspiraldEnergyFlux.c"
#include "LALSimInspiralPNCoefficients.c"
#include "check_series_macros.h"

#ifdef __GNUC__
#define UNUSED __attribute__ ((unused))
#else
#define UNUSED
#endif

/* v at isco */
#define LALSIMINSPIRAL_T1_VISCO 1.L/sqrt(6.L)
/* use error codes above 1024 to avoid conflicts with GSL */
#define LALSIMINSPIRAL_T1_TEST_ISCO 1025
/* Number of variables used for these waveforms */
#define LALSIMINSPIRAL_NUM_T1_VARIABLES 2
/* absolute and relative tolerance for adaptive Runge-Kutta ODE integrator */
#define LALSIMINSPIRAL_T1_ABSOLUTE_TOLERANCE 1.e-12
#define LALSIMINSPIRAL_T1_RELATIVE_TOLERANCE 1.e-12

/**
 * This structure contains the intrinsic parameters and post-newtonian 
 * co-efficients for the denergy/dv and flux expansions. 
 * These are computed by XLALSimInspiralTaylorT1Setup routine.
 */

typedef struct
{
	/* Angular velocity coefficient */
	REAL8 av;

	/* Taylor expansion coefficents in domega/dt */
	expnCoeffsdEnergyFlux akdEF;

	/* symmetric mass ratio and total mass */
	REAL8 nu,m,mchirp;
} expnCoeffsTaylorT1;

typedef REAL8 (SimInspiralTaylorT1Energy)(
	REAL8 v, /**< post-Newtonian parameter */
	expnCoeffsdEnergyFlux *ak
);

typedef REAL8 (SimInspiralTaylorT1dEnergy)(
	REAL8 v, /**< post-Newtonian parameter */
	expnCoeffsdEnergyFlux *ak
);

typedef REAL8 (SimInspiralTaylorT1Flux)(
	REAL8 v, /**< post-Newtonian parameter */
	expnCoeffsdEnergyFlux *ak
);

/**
 * This strucuture contains pointers to the functions for calculating
 * the post-newtonian terms at the desired order. They can be set by
 * XLALSimInspiralTaylorT1Setup by passing an appropriate PN order. 
 */

typedef struct
tagexpnFuncTaylorT1
{
	SimInspiralTaylorT1Energy *energy;
	SimInspiralTaylorT1dEnergy *dEnergy;
	SimInspiralTaylorT1Flux *flux;
} expnFuncTaylorT1;

typedef struct
{
	REAL8 (*dEdv)(REAL8 v, expnCoeffsdEnergyFlux *ak);
	REAL8 (*flux)(REAL8 v, expnCoeffsdEnergyFlux *ak);
	expnCoeffsTaylorT1 ak;
}XLALSimInspiralTaylorT1PNEvolveOrbitParams;

/** 
 * This function is used in the call to the integrator.
 */
static int 
XLALSimInspiralTaylorT1PNEvolveOrbitIntegrand(double UNUSED t, const double y[], double ydot[], void *params)
{
	XLALSimInspiralTaylorT1PNEvolveOrbitParams* p = (XLALSimInspiralTaylorT1PNEvolveOrbitParams*)params;
	ydot[0] = -p->ak.av*p->flux(y[0],&p->ak.akdEF)/p->dEdv(y[0],&p->ak.akdEF);
	ydot[1] = y[0]*y[0]*y[0]*p->ak.av;
	return GSL_SUCCESS;
}


/**
 * This function is used in the call to the integrator to determine the stopping condition.
 */
static int
XLALSimInspiralTaylorT1StoppingTest(double UNUSED t, const double y[], double UNUSED ydot[], void UNUSED *params)
{
	if (y[0] >= LALSIMINSPIRAL_T1_VISCO) /* frequency above ISCO */
		return LALSIMINSPIRAL_T1_TEST_ISCO;
	else /* Step successful, continue integrating */
		return GSL_SUCCESS;
}


/**
 * Set up the expnCoeffsTaylorT1 and expnFuncTaylorT1 structures for
 * generating a TaylorT1 waveform and select the post-newtonian
 * functions corresponding to the desired order. 
 *
 * Inputs given in SI units.
 */
static int 
XLALSimInspiralTaylorT1Setup(
    expnCoeffsTaylorT1 *ak,		/**< coefficients for TaylorT1 evolution [modified] */
    expnFuncTaylorT1 *f,		/**< functions for TaylorT1 evolution [modified] */
    REAL8 m1,				/**< mass of companion 1 */
    REAL8 m2,				/**< mass of companion 2 */
    int O				/**< twice post-Newtonian order */
)
{
    ak->m = m1 + m2;
    REAL8 mu = m1 * m2 / ak->m;
    ak->nu = mu/ak->m;
    ak->mchirp = ak->m * pow(ak->nu, 0.6);
    /* convert mchirp from kg to s */
    ak->mchirp *= LAL_G_SI / pow(LAL_C_SI, 3.0);

    /* Angular velocity co-efficient */
    ak->av = pow(LAL_C_SI, 3.0)/(LAL_G_SI*ak->m);

    /* Taylor co-efficients for E(v). */
    ak->akdEF.ETaN = XLALSimInspiralEnergy_0PNCoeff(ak->nu);
    ak->akdEF.ETa1 = XLALSimInspiralEnergy_2PNCoeff(ak->nu);
    ak->akdEF.ETa2 = XLALSimInspiralEnergy_4PNCoeff(ak->nu);
    ak->akdEF.ETa3 = XLALSimInspiralEnergy_6PNCoeff(ak->nu);

    /* Taylor co-efficients for dE(v)/dv. */
    ak->akdEF.dETaN = 2.0 * ak->akdEF.ETaN;
    ak->akdEF.dETa1 = 2.0 * ak->akdEF.ETa1;
    ak->akdEF.dETa2 = 3.0 * ak->akdEF.ETa2;
    ak->akdEF.dETa3 = 4.0 * ak->akdEF.ETa3;
    
    /* Taylor co-efficients for flux. */
    ak->akdEF.FTaN = XLALSimInspiralTaylorT1Flux_0PNCoeff(ak->nu);
    ak->akdEF.FTa2 = XLALSimInspiralTaylorT1Flux_2PNCoeff(ak->nu);
    ak->akdEF.FTa3 = XLALSimInspiralTaylorT1Flux_3PNCoeff(ak->nu);
    ak->akdEF.FTa4 = XLALSimInspiralTaylorT1Flux_4PNCoeff(ak->nu);
    ak->akdEF.FTa5 = XLALSimInspiralTaylorT1Flux_5PNCoeff(ak->nu);
    ak->akdEF.FTa6 = XLALSimInspiralTaylorT1Flux_6PNCoeff(ak->nu);
    ak->akdEF.FTl6 = XLALSimInspiralTaylorT1Flux_6PNLogCoeff(ak->nu);
    ak->akdEF.FTa7 = XLALSimInspiralTaylorT1Flux_7PNCoeff(ak->nu);

    switch (O)
    {
        case 0:
            f->energy = &XLALSimInspiralEt0;
            f->dEnergy = &XLALSimInspiraldEt0;
            f->flux = &XLALSimInspiralFt0;
            break;
        case 1:
            XLALPrintError("XLAL Error - %s: PN approximant not supported for PN order %d\n", __func__,O);
            XLAL_ERROR(XLAL_EINVAL);
            break;
        case 2:
            f->energy = &XLALSimInspiralEt2;
            f->dEnergy = &XLALSimInspiraldEt2;
            f->flux = &XLALSimInspiralFt2;
            break;
        case 3:
            f->energy = &XLALSimInspiralEt2;
            f->dEnergy = &XLALSimInspiraldEt2;
            f->flux = &XLALSimInspiralFt3;
            break;
        case 4:
            f->energy = &XLALSimInspiralEt4;
            f->dEnergy = &XLALSimInspiraldEt4;
            f->flux = &XLALSimInspiralFt4;
            break;
        case 5:
            f->energy = &XLALSimInspiralEt4;
            f->dEnergy = &XLALSimInspiraldEt4;
            f->flux = &XLALSimInspiralFt5;
            break;
        case 6:
            f->energy = &XLALSimInspiralEt6;
            f->dEnergy = &XLALSimInspiraldEt6;
            f->flux = &XLALSimInspiralFt6;
            break;
        case 7:
        case -1:
            f->energy = &XLALSimInspiralEt6;
            f->dEnergy = &XLALSimInspiraldEt6;
            f->flux = &XLALSimInspiralFt7;
            break;
        case 8:
            XLALPrintError("XLAL Error - %s: PN approximant not supported for PN order %d\n", __func__,O);
            XLAL_ERROR(XLAL_EINVAL);
            break;
        default:
            XLALPrintError("XLAL Error - %s: Unknown PN order in switch\n", __func__);
            XLAL_ERROR(XLAL_EINVAL);
    }
  
  return 0;
}


/**
 * Evolves a post-Newtonian orbit using the Taylor T1 method.
 *
 * See Section IIIA: Alessandra Buonanno, Bala R Iyer, Evan
 * Ochsner, Yi Pan, and B S Sathyaprakash, "Comparison of post-Newtonian
 * templates for compact binary inspiral signals in gravitational-wave
 * detectors", Phys. Rev. D 80, 084043 (2009), arXiv:0907.0700v1
 */
int XLALSimInspiralTaylorT1PNEvolveOrbit(
		REAL8TimeSeries **V,   /**< post-Newtonian parameter [returned] */
		REAL8TimeSeries **phi, /**< orbital phase [returned] */
		REAL8 phic,            /**< coalescence phase */
		REAL8 deltaT,          /**< sampling interval */
		REAL8 m1,              /**< mass of companion 1 */
		REAL8 m2,              /**< mass of companion 2 */
		REAL8 f_min,           /**< start frequency */
		int O                  /**< twice post-Newtonian order */
		)
{
	double lengths;
	int len, intreturn, idx;
	XLALSimInspiralTaylorT1PNEvolveOrbitParams params;
	ark4GSLIntegrator *integrator = NULL;
	expnFuncTaylorT1 expnfunc;
	expnCoeffsTaylorT1 ak;

	if(XLALSimInspiralTaylorT1Setup(&ak,&expnfunc,m1,m2,O))
		XLAL_ERROR(XLAL_EFUNC);

	params.flux=expnfunc.flux;
	params.dEdv=expnfunc.dEnergy;
	params.ak=ak;

	LIGOTimeGPS tc = LIGOTIMEGPSZERO;
	double yinit[LALSIMINSPIRAL_NUM_T1_VARIABLES];
	REAL8Array *yout;

	/* length estimation (Newtonian) */
	/* since integration is adaptive, we could use a better estimate */
	lengths = (5.0/256.0) * pow(LAL_PI, -8.0/3.0) * pow(f_min * ak.mchirp, -5.0/3.0) / f_min;

	yinit[0] = cbrt(LAL_PI * LAL_G_SI * ak.m * f_min) / LAL_C_SI;
	yinit[1] = 0.;

	/* initialize the integrator */
	integrator = XLALAdaptiveRungeKutta4Init(LALSIMINSPIRAL_NUM_T1_VARIABLES,
		XLALSimInspiralTaylorT1PNEvolveOrbitIntegrand,
		XLALSimInspiralTaylorT1StoppingTest,
		LALSIMINSPIRAL_T1_ABSOLUTE_TOLERANCE, LALSIMINSPIRAL_T1_RELATIVE_TOLERANCE);
	if( !integrator )
	{
		XLALPrintError("XLAL Error - %s: Cannot allocate integrator\n", __func__);
		XLAL_ERROR(XLAL_EFUNC);
	}

	/* stop the integration only when the test is true */
	integrator->stopontestonly = 1;

	/* run the integration */
	len = XLALAdaptiveRungeKutta4Hermite(integrator, (void *) &params, yinit, 0.0, lengths, deltaT, &yout);

	intreturn = integrator->returncode;
	XLALAdaptiveRungeKutta4Free(integrator);

	if (!len) 
	{
		XLALPrintError("XLAL Error - %s: integration failed with errorcode %d.\n", __func__, intreturn);
		XLAL_ERROR(XLAL_EFUNC);
	}

	/* Adjust tStart so last sample is at time=0 */
	XLALGPSAdd(&tc, -1.0*(len-1)*deltaT);

	/* allocate memory for output vectors */
	*V = XLALCreateREAL8TimeSeries( "ORBITAL_VELOCITY_PARAMETER", &tc, 0., deltaT, &lalDimensionlessUnit, len);
	*phi = XLALCreateREAL8TimeSeries( "ORBITAL_PHASE", &tc, 0., deltaT, &lalDimensionlessUnit, len);

	if ( !V || !phi )
	{
		XLALDestroyREAL8Array(yout);
		XLAL_ERROR(XLAL_EFUNC);
	}

	/* Compute phase shift to get desired value phic in last sample */
	/* phi here is the orbital phase = 1/2 * GW phase.
	 * End GW phase specified on command line.
	 * Adjust phase so phi = phic/2 at the end */

	phic /= 2.;
	phic -= yout->data[3*len-1];

	/* Copy time series of dynamical variables */
	/* from yout array returned by integrator to output time series */
	/* Note the first 'len' members of yout are the time steps */
	for( idx = 0; idx < len; idx++ )
	{	
		(*V)->data->data[idx]	= yout->data[len+idx];
		(*phi)->data->data[idx]	= yout->data[2*len+idx] + phic;
	}

	XLALDestroyREAL8Array(yout);

	return (int)(*V)->data->length;
}


/**
 * Driver routine to compute the post-Newtonian inspiral waveform.
 *
 * This routine allows the user to specify different pN orders
 * for phasing calcuation vs. amplitude calculations.
 */
int XLALSimInspiralTaylorT1PNGenerator(
		REAL8TimeSeries **hplus,  /**< +-polarization waveform */
	       	REAL8TimeSeries **hcross, /**< x-polarization waveform */
	       	REAL8 phic,               /**< coalescence phase */
	       	REAL8 v0,                 /**< tail-term gauge choice (default = 1) */
	       	REAL8 deltaT,             /**< sampling interval */
	       	REAL8 m1,                 /**< mass of companion 1 */
	       	REAL8 m2,                 /**< mass of companion 2 */
	       	REAL8 f_min,              /**< start frequency */
	       	REAL8 r,                  /**< distance of source */
	       	REAL8 i,                  /**< inclination of source (rad) */
	       	int amplitudeO,           /**< twice post-Newtonian amplitude order */
	       	int phaseO                /**< twice post-Newtonian phase order */
		)
{
	REAL8TimeSeries *V;
	REAL8TimeSeries *phi;
	int status;
	int n;
	n = XLALSimInspiralTaylorT1PNEvolveOrbit(&V, &phi, phic, deltaT, m1, m2, f_min, phaseO);
	if ( n < 0 )
		XLAL_ERROR(XLAL_EFUNC);
	status = XLALSimInspiralPNPolarizationWaveforms(hplus, hcross, V, phi, v0, m1, m2, r, i, amplitudeO);
	XLALDestroyREAL8TimeSeries(phi);
	XLALDestroyREAL8TimeSeries(V);
	if ( status < 0 )
		XLAL_ERROR(XLAL_EFUNC);
	return n;
}


/**
 * Driver routine to compute the post-Newtonian inspiral waveform.
 *
 * This routine uses the same pN order for phasing and amplitude
 * (unless the order is -1 in which case the highest available
 * order is used for both of these -- which might not be the same).
 *
 * Constant log term in amplitude set to 1.  This is a gauge choice.
 */
int XLALSimInspiralTaylorT1PN(
		REAL8TimeSeries **hplus,  /**< +-polarization waveform */
	       	REAL8TimeSeries **hcross, /**< x-polarization waveform */
	       	REAL8 phic,               /**< coalescence phase */
	       	REAL8 deltaT,             /**< sampling interval */
	       	REAL8 m1,                 /**< mass of companion 1 */
	       	REAL8 m2,                 /**< mass of companion 2 */
	       	REAL8 f_min,              /**< start frequency */
	       	REAL8 r,                  /**< distance of source */
	       	REAL8 i,                  /**< inclination of source (rad) */
	       	int O                     /**< twice post-Newtonian order */
		)
{
	/* set v0 to default value 1 */
	return XLALSimInspiralTaylorT1PNGenerator(hplus, hcross, phic, 1.0, deltaT, m1, m2, f_min, r, i, O, O);
}


/**
 * Driver routine to compute the restricted post-Newtonian inspiral waveform.
 *
 * This routine computes the phasing to the specified order, but
 * only computes the amplitudes to the Newtonian (quadrupole) order.
 *
 * Constant log term in amplitude set to 1.  This is a gauge choice.
 */
int XLALSimInspiralTaylorT1PNRestricted(
		REAL8TimeSeries **hplus,  /**< +-polarization waveform */
	       	REAL8TimeSeries **hcross, /**< x-polarization waveform */
	       	REAL8 phic,               /**< coalescence phase */
	       	REAL8 deltaT,             /**< sampling interval */
	       	REAL8 m1,                 /**< mass of companion 1 */
	       	REAL8 m2,                 /**< mass of companion 2 */
	       	REAL8 f_min,              /**< start frequency */
	       	REAL8 r,                  /**< distance of source */
	       	REAL8 i,                  /**< inclination of source (rad) */
	       	int O                     /**< twice post-Newtonian phase order */
		)
{
	/* use Newtonian order for amplitude */
	/* set v0 to default value 1 */
	return XLALSimInspiralTaylorT1PNGenerator(hplus, hcross, phic, 1.0, deltaT, m1, m2, f_min, r, i, 0, O);
}


#if 0
#include <lal/PrintFTSeries.h>
#include <lal/PrintFTSeries.h>
extern int lalDebugLevel;
int main(void)
{
	LIGOTimeGPS tc = { 888888888, 222222222 };
	REAL8 phic = 1.0;
	REAL8 deltaT = 1.0/16384.0;
	REAL8 m1 = 1.4*LAL_MSUN_SI;
	REAL8 m2 = 1.4*LAL_MSUN_SI;
	REAL8 r = 1e6*LAL_PC_SI;
	REAL8 i = 0.5*LAL_PI;
	REAL8 f_min = 100.0;
	int O = -1;
	REAL8TimeSeries *hplus;
	REAL8TimeSeries *hcross;
	lalDebugLevel = 7;
	XLALSimInspiralTaylorT1PN(&hplus, &hcross, &tc, phic, deltaT, m1, m2, f_min, r, i, O);
	LALDPrintTimeSeries(hplus, "hp.dat");
	LALDPrintTimeSeries(hcross, "hc.dat");
	XLALDestroyREAL8TimeSeries(hplus);
	XLALDestroyREAL8TimeSeries(hcross);
	LALCheckMemoryLeaks();
	return 0;
}
#endif
