/************************************ <lalVerbatim file="GeneratePulsarSignalCV">
Author: Prix, Reinhard
$Id$
************************************* </lalVerbatim> */

/********************************************************** <lalLaTeX>
\subsection{Module \texttt{GeneratePulsarSignal}
\label{ss:GeneratePulsarSignal.c}

Module to generate subsequent sky-positions.

\subsubsection*{Prototypes}
\input{GeneratePulsarSignalCP}
\idx{NextSkyPosition()}

\subsubsection*{Description}

This module generates a fake pulsar-signal, either for an isolated or a binary pulsar. 

\subsubsection*{Algorithm}

\subsubsection*{Uses}

\subsubsection*{Notes}

\vfill{\footnotesize\input{GeneratePulsarSignalCV}}

******************************************************* </lalLaTeX> */
#include <lal/AVFactories.h>
#include <lal/SeqFactories.h>
#include <lal/RealFFT.h>

#include "GeneratePulsarSignal.h"

/*----------------------------------------------------------------------*/
/* prototypes for internal functions */
static void write_timeSeriesR4 (FILE *fp, REAL4TimeSeries *series, UINT4 set);
static void write_timeSeriesR8 (FILE *fp, REAL8TimeSeries *series, UINT4 set);
/*----------------------------------------------------------------------*/

NRCSID( PULSARSIGNALC, "$Id$");

extern INT4 lalDebugLevel;

