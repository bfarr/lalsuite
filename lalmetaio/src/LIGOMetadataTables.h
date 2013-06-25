/*
*  Copyright (C) 2007 Andres C. Rodriguez, Badri Krishnan, Sukanta Bose, Alexander Dietz, Duncan Brown, Kipp Cannon, Lisa M. Goggin, Patrick Brady, Robert Adam Mercer, Saikat Ray-Majumder, Anand Sengupta, Stephen Fairhurst, Xavier Siemens, Sean Seader, Thomas Cokelaer
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

/*-----------------------------------------------------------------------
 *
 * File Name: LIGOMetadataTables.h
 *
 * Author: Brown, D. A.
 *
 *-----------------------------------------------------------------------
 */

/**
\author Brown, D. A.
\file
\ingroup lalmetaio
\brief Provides data type definitions for the LIGO metadata database tables.

Only columns that are available for the user defined values are available in the
structures. For example the \c process_id column in the
\c sngl_inspiral table is not in the corresponding LAL structure as it
is generated by the database on insertion of events, not by the user.

\heading{Synopsis}
\code
#include <lal/LIGOMetadataTables.h>
\endcode


*/

#include <lal/LALStdlib.h>

#ifndef _LIGOMETADATATABLES_H
#define _LIGOMETADATATABLES_H

#if defined(__cplusplus)
extern "C" {
#elif 0
} /* so that editors will match preceding brace */
#endif

/* Defines for lengths of database table char arrays       */
/* one longer than as defined in the SQL tables to allow   */
/* strage of the terminating null need for the C %s format */
#define LIGOMETA_PROGRAM_MAX 65
#define LIGOMETA_VERSION_MAX 65
#define LIGOMETA_CVS_REPOSITORY_MAX 257
#define LIGOMETA_COMMENT_MAX 241
#define LIGOMETA_NODE_MAX 65
#define LIGOMETA_USERNAME_MAX 65
#define LIGOMETA_DOMAIN_MAX 64
#define LIGOMETA_IFOS_MAX 13
#define LIGOMETA_PARAM_MAX 33
#define LIGOMETA_TYPE_MAX 17
#define LIGOMETA_VALUE_MAX 1025
#define LIGOMETA_NAME_MAX 257
#define LIGOMETA_STRING_MAX 257
#define LIGOMETA_IFO_MAX 8
#define LIGOMETA_SEARCH_MAX 25
#define LIGOMETA_CHANNEL_MAX 65
#define LIGOMETA_FRAMESETG_MAX 49
#define LIGOMETA_SEGMENTG_MAX 49
#define LIGOMETA_SUMMVALUE_NAME_MAX 129
#define LIGOMETA_SUMMVALUE_COMM_MAX 81
#define LIGOMETA_UNIQUE_MAX 65
#define LIGOMETA_DBUNIQUE_MAX 13
#define LIGOMETA_TRANSDATA_EVENT_TABLE_MAX 19
#define LIGOMETA_TRANSDATA_NAME_MAX 33
#define LIGOMETA_TRANSDATA_UNITS_MAX 17
#define LIGOMETA_TRANSDATA_DATA_MAX 17
#define LIGOMETA_SOURCE_MAX 30
#define LIGOMETA_WAVEFORM_MAX 50
#define LIGOMETA_COORDINATES_MAX 16
#define LIGOMETA_SIMINSTPARAMS_NAME_MAX 25
#define LIGOMETA_SIMINSTPARAMS_COMM_MAX 65
#define LIGOMETA_FILTER_NAME_MAX 65
#define LIGOMETA_STD 100
#define LIGOMETA_INSPIRALTAPER_MAX 35

/** The \c MetadataTableType contains an enum type for each of the possible
    database tables that are provided.
*/
typedef enum
{
  no_table,
  process_table,
  process_params_table,
  search_summary_table,
  search_summvars_table,
  sngl_inspiral_table,
  sngl_inspiral_table_bns,
  sngl_inspiral_table_bcv,
  sngl_ringdown_table,
  sngl_transdata_table,
  multi_inspiral_table,
  sim_inspiral_table,
  sim_ringdown_table,
  summ_value_table,
  sim_inst_params_table,
  coinc_inspiral_table,
  coinc_ringdown_table,
  stochastic_table,
  stoch_summ_table,
  ext_triggers_table,
  filter_table
}
MetadataTableType;


