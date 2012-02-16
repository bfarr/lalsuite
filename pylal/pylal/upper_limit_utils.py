import numpy
from scipy import random
from scipy import interpolate
import bisect
import sys
from glue.ligolw import lsctables
from pylal.xlal import constants

import matplotlib
matplotlib.use("agg")
from matplotlib import pyplot

from pylal import rate


def margLikelihoodMonteCarlo(VTs, lambs, mu, mcerrs=None):
    '''
    This function marginalizes the loudest event likelihood over unknown
    Monte Carlo errors, assumed to be independent between each experiment.
    '''
    if mcerrs is None:
        mcerrs = [0]*len(VTs)

    # combine experiments, propagating statistical uncertainties
    # in the measured efficiency
    likely = 1
    for vA,dvA,mc in zip(VTs,lambs,mcerrs):
        if mc == 0:
            # we have perfectly measured our efficiency in this mass bin
            # so the posterior is given by eqn (11) in BCB
            likely *= (1+mu*vA*dvA)*numpy.exp(-mu*vA)
        else:
            # we have uncertainty in our efficiency in this mass bin and
            # want to marginalize it out using eqn (24) of BCB
            k = (vA/mc)**2 # k is 1./fractional_error**2
            likely *= (1+mu*vA*(1/k+dvA))*(1+mu*vA/k)**(-(k+1))

    return likely


def margLikelihood(VTs, lambs, mu, calerr=0, mcerrs=None):
    '''
    This function marginalizes the loudest event likelihood over unknown
    Monte Carlo and calibration errors. The vector VTs is the sensitive
    volumes for independent searches and lambs is the vector of loudest
    event likelihood. The statistical errors are assumed to be independent
    between each experiment while the calibration errors are applied
    the same in each experiment.
    '''
    if calerr == 0:
        return margLikelihoodMonteCarlo(VTs,lambs,mu,mcerrs)

    std = calerr
    mean = 0 # median volume = 1

    fracerrs = numpy.linspace(0.33,3,5e2) # assume we got the volume to a factor of three or better
    errdist = numpy.exp(-(numpy.log(fracerrs)-mean)**2/(2*std**2))/(fracerrs*std) # log-normal pdf
    errdist /= errdist.sum() #normalize

    likely = sum([ pd*margLikelihoodMonteCarlo(delta*VTs,lambs,mu,mcerrs) for delta, pd in zip(fracerrs,errdist)]) #marginalize over errors

    return likely


def compute_upper_limit(mu_in, post, alpha = 0.9):
    """
    Returns the upper limit mu_high of confidence level alpha for a
    posterior distribution post on the given parameter mu.
    """
    # interpolate to a linearly spaced array for accuracy in mu
    post_rep = interpolate.splrep(mu_in, post)
    mu = numpy.linspace(mu_in.min(),mu_in.max(),len(mu_in))
    post = interpolate.splev(mu, post_rep)

    if 0 < alpha < 1:
        high_idx = bisect.bisect_left( post.cumsum()/post.sum(), alpha )
        # if alpha is in (0,1] and post is non-negative, bisect_left
        # will always return an index in the range of mu since
        # post.cumsum()/post.sum() will always begin at 0 and end at 1
        mu_high = mu[high_idx]
    elif alpha == 1:
        mu_high = numpy.max(mu[post>0])
    else:
        raise ValueError, "Confidence level must be in (0,1]."

    return mu_high


def compute_lower_limit(mu, post, alpha = 0.9):
    """
    Returns the lower limit mu_low of confidence level alpha for a
    posterior distribution post on the given parameter mu.
    """
    if 0 < alpha < 1:
        low_idx = bisect.bisect_right( post.cumsum()/post.sum(), 1-alpha )
        # if alpha is in [0,1) and post is non-negative, bisect_right
        # will always return an index in the range of mu since
        # post.cumsum()/post.sum() will always begin at 0 and end at 1
        mu_low = mu[low_idx]
    elif alpha == 1:
        mu_low = numpy.min(mu[post>0])
    else:
        raise ValueError, "Confidence level must be in (0,1]."

    return mu_low


def confidence_interval( mu, post, alpha = 0.9 ):
    '''
    Returns the minimal-width confidence interval [mu_low,mu_high] of
    confidence level alpha for a distribution post on the parameter mu.
    '''
    if not 0 < alpha < 1:
        raise ValueError, "Confidence level must be in (0,1)."

    # choose a step size for the sliding confidence window
    alpha_step = 0.01

    # initialize the lower and upper limits
    mu_low = numpy.min(mu)
    mu_high = numpy.max(mu)

    # find the smallest window (by delta-mu) stepping by dalpha
    for ai in numpy.arange( 0, 1-alpha, alpha_step ):
        ml = compute_lower_limit( mu, post, 1 - ai )
        mh = compute_upper_limit( mu, post, alpha + ai)
        if mh - ml < mu_high - mu_low:
            mu_low = ml
            mu_high = mh

    return mu_low, mu_high