void
LALGeneratePulsarSignal (LALStatus *stat, REAL4TimeSeries *signal, PulsarSignalParams *params)
{
  static SpinOrbitCWParamStruc sourceParams;
  static CoherentGW sourceSignal;
  DetectorResponse detector;
  UINT4 SSBduration;
  LIGOTimeGPS time = {0,0};

  INITSTATUS( stat, "LALGeneratePulsarSignal", PULSARSIGNALC );
  ATTATCHSTATUSPTR (stat);

  ASSERT (signal != NULL, stat, PULSARSIGNALH_ENULL, PULSARSIGNALH_MSGENULL);
  ASSERT (signal->data == NULL, stat,  PULSARSIGNALH_ENONULL,  PULSARSIGNALH_MSGENONULL);

  /*----------------------------------------------------------------------
   *
   * First call GenerateSpinOrbitCW() to generate the source-signal
   *
   *----------------------------------------------------------------------*/
  sourceParams.position.system = COORDINATESYSTEM_EQUATORIAL;
  sourceParams.position.longitude = params->pulsar.Alpha;
  sourceParams.position.latitude =  params->pulsar.Delta;
  sourceParams.psi = params->pulsar.psi;
  sourceParams.aPlus = params->pulsar.aPlus;
  sourceParams.aCross = params->pulsar.aCross;
  sourceParams.phi0 = params->pulsar.phi0;
  sourceParams.f0 = params->pulsar.f0;
  sourceParams.f = params->pulsar.f;

  /* if pulsar is in binary-orbit, set binary parameters */
  if (params->orbit)
    {
      sourceParams.orbitEpoch = params->orbit->orbitEpoch;
      sourceParams.omega = params->orbit->omega;
      sourceParams.rPeriNorm = params->orbit->rPeriNorm;
      sourceParams.oneMinusEcc = params->orbit->oneMinusEcc;
      sourceParams.angularSpeed = params->orbit->angularSpeed;
    }
  else
    sourceParams.rPeriNorm = 0.0;		/* this defines an isolated pulsar */

  /* start-time in SSB time */
  TRY (ConvertGPS2SSB (stat->statusPtr, &time, params->startTimeGPS, params), stat);
  sourceParams.epoch = time;

  /* pulsar reference-time in SSB frame !*/
  if (params->pulsar.TRefSSB.gpsSeconds != 0)
    sourceParams.spinEpoch = params->pulsar.TRefSSB;
  else
    sourceParams.spinEpoch = time;	/* use start-time */

  /* sampling-timestep and length for source-parameters */
  sourceParams.deltaT = 60;	/* in seconds; hardcoded default from makefakedata_v2 */
  time = params->startTimeGPS;
  time.gpsSeconds += params->duration;
  TRY (ConvertGPS2SSB (stat->statusPtr, &time, time, params), stat);	 /* convert time to SSB */
  SSBduration = time.gpsSeconds - sourceParams.spinEpoch.gpsSeconds;
  sourceParams.length = (UINT4)( 1.0* SSBduration / sourceParams.deltaT );

  /*
   * finally, call the function to generate the source waveform 
   */
  TRY ( LALGenerateSpinOrbitCW (stat->statusPtr, &sourceSignal, &sourceParams), stat);
  /* check that sampling interval was short enough */
  if ( sourceParams.dfdt > 2.0 )  /* taken from makefakedata_v2 */
    {
      LALPrintError ("GenerateSpinOrbitCW() returned df*dt = %f > 2.0", sourceParams.dfdt);
      ABORT (stat, PULSARSIGNALH_ESAMPLING, PULSARSIGNALH_MSGESAMPLING);
    }

  TRY (PrintGWSignal (stat->statusPtr, &sourceSignal, "signal2.agr"), stat);

  /*----------------------------------------------------------------------
   *
   * Now call the function to translate the source-signal into a (heterodyned)
   * signal at the detector 
   *
   *----------------------------------------------------------------------*/
  /* first set up the detector-response */
  detector.transfer = params->transferFunction;
  detector.site = params->site;
  detector.ephemerides = params->ephemerides;
  /* we need to set the heterodyne epoch in GPS time, but we want to use
   * the pulsar reference-time for that (which is in SSB), so we have to convert it first
   */
  TRY ( ConvertSSB2GPS (stat->statusPtr, &time, params->pulsar.TRefSSB, params), stat);
  detector.heterodyneEpoch = time;
  
  /* ok, but we also need to prepare the output time-series */
  signal->data = LALMalloc (sizeof(*(signal->data)));
  signal->data->length = (UINT4)( params->samplingRate * params->duration );
  signal->data->data = LALMalloc ( signal->data->length * (sizeof(*(signal->data->data))) );
  signal->deltaT = 1.0 / params->samplingRate;
  signal->f0 = params->fHeterodyne;
  signal->epoch = params->startTimeGPS;


  printf ("\nLaenge time-series: %d samples\n", signal->data->length);
  
  TRY ( LALSimulateCoherentGW (stat->statusPtr, signal, &sourceSignal, &detector ), stat );

			       
  /*----------------------------------------------------------------------*/
  /* Free all allocated memory that is not returned */
  TRY (LALSDestroyVectorSequence ( stat->statusPtr, &(sourceSignal.a->data)), stat);
  LALFree ( sourceSignal.a );
  TRY (LALSDestroyVector (stat->statusPtr, &(sourceSignal.f->data ) ), stat);
  LALFree (sourceSignal.f);
  TRY (LALDDestroyVector (stat->statusPtr, &(sourceSignal.phi->data )), stat);
  LALFree (sourceSignal.phi);

  DETATCHSTATUSPTR (stat);
  RETURN (stat);
} /* LALGeneratePulsarSignal() */



/*----------------------------------------------------------------------
 * take time-series as input, convert it into SFTs and add noise if given
 *----------------------------------------------------------------------*/