/** The \c InterferometerNumber contains an enum type for describing the
    interferometer.
*/
typedef enum
{
  LAL_IFO_G1,
  LAL_IFO_H1,
  LAL_IFO_H2,
  LAL_IFO_L1,
  LAL_IFO_T1,
  LAL_IFO_V1,
  LAL_NUM_IFO,
  LAL_UNKNOWN_IFO = -1
}
InterferometerNumber;


/** The ProcessIDColumn contains the process_id for a given row in a table.
*/
typedef struct
tagProcessIDColumn
{
  UINT8  id;
  CHAR   textId[LIGOMETA_UNIQUE_MAX];
  UCHAR  dbUniqueId[LIGOMETA_DBUNIQUE_MAX];
}
ProcessIDColumn;


/** The \c EventIDColumn structure is used to link different tables that
refer to the same event. The database column \c event_id is a ID
generated by the database that is generally unique within a given event table
(e.g. \c sngl_inspiral or \c sngl_burst), but may be shared
across tables. For example, an entry in the \c sngl_burst may have
several entries in the \c sngl_transdata table with the same event ID
or a \c sngl_inspiral many have a corresponding entry in the
\c sngl_burst with the same event ID, e.g. for an inspiral-ringdown
search.

When the LAL XML output routines are called, the generate XML that the
database can interpret to set the \c event_id columns correctly
in the tables.

If the user does not wish to make use of the event ID at the level of the
search, for eaxmple if the user is generating a list of inspiral events
and does not want to link these to any other tables, the \c event_id
pointer in the \c SnglInpiralTable may be set to NULL. In this case
the XML output routines will generate the correct XML to tell the database
to generate a unique event ID for each inspiral event, but these should not
be shared across tables.

*/
typedef struct
tagEventIDColumn
{
  struct tagEventIDColumn *next;
  UINT8  id;
  CHAR   textId[LIGOMETA_UNIQUE_MAX];
  UCHAR  dbUniqueId[LIGOMETA_DBUNIQUE_MAX];
  struct tagSnglInspiralTable   *snglInspiralTable;
  struct tagSnglRingdownTable   *snglRingdownTable;
  struct tagSummValueTable      *summValueTable;
  struct tagMultiInspiralTable  *multiInspiralTable;
  struct tagSnglTransdataTable  *snglTransdataTable;
  struct tagSimInspiralTable    *simInspiralTable;
  struct tagSimRingdownTable    *simRingdownTable;
  struct tagCoincInspiralTable  *coincInspiralTable;
  struct tagCoincRingdownTable  *coincRingdownTable;
  struct tagInspiralTemplate    *inspiralTemplate;
}
EventIDColumn;


typedef struct
tagProcessTable
{
  struct tagProcessTable *next;
  CHAR          program[LIGOMETA_PROGRAM_MAX];
  CHAR          version[LIGOMETA_VERSION_MAX];
  CHAR          cvs_repository[LIGOMETA_CVS_REPOSITORY_MAX];
  LIGOTimeGPS   cvs_entry_time;
  CHAR          comment[LIGOMETA_COMMENT_MAX];
  INT4          is_online;
  CHAR          node[LIGOMETA_NODE_MAX];
  CHAR          username[LIGOMETA_USERNAME_MAX];
  LIGOTimeGPS   start_time;
  LIGOTimeGPS   end_time;
  INT4          jobid;
  CHAR          domain[LIGOMETA_DOMAIN_MAX];
  INT4          unix_procid;
  CHAR          ifos[LIGOMETA_IFOS_MAX];
  long          process_id;
}
ProcessTable;


typedef struct
tagProcessParamsTable
{
  struct tagProcessParamsTable *next;
  CHAR          program[LIGOMETA_PROGRAM_MAX];
  long          process_id;
  CHAR          param[LIGOMETA_PARAM_MAX];
  CHAR          type[LIGOMETA_TYPE_MAX];
  CHAR          value[LIGOMETA_VALUE_MAX];
}
ProcessParamsTable;



/* shared object is lalapps, lalwrapper_cvs_tag is lalapps cvs tag */
/* lal_cvs tag is populated automatically                          */
typedef struct
tagSearchSummaryTable
{
  struct tagSearchSummaryTable *next;
  long          process_id;
  CHAR          comment[LIGOMETA_COMMENT_MAX];
  LIGOTimeGPS   in_start_time;
  LIGOTimeGPS   in_end_time;
  LIGOTimeGPS   out_start_time;
  LIGOTimeGPS   out_end_time;
  INT4          nevents;
  INT4          nnodes;
  CHAR          ifos[LIGOMETA_IFOS_MAX];
}
SearchSummaryTable;