def integrate_efficiency(dbins, eff, err=0, logbins=False):

    if logbins:
        logd = numpy.log(dbins)
        dlogd = logd[1:]-logd[:-1]
        dreps = numpy.exp( (numpy.log(dbins[1:])+numpy.log(dbins[:-1]))/2) # log midpoint
        vol = numpy.sum( 4*numpy.pi *dreps**3 *eff *dlogd )
        verr = numpy.sqrt(numpy.sum( (4*numpy.pi *dreps**3 *err *dlogd)**2 )) #propagate errors in eff to errors in v
    else:
        dd = dbins[1:]-dbins[:-1]
        dreps = (dbins[1:]+dbins[:-1])/2 #midpoint
        vol = numpy.sum( 4*numpy.pi *dreps**2 *eff *dd )
        verr = numpy.sqrt(numpy.sum( (4*numpy.pi *dreps**2 *err *dd)**2 )) #propagate errors in eff to errors in v

    return vol, verr


def compute_efficiency(f_dist,m_dist,dbins):
    '''
    Compute the efficiency as a function of distance for the given sets of found
    and missed injection distances.
    Note that injections that do not fit into any dbin get lost :(.
    '''
    efficiency = numpy.zeros( len(dbins)-1 )
    error = numpy.zeros( len(dbins)-1 )
    for j, dlow in enumerate(dbins[:-1]):
        dhigh = dbins[j+1]
        found = numpy.sum( (dlow <= f_dist)*(f_dist < dhigh) )
        missed = numpy.sum( (dlow <= m_dist)*(m_dist < dhigh) )
        if found+missed == 0: missed = 1.0 #avoid divide by 0 in empty bins
        efficiency[j] = 1.0*found /(found + missed)
        error[j] = numpy.sqrt(efficiency[j]*(1-efficiency[j])/(found+missed))

    return efficiency, error


def mean_efficiency_volume(found, missed, dbins):

    if len(found) == 0: # no efficiency here
        return numpy.zeros(len(dbins)-1),numpy.zeros(len(dbins)-1), 0, 0

    # only need distances
    f_dist = numpy.array([l.distance for l in found])
    m_dist = numpy.array([l.distance for l in missed])

    # compute the efficiency and its variance
    eff, err = compute_efficiency(f_dist,m_dist,dbins)
    vol, verr = integrate_efficiency(dbins, eff, err)

    return eff, err, vol, verr


def filter_injections_by_mass(injs, mbins, bin_num , bin_type, bin_num2=None):
    '''
    For a given set of injections (sim_inspiral rows), return the subset
    of injections that fall within the given mass range.
    '''

    if bin_type == "Mass1_Mass2":
        m1bins = numpy.concatenate((mbins.lower()[0],numpy.array([mbins.upper()[0][-1]])))
        m1lo = m1bins[bin_num]
        m1hi = m1bins[bin_num+1]
        m2bins = numpy.concatenate((mbins.lower()[1],numpy.array([mbins.upper()[1][-1]])))
        m2lo = m2bins[bin_num2]
        m2hi = m2bins[bin_num2+1]
        newinjs = [l for l in injs if ( (m1lo<= l.mass1 <m1hi and m2lo<= l.mass2 <m2hi) or (m1lo<= l.mass2 <m1hi and m2lo<= l.mass1 <m2hi))]
        return newinjs

    mbins = numpy.concatenate((mbins.lower()[0],numpy.array([mbins.upper()[0][-1]])))
    mlow = mbins[bin_num]
    mhigh = mbins[bin_num+1]
    if bin_type == "Chirp_Mass":
        newinjs = [l for l in injs if (mlow <= l.mchirp < mhigh)]
    elif bin_type == "Total_Mass":
        newinjs = [l for l in injs if (mlow <= l.mass1+l.mass2 < mhigh)]
    elif bin_type == "Component_Mass": #it is assumed that m2 is fixed
        newinjs = [l for l in injs if (mlow <= l.mass1 < mhigh)]
    elif bin_type == "BNS_BBH":
        if bin_num == 0 or bin_num == 2: #BNS/BBH case
            newinjs = [l for l in injs if (mlow <= l.mass1 < mhigh and mlow <= l.mass2 < mhigh)]
        else:
            newinjs = [l for l in injs if (mbins[0] <= l.mass1 < mbins[1] and mbins[2] <= l.mass2 < mbins[3])] #NSBH
            newinjs += [l for l in injs if (mbins[0] <= l.mass2 < mbins[1] and mbins[2] <= l.mass1 < mbins[3])] #BHNS

    return newinjs