void
AddSignalToSFTs (LALStatus *stat, SFTVector *outputSFTs, REAL4TimeSeries *signal, SFTParams *params)
{
  UINT4 numSFTs, numSamples;
  UINT4 iSFT;
  REAL8 duration, tFirst, tLast, tt, delay;
  REAL8 Band;
  RealFFTPlan *pfwd = NULL;
  LIGOTimeGPS *timestamps;
  REAL4Vector timeStretch = {0,0};
  UINT4 shift;

  INITSTATUS( stat, "AddSignalToSFTs", PULSARSIGNALC);
  ATTATCHSTATUSPTR( stat );
  
  ASSERT (outputSFTs != NULL, stat, PULSARSIGNALH_ENULL, PULSARSIGNALH_MSGENULL);
  ASSERT (outputSFTs->SFTs == NULL, stat,  PULSARSIGNALH_ENONULL,  PULSARSIGNALH_MSGENONULL);
  ASSERT (signal != NULL, stat, PULSARSIGNALH_ENULL, PULSARSIGNALH_MSGENULL);
  ASSERT (params != NULL, stat, PULSARSIGNALH_ENULL, PULSARSIGNALH_MSGENULL);


  /* NOTE: we have to increase FreqBand slightly to make sure
   * that the resulting number of samples in an SFT is an integer 
   * the user shouldn't care about this...: max diff = 1/Tsft
   */
  numSamples = (UINT4) ceil( 2.0 * params->FreqBand * params->Tsft);	/* round up */
  Band = 1.0 * numSamples / (2.0 * params->Tsft );

  printf ("\nFreqBand = %f\n", params->FreqBand);
  printf ("nsamples = %d\n", numSamples);
  printf ("->Band = %f\n", Band);
  printf ("Tsft = %f\n", params->Tsft);

  /* Prepare FFT: compute plan for FFTW */
  TRY (LALCreateForwardRealFFTPlan(stat->statusPtr, &pfwd, numSamples, 0), stat);

  /* determine number of SFTs to be produced */
  duration = 1.0* signal->data->length * signal->deltaT;
  LALGPStoFloat (stat->statusPtr, &tFirst, &(signal->epoch) );	/* REAL8 start-time in s */
  tLast = tFirst + (duration - params->Tsft);

  if (params->timestamps == NULL)	/* no timestamps: cover total time-series */
    {
      numSFTs = (UINT4)( duration / params->Tsft );
      /* ok, this might seem a bit stupid, but the rest of the code gets clearer
       * if we _always_ work with timestamps. Therefore, we just generate them
       * now if none have been provided by the user
       */
      if ( (timestamps = LALCalloc (1, numSFTs * sizeof (LIGOTimeGPS) )) == NULL) {
	ABORT (stat, PULSARSIGNALH_EMEM, PULSARSIGNALH_MSGEMEM);
      }

      for (iSFT = 0; iSFT < numSFTs; iSFT++)
	{
	  tt = tFirst + iSFT * params->Tsft;
	  LALFloatToGPS (stat->statusPtr, &(timestamps[iSFT]), &tt);
	} /* for iSFT */
    }
  else	/* ok, let's use them timestamps.. */
    {
      timestamps = params->timestamps;
      numSFTs = params->Nsft;
    } /* if timestamps */


  /* reserve memory for output SFTs */
  outputSFTs->length = numSFTs;
  outputSFTs->SFTs = LALCalloc (1, numSFTs * sizeof( SFTtype ) );
  if (outputSFTs->SFTs == NULL) {
    ABORT (stat, PULSARSIGNALH_EMEM, PULSARSIGNALH_MSGEMEM);
  }

  /* main loop */
  for (iSFT = 0; iSFT < numSFTs; iSFT++)
    {
      outputSFTs->SFTs[iSFT].f0 = signal->f0;
      outputSFTs->SFTs[iSFT].deltaF = 1.0 / params->Tsft;

      LALCCreateVector(stat->statusPtr, &(outputSFTs->SFTs[iSFT].data), (UINT4)(1.0*numSamples/2 + 1) );
      BEGINFAIL(stat) {
	UINT4 j;
	for (j=0; j < iSFT; j++) 
	  LALCDestroyVector (stat->statusPtr, &(outputSFTs->SFTs[iSFT].data) );
	LALFree (outputSFTs->SFTs);
	LALDestroyRealFFTPlan(stat->statusPtr, &pfwd);
      } ENDFAIL(stat);

    
      LALGPStoFloat (stat->statusPtr, &tt, &timestamps[iSFT] );
      /* brief consistency-check */
      if (  (tt < tFirst) || (tt > tLast) ) {
	ABORT (stat, PULSARSIGNALH_ETIMEBOUND, PULSARSIGNALH_MSGETIMEBOUND);
      }

      delay = tt - tFirst;
      shift = (UINT4) (delay / signal->deltaT + 0.5);	/* round properly */
      
      timeStretch.length = numSamples;    
      timeStretch.data = signal->data->data + shift;		/* point to the right sample-bin */

      /* the central step: FFT the ith time-stretch into an SFT-slot */
      LALForwardRealFFT (stat->statusPtr, outputSFTs->SFTs[iSFT].data, &timeStretch, pfwd);
      BEGINFAIL(stat) {
	UINT4 j;
	for (j=0; j < iSFT; j++) 
	  LALCDestroyVector (stat->statusPtr, &(outputSFTs->SFTs[iSFT].data) );
	LALFree (outputSFTs->SFTs);
	LALDestroyRealFFTPlan(stat->statusPtr, &pfwd);
      } ENDFAIL(stat);

      /* Now set the ACTUAL timestamp corresponding to the shift ! */
      delay = 1.0 * shift * signal->deltaT;
      tt = tFirst + delay;
      LALFloatToGPS (stat->statusPtr, &(outputSFTs->SFTs[iSFT].epoch), &tt );

    } /* for iSFT < numSFTs */ 


  /* clean up */
  LALDestroyRealFFTPlan(stat->statusPtr, &pfwd);
  /* did we get timestamps or did we make them? */
  if (params->timestamps == NULL)
    LALFree (timestamps);

  DETATCHSTATUSPTR( stat );
  RETURN (stat);

} /* AddSignalToSFTs() */