typedef struct
tagSearchSummvarsTable
{
  struct tagSearchSummvarsTable *next;
  CHAR          name[LIGOMETA_NAME_MAX];
  CHAR          string[LIGOMETA_STRING_MAX];
  REAL8         value;
}
SearchSummvarsTable;


typedef struct
tagSnglBurst
{
	struct tagSnglBurst *next;
	CHAR ifo[LIGOMETA_IFO_MAX];
	CHAR search[LIGOMETA_SEARCH_MAX];
	CHAR channel[LIGOMETA_CHANNEL_MAX];
	LIGOTimeGPS start_time;
	LIGOTimeGPS peak_time;
	REAL4 duration;
	REAL4 central_freq;
	REAL4 bandwidth;
	REAL4 amplitude;
	REAL4 snr;
	REAL4 confidence;
	REAL8 chisq;
	REAL8 chisq_dof;
	long process_id;
	long event_id;
}
SnglBurst;

typedef struct
tagSnglInspiralTable
{
  struct tagSnglInspiralTable *next;
  CHAR          ifo[LIGOMETA_IFO_MAX];
  CHAR          search[LIGOMETA_SEARCH_MAX];
  CHAR          channel[LIGOMETA_CHANNEL_MAX];
  LIGOTimeGPS   end_time;
  REAL8         end_time_gmst;
  LIGOTimeGPS   impulse_time;
  REAL8         template_duration;
  REAL8         event_duration;
  REAL4         amplitude;
  REAL4         eff_distance;
  REAL4         coa_phase;
  REAL4         mass1;
  REAL4         mass2;
  REAL4         mchirp;
  REAL4         mtotal;
  REAL4         eta;
  REAL4         kappa;
  REAL4         chi;
  REAL4         tau0;
  REAL4         tau2;
  REAL4         tau3;
  REAL4         tau4;
  REAL4         tau5;
  REAL4         ttotal;
  REAL4         psi0;
  REAL4         psi3;
  REAL4         alpha;
  REAL4         alpha1;
  REAL4         alpha2;
  REAL4         alpha3;
  REAL4         alpha4;
  REAL4         alpha5;
  REAL4         alpha6;
  REAL4         beta;
  REAL4         f_final;
  REAL4         snr;
  REAL4         chisq;
  INT4          chisq_dof;
  REAL4         bank_chisq;
  INT4          bank_chisq_dof;
  REAL4         cont_chisq;
  INT4          cont_chisq_dof;
  REAL8         sigmasq;
  REAL4         rsqveto_duration;
  REAL4         Gamma[10];    /* metric co-efficients */
  REAL4         spin1x;
  REAL4         spin1y;
  REAL4         spin1z;
  REAL4         spin2x;
  REAL4         spin2y;
  REAL4         spin2z;
  EventIDColumn *event_id;
}
SnglInspiralTable;


/** This structure contains the required information for generating a ringdown
template and storing ringdown triggers.  The fields are:
<dl>
<dt>ifo</dt><dd> The interferometer in which the trigger is found.</dd>
<dt>channel</dt><dd> The interferometer channel in which the ringdown is
found.</dd>
<dt>start_time</dt><dd> The GPS start time of the ringdown trigger.</dd>
<dt>frequency</dt><dd> The central frequency of the ringdown waveform
    (in Hz).</dd>
<dt>quality</dt><dd> The quality factor \f$Q\f$ of the ringdown waveform.</dd>
<dt>phase</dt><dd> The initial phase of the ringdown in radians.
    Zero is a cosine-phase ringdown; \f$-\pi/2\f$ is a sine-phase ringdown.</dd>
<dt>mass</dt><dd> The mass \f$M\f$ of the black hole (in solar
    masses, \f$M_\odot\f$).</dd>
<dt>spin</dt><dd> The dimensionless spin parameter of the
    black hole \f${\hat{a}}\f$ where the spin is \f$S={\hat{a}}GM^2/c\f$ (\f$G\f$ is
    Newton constant and \f$c\f$ is the speed of light).</dd>
<dt>epsilon</dt><dd> The fractional mass loss \f$\epsilon\f$
    of the initial black hole mass in ringdown radiation.</dd>
<dt>amplitude</dt><dd> The peak strain amplitude \f$h_0\f$ of the waveform,
    given by
    \f{equation}{
    h_0=\sqrt{\frac{5}{2}\epsilon}\left(\frac{GM}{c^2 r}\right)
    Q^{-\frac{1}{2}} F(Q)^{-\frac{1}{2}} g(a)^{-\frac{1}{2}},
    \f}
    where \f$F(Q) = 1 + 7/(24 Q^2)\f$, \f$g(a) = 1 - 0.63(1-a)^{3/10}\f$ and \f$r\f$
    is the distance to the trigger.</dd>
<dt>snr</dt><dd> The signal-to-noise ratio of the trigger.</dd>
<dt>eff_dist</dt><dd> The effective distance to the trigger source in
    megaparsecs (Mpc).</dd>
<dt>sigma_sq</dt><dd> The variance of the matched filter used to find the
    trigger.</dd>
</dl>

*/
typedef struct
tagSnglRingdownTable
{
  struct tagSnglRingdownTable *next;
  CHAR          ifo[LIGOMETA_IFO_MAX];
  CHAR          channel[LIGOMETA_CHANNEL_MAX];
  LIGOTimeGPS   start_time;
  REAL8         start_time_gmst;
  REAL4         frequency;
  REAL4         quality;
  REAL4         phase;
  REAL4         mass;
  REAL4         spin;
  REAL4         epsilon;
  INT4          num_clust_trigs;
  REAL4         ds2_H1H2;
  REAL4         ds2_H1L1;
  REAL4         ds2_H1V1;
  REAL4         ds2_H2L1;
  REAL4         ds2_H2V1;
  REAL4         ds2_L1V1;
  REAL4         amplitude;
  REAL4         snr;
  REAL4         eff_dist;
  REAL8         sigma_sq;
  EventIDColumn *event_id;
}
SnglRingdownTable;