def compute_volume_vs_mass(found, missed, mass_bins, bin_type, catalog=None, dbins=None, ploteff=False,logd=False):
    """
    Compute the average luminosity an experiment was sensitive to given the sets
    of found and missed injections and assuming luminosity is unformly distributed
    in space.
    """
    # mean and std estimate for luminosity (in L10s)
    volArray = rate.BinnedArray(mass_bins)
    vol2Array = rate.BinnedArray(mass_bins)

    # found/missed stats
    foundArray = rate.BinnedArray(mass_bins)
    missedArray = rate.BinnedArray(mass_bins)

    #
    # compute the mean luminosity in each mass bin
    #
    effvmass = []
    errvmass = []

    if bin_type == "Mass1_Mass2": # two-d case first
        for j,mc1 in enumerate(mass_bins.centres()[0]):
            for k,mc2 in enumerate(mass_bins.centres()[1]):
                newfound = filter_injections_by_mass( found, mass_bins, j, bin_type, k)
                newmissed = filter_injections_by_mass( missed, mass_bins, j, bin_type, k)

                foundArray[(mc1,mc2)] = len(newfound)
                missedArray[(mc1,mc2)] = len(newmissed)

                # compute the volume using this injection set
                meaneff, efferr, meanvol, volerr = mean_efficiency_volume(newfound, newmissed, dbins)
                effvmass.append(meaneff)
                errvmass.append(efferr)
                volArray[(mc1,mc2)] = meanvol
                vol2Array[(mc1,mc2)] = volerr

        return volArray, vol2Array, foundArray, missedArray, effvmass, errvmass


    for j,mc in enumerate(mass_bins.centres()[0]):

        # filter out injections not in this mass bin
        newfound = filter_injections_by_mass( found, mass_bins, j, bin_type)
        newmissed = filter_injections_by_mass( missed, mass_bins, j, bin_type)

        foundArray[(mc,)] = len(newfound)
        missedArray[(mc,)] = len(newmissed)

        # compute the volume using this injection set
        meaneff, efferr, meanvol, volerr = mean_efficiency_volume(newfound, newmissed, dbins)
        effvmass.append(meaneff)
        errvmass.append(efferr)
        volArray[(mc,)] = meanvol
        vol2Array[(mc,)] = volerr

    return volArray, vol2Array, foundArray, missedArray, effvmass, errvmass


def log_volume_derivative_fit(x, vols, xhat):
    '''
    Performs a linear least squares fit for each mass bin to find the 
    (logarithmic) derivative of the search volume vs x at the given xhat.
    '''
    if numpy.min(vols) == 0:
        print >> sys.stderr, "Warning: cannot fit log volume derivative as all volumes are zero!"
        return (0,0)

    coeffs, resids, rank, svs, rcond = numpy.polyfit(x, numpy.log(vols), 1, full=True)
    if coeffs[0] < 0:
        coeffs[0] = 0  #negative derivatives may arise from rounding error
        print >> sys.stderr, "Warning: Derivative fit resulted in Lambda =", coeffs[0]
        print >> sys.stderr, "The value Lambda = 0 was substituted"

    return coeffs


def get_background_livetime(connection, verbose=False):
    '''
    Query the database for the background livetime for the input instruments set.
    This is equal to the sum of the livetime of the time slide experiments.
    '''
    query = """
    SELECT instruments, duration
    FROM experiment_summary
    JOIN experiment ON experiment_summary.experiment_id == experiment.experiment_id
    WHERE experiment_summary.datatype == "slide";
    """

    bglivetime = {}
    for inst,lt in connection.cursor().execute(query):
        inst =  frozenset(lsctables.instrument_set_from_ifos(inst))
        try:
            bglivetime[inst] += lt
        except KeyError:
            bglivetime[inst] = lt

    if verbose:
        for inst in bglivetime.keys():
            print >>sys.stdout,"The background livetime for time slide experiments on %s data: %d seconds (%.2f years)" % (','.join(sorted(list(inst))),bglivetime[inst],bglivetime[inst]/float(constants.LAL_YRJUL_SI))

    return bglivetime