/*----------------------------------------------------------------------
 * convert earth-frame GPS time into barycentric-frame SSB time for given source
 *----------------------------------------------------------------------*/
void
ConvertGPS2SSB (LALStatus* stat, LIGOTimeGPS *SSBout, LIGOTimeGPS GPSin, PulsarSignalParams *params)
{
  EarthState earth;
  EmissionTime emit;
  BarycenterInput baryinput;

  INITSTATUS( stat, "ConvertGPS2SSB", PULSARSIGNALC );
  ATTATCHSTATUSPTR (stat);

  ASSERT (SSBout != NULL, stat,  PULSARSIGNALH_ENULL,  PULSARSIGNALH_MSGENULL);
  ASSERT (params != NULL, stat,  PULSARSIGNALH_ENULL,  PULSARSIGNALH_MSGENULL);

  baryinput.site = *(params->site);
  /* account for a quirk in LALBarycenter(): -> see documentation of type BarycenterInput */
  baryinput.site.location[0] /= LAL_C_SI;
  baryinput.site.location[1] /= LAL_C_SI;
  baryinput.site.location[2] /= LAL_C_SI;
  baryinput.alpha = params->pulsar.Alpha;
  baryinput.delta = params->pulsar.Delta;
  baryinput.dInv = 0.e0;	/* following makefakedata_v2 */

  baryinput.tgps = GPSin;

  TRY (LALBarycenterEarth(stat->statusPtr, &earth, &GPSin, params->ephemerides), stat);
  TRY (LALBarycenter(stat->statusPtr, &emit, &baryinput, &earth), stat);

  *SSBout = emit.te;

  DETATCHSTATUSPTR (stat);
  RETURN (stat);

} /* ConvertGPS2SSB() */


/*----------------------------------------------------------------------
 * convert  barycentric frame SSB time into earth-frame GPS time
 *
 * NOTE: this uses simply the inversion-routine used in the original
 *       makefakedata_v2
 *----------------------------------------------------------------------*/
void
ConvertSSB2GPS (LALStatus *stat, LIGOTimeGPS *GPSout, LIGOTimeGPS SSBin, PulsarSignalParams *params)
{
  LIGOTimeGPS SSBofguess;
  LIGOTimeGPS GPSguess;
  INT4 iterations, E9=1000000000;
  INT8 delta, guess;

  INITSTATUS( stat, "ConvertSSB2GPS", PULSARSIGNALC );
  ATTATCHSTATUSPTR (stat);


  /* 
   * To start root finding, use SSBpulsarparams as guess 
   * (not off by more than 400 secs! 
   */
  GPSguess = SSBin;

  /* now find GPS time corresponding to SSBin by iterations */
  for (iterations = 0; iterations < 100; iterations++) 
    {
      /* find SSB time of guess */
      TRY ( ConvertGPS2SSB (stat->statusPtr, &SSBofguess, GPSguess, params), stat);

      /* compute difference between that and what we want */
      delta  = SSBin.gpsSeconds;
      delta -= SSBofguess.gpsSeconds;
      delta *= E9;
      delta += SSBin.gpsNanoSeconds;
      delta -= SSBofguess.gpsNanoSeconds;
      
      /* break if we've converged: let's be strict to < 1 ns ! */
      if (delta == 0)
	break;

      /* use delta to make next guess */
      guess  = GPSguess.gpsSeconds;
      guess *= E9;
      guess += GPSguess.gpsNanoSeconds;
      guess += delta;

      GPSguess.gpsSeconds = guess / E9;
      guess -= GPSguess.gpsSeconds * E9;	/* get ns remainder */
      GPSguess.gpsNanoSeconds = guess;

    } /* for iterations < 100 */

  /* check for convergence of root finder */
  if (iterations == 100) {
    ABORT ( stat, PULSARSIGNALH_ESSBCONVERT, PULSARSIGNALH_MSGESSBCONVERT);
  }

  /* Now that we've found the GPS time that corresponds to the given SSB time */
  *GPSout = GPSguess;
  
  
  DETATCHSTATUSPTR (stat);
  RETURN (stat);

} /* ConvertSSB2GPS() */