typedef struct
tagMultiInspiralTable
{
  struct tagMultiInspiralTable *next;
  CHAR          ifos[LIGOMETA_IFOS_MAX];
  CHAR          search[LIGOMETA_SEARCH_MAX];
  LIGOTimeGPS   end_time;
  REAL8         end_time_gmst;
  LIGOTimeGPS   impulse_time;
  REAL4         amplitude;
  REAL4         distance;
  REAL4         eff_dist_h1;
  REAL4         eff_dist_h2;
  REAL4         eff_dist_l;
  REAL4         eff_dist_g;
  REAL4         eff_dist_t;
  REAL4         eff_dist_v;
  REAL4         eff_dist_h1h2;
  REAL4         coa_phase;
  REAL4         mass1;
  REAL4         mass2;
  REAL4         mchirp;
  REAL4         eta;
  REAL4         chi;
  REAL4         kappa;
  REAL4         tau0;
  REAL4         tau2;
  REAL4         tau3;
  REAL4         tau4;
  REAL4         tau5;
  REAL4         ttotal;
  REAL4         snr;
  INT4          snr_dof;
  REAL4         chisq;
  INT4          chisq_dof;
  REAL4         bank_chisq;
  INT4          bank_chisq_dof;
  REAL4         cont_chisq;
  INT4          cont_chisq_dof;
  REAL4         trace_snr;
  REAL4         snr_h1;
  REAL4         snr_h2;
  REAL4         snr_l;
  REAL4         snr_g;
  REAL4         snr_t;
  REAL4         snr_v;
  REAL4         amp_term_1;
  REAL4         amp_term_2;
  REAL4         amp_term_3;
  REAL4         amp_term_4;
  REAL4         amp_term_5;
  REAL4         amp_term_6;
  REAL4         amp_term_7;
  REAL4         amp_term_8;
  REAL4         amp_term_9;
  REAL4         amp_term_10;
  REAL8         sigmasq_h1;
  REAL8         sigmasq_h2;
  REAL8         sigmasq_l;
  REAL8         sigmasq_g;
  REAL8         sigmasq_t;
  REAL8         sigmasq_v;
  REAL4         chisq_h1;
  REAL4         chisq_h2;
  REAL4         chisq_l;
  REAL4         chisq_g;
  REAL4         chisq_t;
  REAL4         chisq_v;
  INT4          sngl_chisq_dof;
  REAL4         bank_chisq_h1;
  REAL4         bank_chisq_h2;
  REAL4         bank_chisq_l;
  REAL4         bank_chisq_g;
  REAL4         bank_chisq_t;
  REAL4         bank_chisq_v;
  INT4          sngl_bank_chisq_dof;
  REAL4         cont_chisq_h1;
  REAL4         cont_chisq_h2;
  REAL4         cont_chisq_l;
  REAL4         cont_chisq_g;
  REAL4         cont_chisq_t;
  REAL4         cont_chisq_v;
  INT4          sngl_cont_chisq_dof;
  REAL4         ra;
  REAL4         dec;
  REAL4         ligo_angle;
  REAL4         ligo_angle_sig;
  REAL4         inclination;
  REAL4         polarization;
  REAL4         null_statistic;
  REAL4         null_stat_h1h2;
  REAL4         null_stat_degen;
  EventIDColumn *event_id;
  COMPLEX8      h1quad;
  COMPLEX8      h2quad;
  COMPLEX8      l1quad;
  COMPLEX8      g1quad;
  COMPLEX8      t1quad;
  COMPLEX8      v1quad;
  REAL4         coh_snr_h1h2;
  REAL4         cohSnrSqLocal;
  REAL4         autoCorrCohSq;
  REAL4         crossCorrCohSq;
  REAL4         autoCorrNullSq;
  REAL4         crossCorrNullSq;
  REAL8         ampMetricEigenVal1;
  REAL8         ampMetricEigenVal2;
  EventIDColumn *time_slide_id;
}
MultiInspiralTable;