/*----------------------------------------------------------------------
 * export a REAL4 time-series in an xmgrace graphics file 
 *----------------------------------------------------------------------*/
void
LALPrintR4TimeSeries (LALStatus *stat, REAL4TimeSeries *series, const CHAR *fname)
{
  FILE *fp = NULL;
  UINT4 set;
  CHAR *xmgrHeader = 
    "@version 50103\n"
    "@xaxis label \"T (sec)\"\n"
    "@yaxis label \"A\"\n";

  /* Set up shop. */
  INITSTATUS( stat, "PrintR4TimeSeries", PULSARSIGNALC);
  ATTATCHSTATUSPTR( stat );

  fp = fopen (fname, "w");

  if( !fp ) {
    ABORT (stat, PULSARSIGNALH_ESYS, PULSARSIGNALH_MSGESYS);
  }
  
  fprintf (fp, xmgrHeader);
  fprintf (fp, "@title \"REAL4 Time-Series: %s\"\n", series->name);
  fprintf (fp, "@subtitle \"GPS-start: %f, f0 = %e\"\n", 
	   1.0*series->epoch.gpsSeconds + series->epoch.gpsNanoSeconds / 1.0e9, series->f0);

  set = 0;

  write_timeSeriesR4 (fp, series, set);
      
  fclose(fp);

  DETATCHSTATUSPTR( stat );
  RETURN (stat);

} /* PrintR4TimeSeries() */

/*----------------------------------------------------------------------
 * write a CoherentGW type signal into an xmgrace file
 *----------------------------------------------------------------------*/
void
PrintGWSignal (LALStatus *stat, CoherentGW *signal, const CHAR *fname)
{
  FILE *fp;
  UINT4 set;
  CHAR *xmgrHeader = 
    "@version 50103\n"
    "@xaxis label \"T (sec)\"\n"
    "@yaxis label \"A\"\n";


  INITSTATUS( stat, "PrintGWSignal", PULSARSIGNALC);
  ATTATCHSTATUSPTR( stat );

  fp = fopen (fname, "w");

  if( !fp ) {
    ABORT (stat, PULSARSIGNALH_ESYS, PULSARSIGNALH_MSGESYS);
  }

  fprintf (fp, xmgrHeader);
  fprintf (fp, "@title \"GW source signal\"\n");
  fprintf (fp, "@subtitle \"position: (alpha,delta) = (%f, %f), psi = %f\"\n", 
	   signal->position.longitude, signal->position.latitude, signal->psi);

  set = 0;
  write_timeSeriesR4 (fp, signal->f, set++);
  write_timeSeriesR8 (fp, signal->phi, set++);

  fclose (fp);

  DETATCHSTATUSPTR( stat );
  RETURN (stat);

} /* PrintGWSignal() */


/* internal helper-function */
void
write_timeSeriesR4 (FILE *fp, REAL4TimeSeries *series, UINT4 set)
{
  REAL8 timestamp; 
  UINT4 i;


  if (series == NULL)
    {
      printf ("\nset %d is empty\n", set);
      return; 
    }

  /* Print set header. */
  fprintf( fp, "\n@target G0.S%d\n@type xy\n", set);
  fprintf( fp, "@s%d color (0,0,0)\n", set );

  timestamp = 1.0*series->epoch.gpsSeconds + series->epoch.gpsNanoSeconds * 1.0e-9;

  for( i = 0; i < series->data->length; i++)
    {
      fprintf( fp, "%16.9f %e\n", timestamp, series->data->data[i] );
      timestamp += series->deltaT;
    }

  return;

} /* write_timeSeriesR4() */