typedef struct
tagSimInspiralTable
{
  struct tagSimInspiralTable *next;
  CHAR           waveform[LIGOMETA_WAVEFORM_MAX];
  LIGOTimeGPS    geocent_end_time;
  LIGOTimeGPS    h_end_time;
  LIGOTimeGPS    l_end_time;
  LIGOTimeGPS    g_end_time;
  LIGOTimeGPS    t_end_time;
  LIGOTimeGPS    v_end_time;
  REAL8          end_time_gmst;
  CHAR           source[LIGOMETA_SOURCE_MAX];
  REAL4          mass1;
  REAL4          mass2;
  REAL4          eta;
  REAL4          distance;
  REAL4          longitude;
  REAL4          latitude;
  REAL4          inclination;
  REAL4          coa_phase;
  REAL4          polarization;
  REAL4          psi0;
  REAL4          psi3;
  REAL4          alpha;
  REAL4          alpha1;
  REAL4          alpha2;
  REAL4          alpha3;
  REAL4          alpha4;
  REAL4          alpha5;
  REAL4          alpha6;
  REAL4          beta;
  REAL4          spin1x;
  REAL4          spin1y;
  REAL4          spin1z;
  REAL4          spin2x;
  REAL4          spin2y;
  REAL4          spin2z;
  REAL4          theta0;
  REAL4          phi0;
  REAL4          f_lower;
  REAL4          f_final;
  REAL4          mchirp;
  REAL4          eff_dist_h;
  REAL4          eff_dist_l;
  REAL4          eff_dist_g;
  REAL4          eff_dist_t;
  REAL4          eff_dist_v;
  REAL4	         qmParameter1;
  REAL4		 qmParameter2;
  EventIDColumn  *event_id;
  INT4           numrel_mode_min;
  INT4           numrel_mode_max;
  CHAR           numrel_data[LIGOMETA_STRING_MAX];
  INT4           amp_order;
  CHAR           taper[LIGOMETA_INSPIRALTAPER_MAX];
  INT4           bandpass;
}
SimInspiralTable;

/** The \c CoincInspiralTable contains a set of pointers to SnglInspiral
tables.  The length of the array is set to \c LAL_NUM_IFO which is a
value of the enum \c InterferometerNumber described above.  This enum
also provides an easy way to access the \c SnglInspiral corresponding to
each ifo.  In addition the table, it contains a field in which to store the
number and names of the IFOs which have non-null SnglInspiralTables.  Finally,
it contains a pointer to an associated \c SimInspiral.  This table is
meant to provide a simple way to manipulate coincident triggers.
*/
typedef struct
tagCoincInspiralTable
{
  struct tagCoincInspiralTable *next;
  CHAR                ifos[LIGOMETA_IFOS_MAX];
  INT4                numIfos;
  SnglInspiralTable  *snglInspiral[LAL_NUM_IFO];
  SimInspiralTable   *simInspiral;
  REAL4              stat;
}
CoincInspiralTable;

/** The \c CoincInspiralSlideTable contains a pointer to a CoincInspiral
table meant for a particular time slide.  In addition to the table, it
contains a field in which to store the time slide number, the analyzed time for
this time slide, and a pointer to the next \c CoincInspiralSlideTable for
a different time slide.
*/
typedef struct
tagCoincInspiralSlideTable
{
  struct tagCoincInspiralSlideTable *next;
  REAL4               slideTimeAnalyzed;
  INT4                slideNum;
  REAL4               currentRate;
  CoincInspiralTable  *coincInspiral;
}
CoincInspiralSlideTable;


/** The \c SimBurst structure describes a burst injection.
*/
typedef struct
tagSimBurst
{
	struct tagSimBurst *next;
	long process_id;
	char waveform[LIGOMETA_WAVEFORM_MAX];
	REAL8 ra;
	REAL8 dec;
	REAL8 psi;
	LIGOTimeGPS time_geocent_gps;
	REAL8 time_geocent_gmst;
	REAL8 duration;
	REAL8 frequency;
	REAL8 bandwidth;
	REAL8 q;
	REAL8 pol_ellipse_angle;
	REAL8 pol_ellipse_e;
	REAL8 amplitude;
	REAL8 hrss;
	REAL8 egw_over_rsquared;
	unsigned long waveform_number;
	long time_slide_id;
	long simulation_id;
} SimBurst;


/** This structure contains the required information for generating a ringdown
injection.  The fields are:
<dl>
<dt>waveform</dt><dd> Description of the type of waveform to inject.</dd>
<dt>coordinates</dt><dd> Type of coordinate system to perform injection.</dd>
<dt>geocent_start_time</dt><dd> The GPS start time of the waveform at the
    center of the earth.</dd>
<dt>h_start_time</dt><dd> The GPS start time of the waveform at the
    LIGO Hanford Observatory.</dd>
<dt>l_start_time</dt><dd> The GPS start time of the waveform at the
    LIGO Livingston Observatory.</dd>
<dt>v_start_time</dt><dd> The GPS start time of the waveform at the
    Virgo Observatory.</dd>
<dt>start_time_gmst</dt><dd> The start time of the waveform at the
    in siderial time.</dd>
<dt>longitude</dt><dd> The longitude of the source defined in the
    stated coordinate system.</dd>
<dt>latitude</dt><dd> The latitude of the source defined in the
    stated coordinate system.</dd>
<dt>distance</dt><dd> The distance to the source defined in the
    stated coordinate system in megaparsecs (Mpc).</dd>
<dt>inclination</dt><dd> The inclination angle \f$\iota\f$ of the source.</dd>
<dt>polarization</dt><dd> The polarization angle \f$\Psi\f$ of the source.</dd>
<dt>frequency</dt><dd> The central frequency of the ringdown waveform
    (in Hz).</dd>
<dt>quality</dt><dd> The quality factor \f$Q\f$ of the ringdown waveform.</dd>
<dt>phase</dt><dd> The initial phase of the ringdown signal \f$\phi_0\f$.</dd>
<dt>mass</dt><dd> The mass \f$M\f$ of the black hole (in solar
    masses, \f$M_\odot\f$).</dd>
<dt>spin</dt><dd> The dimensionless spin parameter of the
    black hole \f${\hat{a}}\f$ where the spin is \f$S={\hat{a}}GM^2/c\f$ (\f$G\f$ is
    Newton constant and \f$c\f$ is the speed of light).</dd>
<dt>epsilon</dt><dd> The fractional mass loss \f$\epsilon\f$
    of the initial black hole mass in ringdown radiation.</dd>
<dt>amplitude</dt><dd> The peak strain amplitude \f$h_0\f$ of the waveform,
    given by
    \f{equation}{
    h_0=\sqrt{\frac{5}{2}\epsilon}\left(\frac{GM}{c^2 r}\right)
    Q^{-\frac{1}{2}} F(Q)^{-\frac{1}{2}} g(a)^{-\frac{1}{2}},
    \f}
    where \f$F(Q) = 1 + 7/(24 Q^2)\f$, \f$g(a) = 1 - 0.63(1-a)^{3/10}\f$ and \f$r\f$
    is the distance to the trigger.</dd>
<dt>h_eff_dist</dt><dd> The effective distance to the source from the
    LIGO Hanford Observatory in megaparsecs (Mpc).</dd>
<dt>l_eff_dist</dt><dd> The effective distance to the source from the
    LIGO Livingston Observatory in megaparsecs (Mpc).</dd>
<dt>v_eff_dist</dt><dd> The effective distance to the source from the
    Virgo Observatory in megaparsecs (Mpc).</dd>
<dt>hrss</dt><dd> Define.</dd>
<dt>hrss_h</dt><dd> Define.</dd>
<dt>hrss_l</dt><dd> Define.</dd>
<dt>hrss_v</dt><dd> Define.</dd>
</dl>
*/
typedef struct
tagSimRingdownTable
{
  struct tagSimRingdownTable *next;
  CHAR           waveform[LIGOMETA_WAVEFORM_MAX];
  CHAR           coordinates[LIGOMETA_COORDINATES_MAX];
  LIGOTimeGPS    geocent_start_time;
  LIGOTimeGPS    h_start_time;
  LIGOTimeGPS    l_start_time;
  LIGOTimeGPS    v_start_time;
  REAL8          start_time_gmst;
  REAL4          longitude;
  REAL4          latitude;
  REAL4          distance;
  REAL4          inclination;
  REAL4          polarization;
  REAL4          frequency;
  REAL4          quality;
  REAL4          phase;
  REAL4          mass;
  REAL4          spin;
  REAL4          epsilon;
  REAL4          amplitude;
  REAL4          eff_dist_h;
  REAL4          eff_dist_l;
  REAL4          eff_dist_v;
  REAL4          hrss;
  REAL4          hrss_h;
  REAL4          hrss_l;
  REAL4          hrss_v;
  EventIDColumn  *event_id;
}
SimRingdownTable;