void
write_timeSeriesR8 (FILE *fp, REAL8TimeSeries *series, UINT4 set)
{
  REAL8 timestamp; 
  UINT4 i;
  /* Print set header. */
  fprintf( fp, "\n@target G0.S%d\n@type xy\n", set);
  fprintf( fp, "@s%d color (0,0,0)\n", set );

  timestamp = 1.0*series->epoch.gpsSeconds + series->epoch.gpsNanoSeconds * 1.0e-9;

  for( i = 0; i < series->data->length; i++)
    {
      fprintf( fp, "%f %e\n", timestamp, series->data->data[i] );
      timestamp += series->deltaT;
    }

  return;

} /* write_timeSeriesR4() */




void 
write_SFT (LALStatus *stat, SFTtype *sft, const CHAR *fname)
{
  FILE *fp;
  REAL4 rpw,ipw;
  INT4 i;
  REAL8 freqBand;

  struct headertag {
    REAL8 endian;
    INT4  gps_sec;
    INT4  gps_nsec;
    REAL8 tbase;
    INT4  firstfreqindex;
    INT4  nsamples;
  } header;

  INITSTATUS( stat, "write_SFT", PULSARSIGNALC);
  ATTATCHSTATUSPTR( stat );

  fp = fopen (fname, "w");

  if( !fp ) {
    ABORT (stat, PULSARSIGNALH_ESYS, PULSARSIGNALH_MSGESYS);
  }

  header.endian=1.0;
  header.gps_sec = sft->epoch.gpsSeconds;
  header.gps_nsec = sft->epoch.gpsNanoSeconds;

  freqBand = (sft->data->length -1 ) * sft->deltaF;
  header.tbase = (REAL4) ( (sft->data->length - 1.0) / freqBand);
  header.firstfreqindex = (INT4)(sft->f0 * header.tbase + 0.5);
  header.nsamples=sft->data->length - 1;

  printf ("\nwrite_SFT():\nfreqBand = %f\n", freqBand);
  printf ("f0 = %f\ndf = %f\nnsamples = %d\n", sft->f0, sft->deltaF, header.nsamples);
  printf ("Tsft = %f\n", header.tbase);  

  /* write header */
  if (fwrite( (void*)&header, sizeof(header), 1, fp) != 1) {
    ABORT (stat, PULSARSIGNALH_ESYS, PULSARSIGNALH_MSGESYS);
  }

  for (i=0; i < header.nsamples; i++)
    {
      rpw = sft->data->data[i].re;
      ipw = sft->data->data[i].im;

      if (fwrite((void*)&rpw, sizeof(REAL4), 1, fp) != 1) { 
	ABORT (stat, PULSARSIGNALH_ESYS, PULSARSIGNALH_MSGESYS); 
      }
      if (fwrite((void*)&ipw, sizeof(REAL4),1,fp) != 1) {  
	ABORT (stat, PULSARSIGNALH_ESYS, PULSARSIGNALH_MSGESYS); 
      }
        
    } /* for i < nsamples */
  
  fclose(fp);

  DETATCHSTATUSPTR( stat );
  RETURN (stat);
  
} /* write_SFT() */

/* little debug-function: compare two sft's and print relative errors */
void
compare_SFTs (COMPLEX8Vector *sft1, COMPLEX8Vector *sft2)
{
  UINT4 i;
  REAL4 maxdiff = 0;

  if (sft1->length != sft2->length) 
    {
      printf ("\ncompare_SFTs(): lengths differ! %d != %d\n", sft1->length, sft2->length);
      return;
    }
#define MAX(x,y) (x > y ? x : y)

  for (i=0; i < sft1->length; i++)
    {
      maxdiff = MAX ( maxdiff, abs ( 2.0*(sft1->data[i].re - sft2->data[i].re) / (sft1->data[i].re + sft2->data[i].re) ) );
      maxdiff = MAX ( maxdiff, abs ( 2.0*(sft1->data[i].im - sft2->data[i].im) / (sft1->data[i].im + sft2->data[i].im) ) );
    }

  printf ("\ncompare_SFTs(): maximal relative difference: %f\n", maxdiff);

} /* compare_SFTs() */