typedef struct
tagCoincRingdownTable
{
  struct tagCoincRingdownTable *next;
  CHAR                ifos[LIGOMETA_IFOS_MAX];
  INT4                numIfos;
  SnglRingdownTable  *snglRingdown[LAL_NUM_IFO];
  SimRingdownTable   *simRingdown;
  REAL4              ds2_H1H2;
  REAL4              ds2_H1L1;
  REAL4              ds2_H1V1;
  REAL4              ds2_H2L1;
  REAL4              ds2_H2V1;
  REAL4              ds2_L1V1;
}
CoincRingdownTable;

typedef struct
tagSnglTransdataTable
{
  CHAR          event_table[LIGOMETA_TRANSDATA_EVENT_TABLE_MAX];
  CHAR          ifo[LIGOMETA_IFO_MAX];
  CHAR          name[LIGOMETA_TRANSDATA_NAME_MAX];
  INT4          dimensions;
  INT4          x_bins;
  REAL8         x_start;
  REAL8         x_end;
  CHAR          x_units[LIGOMETA_TRANSDATA_UNITS_MAX];
  INT4          y_bins;
  REAL8         y_start;
  REAL8         y_end;
  CHAR          y_units[LIGOMETA_TRANSDATA_UNITS_MAX];
  CHAR          data_type[LIGOMETA_TRANSDATA_DATA_MAX];
  CHAR          data_units[LIGOMETA_TRANSDATA_DATA_MAX];
  UCHAR         *trans_data; /* must be big Endian */
  INT4          transdata_length;
  EventIDColumn *event_id;
}
SnglTransdataTable;

typedef struct
tagSummValueTable
{
  struct tagSummValueTable *next;
  CHAR          program[LIGOMETA_PROGRAM_MAX];
  CHAR          frameset_group[LIGOMETA_FRAMESETG_MAX];
  CHAR          segment_group[LIGOMETA_SEGMENTG_MAX];
  INT4          version;
  LIGOTimeGPS   start_time;
  LIGOTimeGPS   end_time;
  CHAR          ifo[LIGOMETA_IFO_MAX];
  CHAR          name[LIGOMETA_SUMMVALUE_NAME_MAX];
  REAL4         value;
  REAL4         error;
  INT4          intvalue;
  CHAR          comment[LIGOMETA_SUMMVALUE_COMM_MAX];
}
SummValueTable;


typedef struct
tagSimInstParamsTable
{
  struct tagSimInstParamsTable *next;
  CHAR          name[LIGOMETA_SIMINSTPARAMS_NAME_MAX];
  CHAR          comment[LIGOMETA_SIMINSTPARAMS_COMM_MAX];
  REAL8         value;
}
SimInstParamsTable;

/** The \c StochasticTable contains output parameters relevant for the
stochastic search. The IFOs, channels, start time, duration, minimum and
maximum frequency, cross correlation statistic and theoretical variance
are stored.
*/
typedef struct
tagStochasticTable
{
  struct tagStochasticTable *next;
  CHAR          ifo_one[LIGOMETA_IFO_MAX];
  CHAR          ifo_two[LIGOMETA_IFO_MAX];
  CHAR          channel_one[LIGOMETA_CHANNEL_MAX];
  CHAR          channel_two[LIGOMETA_CHANNEL_MAX];
  LIGOTimeGPS   start_time;
  LIGOTimeGPS   duration;
  REAL8         f_min;
  REAL8         f_max;
  REAL8         cc_stat;
  REAL8         cc_sigma;
}
StochasticTable;

/** The \c StochSummTable contains a summary of a stochastic search. It
can be used to fully summerise the parameters used for a full search.
*/
typedef struct
tagStochSummTable
{
  struct tagStochSummTable *next;
  CHAR          ifo_one[LIGOMETA_IFO_MAX];
  CHAR          ifo_two[LIGOMETA_IFO_MAX];
  CHAR          channel_one[LIGOMETA_CHANNEL_MAX];
  CHAR          channel_two[LIGOMETA_CHANNEL_MAX];
  LIGOTimeGPS   start_time;
  LIGOTimeGPS   end_time;
  REAL8         f_min;
  REAL8         f_max;
  REAL8         y_opt;
  REAL8         error;
}
StochSummTable;


typedef struct
tagExtTriggerTable
{
  struct        tagExtTriggerTable *next;
  CHAR          det_alts[LIGOMETA_STD];
  CHAR          det_band[LIGOMETA_STD];
  CHAR          det_fluence[LIGOMETA_STD];
  CHAR          det_fluence_int[LIGOMETA_STD];
  CHAR          det_name[LIGOMETA_STD];
  CHAR          det_peak[LIGOMETA_STD];
  CHAR          det_peak_int[LIGOMETA_STD];
  CHAR          det_snr[LIGOMETA_STD];
  INT4          email_time;
  REAL4         event_dec;
  REAL4         event_dec_err;
  CHAR          event_epoch[LIGOMETA_STD];
  CHAR          event_err_type[LIGOMETA_STD];
  REAL4         event_ra;
  REAL4         event_ra_err;
  INT4          start_time;
  INT4          start_time_ns;
  CHAR          event_type[LIGOMETA_STD];
  REAL4         event_z;
  REAL4         event_z_err;
  CHAR          notice_comments[LIGOMETA_STD];
  CHAR          notice_id[LIGOMETA_STD];
  CHAR          notice_sequence[LIGOMETA_STD];
  INT4          notice_time;
  CHAR          notice_type[LIGOMETA_STD];
  CHAR          notice_url[LIGOMETA_STD];
  REAL4         obs_fov_dec;
  REAL4         obs_fov_dec_width;
  REAL4         obs_fov_ra;
  REAL4         obs_fov_ra_width;
  REAL4         obs_loc_ele;
  REAL4         obs_loc_lat;
  REAL4         obs_loc_long;
  REAL4         ligo_fave_lho;
  REAL4         ligo_fave_llo;
  REAL4         ligo_delay;
  INT4          event_number_gcn;
  CHAR          event_number_grb[8];
  INT4          event_status;
  EventIDColumn *event_id;
}
ExtTriggerTable;

typedef struct
tagFilterTable
{
  struct tagFilterTable *next;
  CHAR          program[LIGOMETA_PROGRAM_MAX];
  INT4          start_time;
  CHAR          filter_name[LIGOMETA_FILTER_NAME_MAX];
  CHAR          comment[LIGOMETA_SUMMVALUE_COMM_MAX];
}
FilterTable;


/** This structure corresponds to one row of a time_slide table.
*/
typedef struct
tagTimeSlide
{
  struct tagTimeSlide *next;
  long           process_id;
  long           time_slide_id;
  CHAR           instrument[LIGOMETA_STRING_MAX];
  REAL8          offset;
}
TimeSlide;

typedef union
tagMetadataTable
{
  ProcessTable          *processTable;
  ProcessParamsTable    *processParamsTable;
  SearchSummaryTable    *searchSummaryTable;
  SearchSummvarsTable   *searchSummvarsTable;
  SnglBurst             *snglBurst;
  SnglInspiralTable     *snglInspiralTable;
  SnglRingdownTable     *snglRingdownTable;
  MultiInspiralTable    *multiInspiralTable;
  SimInspiralTable      *simInspiralTable;
  SimRingdownTable      *simRingdownTable;
  SnglTransdataTable    *snglTransdataTable;
  SummValueTable        *summValueTable;
  SimInstParamsTable    *simInstParamsTable;
  StochasticTable       *stochasticTable;
  StochSummTable        *stochSummTable;
  ExtTriggerTable       *extTriggerTable;
  FilterTable           *filterTable;
}
MetadataTable;

typedef struct
tagSegmentTable
{
  struct tagSegmentTable *next;
  INT4 creator_db;
  long process_id;
  long segment_id;
  LIGOTimeGPS start_time;
  LIGOTimeGPS end_time;
  long segment_def_id;
  INT4 segment_def_cdb; 
}
SegmentTable;

typedef struct
tagTimeSlideSegmentMapTable
{
  struct tagTimeSlideSegmentMapTable *next;
  long segment_def_id;
  long time_slide_id;
}
TimeSlideSegmentMapTable;

#if 0
{ /* so that editors will match succeeding brace */
#elif defined(__cplusplus)
}
#endif

#endif /* _LIGOMETADATATABLES_H */
