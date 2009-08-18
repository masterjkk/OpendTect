/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiegeocalculator.cc,v 1.25 2009-08-18 13:17:24 cvsbruno Exp $";


#include "arraynd.h"
#include "arrayndutils.h"
#include "arrayndimpl.h"

#include "fft.h"
#include "sorting.h"
#include "wavelet.h"
#include "hilberttransform.h"
#include "genericnumer.h"
#include "welltiegeocalculator.h"
#include "welltieunitfactors.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "welltrack.h"
#include "welltiesetup.h"
#include "welld2tmodel.h"
#include "survinfo.h"

#include <complex>
#include <algorithm>

WellTieGeoCalculator::WellTieGeoCalculator( const WellTieParams* p,
					    const Well::Data* wd )
		: params_(*p)
		, wtsetup_(p->getSetup())	
		, wd_(*wd) 
		, denfactor_(params_.getUnits().denFactor())
		, velfactor_(params_.getUnits().velFactor())
{
}    

//each sample is converted to a time using the travel time log
//the correspondance between samples and depth values provides a first
//TWT approx.
Well::D2TModel* WellTieGeoCalculator::getModelFromVelLog( const char* vellog,
							  bool doclean )
{
    const Well::Log& log = *wd_.logs().getLog( vellog );
    TypeSet<float> vals, d2t, dpt, time, depth;

    dpt += -wd_.track().value(0);
    vals += 0;
    for ( int idx=0; idx<log.size(); idx++ )
    {
	vals += log.valArr()[idx];
	dpt  += log.dah( idx );
    }

    if ( doclean )
    {
	interpolateLogData( dpt, log.dahStep(true), true );
	interpolateLogData( vals, log.dahStep(true), false );
    }

    mAllocVarLenArr( int, zidxs, vals.size() );
    if ( !wtsetup_.issonic_ )
    {
	for ( int idx=0; idx<vals.size(); idx++ )
	{
	    time +=  vals[idx]*velfactor_*0.001;
	    depth += dpt[idx];
	    zidxs[idx] += idx;
	}
    }
    else
    {
	TWT2Vel( vals, dpt, d2t, false );

	int idx=0;
	while ( idx < vals.size() )
	{
	    time  += d2t[idx];
	    depth += dpt[idx];
	    zidxs[idx] += idx;
	    idx++;
	}
    }
    sort_coupled( time.arr(), mVarLenArr(zidxs), vals.size() );

    Well::D2TModel* d2tnew = new Well::D2TModel;
    for ( int dahidx=0; dahidx<depth.size(); dahidx++ )
	d2tnew->add( depth[dahidx], time[dahidx] );

    return d2tnew;
}


void WellTieGeoCalculator::setVelLogDataFromModel(
				          const Array1DImpl<float>& depthvals,  					  const Array1DImpl<float>& velvals, 
					  Array1DImpl<float>& outp	) 
{
    const Well::D2TModel* d2t = wd_.d2TModel();
    TypeSet<float> time, depth, veldata;
    
    for ( int idx=0; idx<depthvals.info().getSize(0); idx++ )
    {
	depth += depthvals.get(idx);
 	time  += d2t->getTime(depthvals.get(idx));
    }
    TWT2Vel( time, depth, veldata, true );

    for ( int idx=0; idx<depthvals.info().getSize(0); idx++ )
	outp.setValue( idx, veldata[idx] );
}


Well::D2TModel* WellTieGeoCalculator::getModelFromVelLogData(
					    const Array1DImpl<float>& velvals, 
					    const Array1DImpl<float>& dptvals )
{
    const int datasz = dptvals.info().getSize(0);
    Well::D2TModel* d2tnew = new Well::D2TModel;
    //set KB depth/time
    d2tnew->add ( -wd_.track().value(0) , 0 );
    //set other depth/times 
    for ( int idx=1; idx<datasz; idx++ )
	d2tnew->add( dptvals.get(idx), velvals.get(idx) );
    return d2tnew;
}


//Small TWT/Interval Velocity converter
#define mFactor 10e6
void WellTieGeoCalculator::TWT2Vel( const TypeSet<float>& timevel,
				     const TypeSet<float>& dpt,	
				     TypeSet<float>& outp, bool t2vel  )
{
    outp += 0;
    if ( t2vel )
    {
	for ( int idx=1; idx<timevel.size(); idx++ )
	    outp +=  ( timevel[idx]-timevel[idx-1] )
		    /( (dpt[idx]-dpt[idx-1])/velfactor_*2 );
	outp[0] = outp[1];
    }
    else 
    {
	for ( int idx=1; idx<timevel.size(); idx++ )
	    outp +=  2*( dpt[idx]-dpt[idx-1] )*timevel[idx]/velfactor_;

	for ( int idx=1;  idx<timevel.size(); idx++ )
	    outp[idx] += outp[idx-1];
    }
}


void WellTieGeoCalculator::stretchArr( const Array1DImpl<float>& inp,
				       Array1DImpl<float>& outp, int idxstart,
				       int idxstop, int idxpick, int idxlast )
{
    const int datasz = inp.info().getSize(0);
    const float stretchfac = ( idxstop-idxstart )/(float)(idxpick-idxstart); 
    const float squeezefac = ( idxlast-idxstop  )/(float)(idxlast-idxpick);
   
    float val = 0;
    for ( int idx=idxstart; idx<idxstop; idx++ )
    {
	float curval = ( idx - idxstart )*stretchfac + idxstart;
	int curidx = (int)curval;
	if ( curidx >= datasz-1 || curidx < 0) continue;
	interpolAtIdx( inp.get( curidx ), inp.get( curidx+1), curval, val );
	outp.setValue( idx, val );
    }
    for ( int idx=idxstop; idx<idxlast; idx++ )
    {
	float curval = ( idx - idxlast )*squeezefac + idxlast;
	int curidx = (int)curval;
	if ( curidx >= datasz-1 || curidx < 0 ) continue;
	interpolAtIdx( inp.get( curidx ), inp.get( curidx+1), curval, val );
	outp.setValue( idx , val );
    }
}


int WellTieGeoCalculator::getIdx( const Array1DImpl<float>& inp, float pos )
{
    int idx = 0;
    while ( inp.get(idx)<pos )
    {
	if( idx == inp.info().getSize(0)-1 )
	    break;
	idx++;
    }
    return idx;
}


void WellTieGeoCalculator::interpolAtIdx( float prevval, float postval,
				 	float curval, float& outval )
{
    int curidx = (int)curval;
    outval = ( curval - curidx ) * ( postval - prevval ) + prevval;
}


//Small Data Interpolator, specially designed for log (dah) data
//assumes no possible negative values in dens or vel log
//will replace them by interpolated values if so. 
void WellTieGeoCalculator::interpolateLogData( TypeSet<float>& data,
					       float dahstep, bool isdah )
{
    int startidx = getFirstDefIdx( data );
    int lastidx = getLastDefIdx( data );

    for ( int idx=startidx; idx<lastidx; idx++)
    {
	//no negative values in dens or vel log assumed
	if ( idx && !isdah && ( mIsUdf(data[idx]) || data[idx]<0 ) )
	    data[idx] = data[idx-1];
	if ( idx && isdah && (mIsUdf(data[idx]) || data[idx]<data[idx-1]
	     || data[idx] >= data[lastidx])  )
	    data[idx] = data[idx-1] + dahstep;
    }
    for ( int idx=0; idx<startidx; idx++ )
	data[idx] = data[startidx];
    for ( int idx=lastidx; idx<data.size(); idx++ )
	data[idx] = data[lastidx];
}


int WellTieGeoCalculator::getFirstDefIdx( const TypeSet<float>& logdata )
{
    int idx = 0;
    while ( mIsUdf(logdata[idx]) )
	idx++;
    return idx;
}


int WellTieGeoCalculator::getLastDefIdx( const TypeSet<float>& logdata )
{
    int idx = logdata.size()-1;
    while ( mIsUdf( logdata[idx] ) )
	idx--;
    return idx;
}


bool WellTieGeoCalculator::isValidLogData( const TypeSet<float>& logdata )
{
    if ( logdata.size() == 0 || getFirstDefIdx(logdata) > logdata.size() )
	return false;
    return true;
}



//low pass filter, almost similar as this of the freqfilter attribute
//TODO put in algo
#define mDoTransform(tf,isstraight,inp,outp,sz) \
{   \
    tf.setInputInfo(Array1DInfoImpl(sz));\
    tf.setDir(isstraight);\
    tf.init();\
    tf.transform(*inp,*outp);\
}
void WellTieGeoCalculator::lowPassFilter( Array1DImpl<float>& vals, float cutf )
{
    const int filtersz = vals.info().getSize(0);
    if ( filtersz < 10 ) return;
    const int bordersz = 51*20;
    if ( bordersz > filtersz ) return;
    const float df = FFT::getDf( params_.dpms_.timeintv_.step, filtersz );

    Array1DImpl<float>* orgvals = new Array1DImpl<float>( filtersz );
    Array1DImpl<float>* filterborders = new Array1DImpl<float>( 2*bordersz );
    Array1DImpl<float>* freqarr = new Array1DImpl<float>( filtersz ); 
    Array1DImpl<float_complex>* cfreqoutp 
				= new Array1DImpl<float_complex>(filtersz);
    memcpy( orgvals->getData(), vals.getData(), sizeof(float)*filtersz );
    
    for ( int idx=0; idx<filtersz; idx++ )
    {
	cfreqoutp->set( idx, 0 );
	freqarr->set( idx, idx*df ); 
    }
    for ( int idx=0; idx<bordersz; idx++ )
	filterborders->set( idx, 1 ); 

    ArrayNDWindow window( Array1DInfoImpl(2*bordersz), false, "CosTaper", .05 );
    window.apply( filterborders );

    HilbertTransform hil;
    hil.setCalcRange( 0, filtersz, 0 );
    Array1DImpl<float_complex>* cvals =new Array1DImpl<float_complex>(filtersz);
    mDoTransform( hil, true, orgvals, cvals, filtersz );
    delete orgvals;
    
    window.apply( cvals );
    const float avg = computeAvg( cvals ).real();
    removeBias( cvals );
    
    FFT fft(false);
    Array1DImpl<float_complex>* cfreqinp 
			= new Array1DImpl<float_complex>(filtersz);
    mDoTransform( fft, true, cvals, cfreqinp, filtersz );
    delete cvals;

    const float infborderfreq = cutf - bordersz/2*df; 
    const float supborderfreq = cutf + bordersz/2*df; 

    int idarray = 0;
    for ( int idx=0; idx<filtersz/2; idx++ )
    {
	float_complex outpval, revoutpval;
	const int revidx = filtersz-idx-1;
	if ( freqarr->get(idx) < infborderfreq )
	{
	    outpval = cfreqinp->get(idx);
	    revoutpval = cfreqinp->get( revidx );
	}
	else if ( freqarr->get(idx) < supborderfreq 
		&& freqarr->get(idx) > infborderfreq  )
	{
	    outpval = cfreqinp->get(idx)*filterborders->get( idarray );
	    revoutpval = cfreqinp->get(revidx)
				*filterborders->get( bordersz-1-idarray );
	    idarray++;
	}
	cfreqoutp->set( idx, outpval );
	cfreqoutp->set( filtersz-idx-1, revoutpval );
    }
     delete cfreqinp; delete filterborders;

    Array1DImpl<float_complex>* coutp =new Array1DImpl<float_complex>(filtersz);
    mDoTransform( fft, false, cfreqoutp, coutp, filtersz );
    delete cfreqoutp;
    const int threshold = 100; 
    const int actfiltersz = filtersz/threshold;
    for ( int idx=actfiltersz; idx<filtersz; idx++ )
    {
	const float val = coutp->get( idx ).real() + avg;
	vals.set( idx, val );
    }
    for ( int idx=0; idx<actfiltersz; idx++ )
	vals.set( idx, vals.get( actfiltersz + 1 ) );
}


//resample data. If higher step, linear interpolation
//else nearest sample
void WellTieGeoCalculator::resampleData( const Array1DImpl<float>& invals,
				 Array1DImpl<float>& outvals, float step )
{
    int datasz = invals.info().getSize(0);
    if ( step >1  )
    {
	for ( int idx=0; idx<datasz-1; idx++ )
	{
	    float stepval = ( invals.get(idx+1) - invals.get(idx) ) / step;
	    for ( int stepidx=0; stepidx<step; stepidx++ )
		outvals.setValue( idx, invals.get(idx) + stepidx*stepval ); 
	}
	outvals.setValue( datasz-1, invals.get(datasz-1) );
    }
    else 
    {
	for ( int idx=0; idx<datasz*step; idx+=(int)(1/step) ) 
	    outvals.setValue( idx, invals.get(idx) );
    }
}


void WellTieGeoCalculator::computeAI( const Array1DImpl<float>& velvals,
				   const Array1DImpl<float>& denvals,
				   Array1DImpl<float>& aivals )
{
    const int datasz = aivals.info().getSize(0);
    const bool issonic = params_.getSetup().issonic_;
    float prevval = 0;
    for ( int idx=0; idx<datasz; idx++ )
    {
	float velval = issonic ? velvals.get(idx) : 1/velvals.get(idx);
	float denval = denvals.get( idx );
	float aival = denval/velval*mFactor*denfactor_*velfactor_;
	if ( mIsUdf(denval) || mIsUdf(velval) )
	    aival = prevval;
	aivals.setValue( idx, aival );
	prevval = aival;
    }
}


//Compute reflectivity values at display sample step (Survey step)
void WellTieGeoCalculator::computeReflectivity(const Array1DImpl<float>& aivals,
					       Array1DImpl<float>& reflvals,
					       int shiftstep )
{
    float prevval, ai1, ai2, rval = 0; 
    const int sz = reflvals.info().getSize(0);
    if ( sz<2 ) return;

    for ( int idx=0; idx<sz-1; idx++ )
    {
	ai2 = aivals.get( shiftstep*(idx+1));
	ai1 = aivals.get( shiftstep*(idx)  );	   

	if ( (ai1 + ai2 ) == 0 )
	    rval = prevval;
	else if ( !mIsUdf( ai1 ) || !mIsUdf( ai2 ) )
	    rval =  ( ai2 - ai1 ) / ( ai2 + ai1 );     
	
	reflvals.setValue( idx, rval ); 
	prevval = rval;
    }
    reflvals.setValue( 0, reflvals.get(1) ); 
    reflvals.setValue( sz-1, reflvals.get(sz-2) ); 
}


void WellTieGeoCalculator::convolveWavelet( const Array1DImpl<float>& wvltvals,
					const Array1DImpl<float>& reflvals,
					Array1DImpl<float>& synvals, int widx )
{
    int reflsz = reflvals.info().getSize(0);
    int wvltsz = wvltvals.info().getSize(0);
    float* outp = new float[reflsz];

    GenericConvolve( wvltsz, -widx, wvltvals.getData(),
		     reflsz, 0  	 , reflvals.getData(),
		     reflsz, 0  	 , outp );

    memcpy( synvals.getData(), outp, reflsz*sizeof(float));
    delete outp;
}


#define mNoise 0.05
void WellTieGeoCalculator::deconvolve( const Array1DImpl<float>& tinputvals,
				       const Array1DImpl<float>& tfiltervals,
				       Array1DImpl<float>& deconvals, 
				       int wvltsz )
{
    const int sz = tinputvals.info().getSize(0);
    const int filtersz = tfiltervals.info().getSize(0);
    if ( !sz || !filtersz || filtersz<wvltsz ) return;

    ArrayNDWindow window( Array1DInfoImpl(filtersz), false, "CosTaper", 0.1 );
    Array1DImpl<float>* inputvals = new Array1DImpl<float>( filtersz );
    Array1DImpl<float>* filtervals = new Array1DImpl<float>( filtersz );
    memcpy( inputvals->getData(), tinputvals.arr(), filtersz*sizeof(float) );
    memcpy( filtervals->getData(), tfiltervals.arr(), filtersz*sizeof(float) );
    window.apply( inputvals );		removeBias( inputvals );
    window.apply( filtervals );		removeBias( filtervals );

    HilbertTransform hil;
    hil.setCalcRange(0, filtersz, 0);
    Array1DImpl<float_complex>* cinputvals = 
				new Array1DImpl<float_complex>( filtersz );
    mDoTransform( hil, true, inputvals, cinputvals, filtersz );
    delete inputvals;
    Array1DImpl<float_complex>* cfiltervals = 
				new Array1DImpl<float_complex>( filtersz );
    hil.setCalcRange(0, filtersz, 0);
    mDoTransform( hil, true, filtervals, cfiltervals, filtersz );
    delete filtervals;
   
    FFT fft(false);
    Array1DImpl<float_complex>* cfreqinputvals = 
				new Array1DImpl<float_complex>( filtersz );
    mDoTransform( fft, true, cinputvals, cfreqinputvals, filtersz );
    delete cinputvals;
    Array1DImpl<float_complex>* cfreqfiltervals = 
				new Array1DImpl<float_complex>( filtersz );
    mDoTransform( fft, true, cfiltervals, cfreqfiltervals, filtersz );

    Spectrogram spec;
    Array1DImpl<float_complex>* cspecfiltervals = 
				new Array1DImpl<float_complex>( filtersz );
    mDoTransform( spec, true, cfiltervals, cspecfiltervals, filtersz );
    delete cfiltervals;

    float_complex wholespec = 0;
    float_complex noise = mNoise/filtersz;
    for ( int idx=0; idx<filtersz; idx++ )
	wholespec += cspecfiltervals->get( idx );  
    float_complex cnoiseshift = noise*wholespec;
   
    Array1DImpl<float_complex>* cdeconvvals = 
				new  Array1DImpl<float_complex>( filtersz ); 
    for ( int idx=0; idx<filtersz; idx++ )
    {
	float_complex inputval = cfreqinputvals->get(idx);
	float_complex filterval = cfreqfiltervals->get(idx);

	double rfilterval = filterval.real();
	double ifilterval = filterval.imag();
	float_complex conjfilterval = float_complex( rfilterval ,-ifilterval ); 

	float_complex num = inputval * conjfilterval;
	float_complex denom = filterval * conjfilterval + cnoiseshift;
	float_complex res = num / denom;

	cdeconvvals->setValue( idx, res );
    }
    delete cfreqinputvals; delete  cfreqfiltervals;

    float avg = 0;
    for ( int idx=0; idx<filtersz; idx++ )
	avg += abs( cdeconvvals->get( idx ) )/filtersz;
    for ( int idx=0; idx<filtersz; idx++ )
    {
	if ( abs( cdeconvvals->get( idx ) ) < avg/4 )
	    cdeconvvals->set( idx, 0 );
    }

    Array1DImpl<float_complex>* ctimedeconvvals = 
				new Array1DImpl<float_complex>( filtersz );
    mDoTransform( fft, false, cdeconvvals, ctimedeconvvals, filtersz );
    delete cdeconvvals;

    int mid = (int)(filtersz)/2;
    for ( int idx=0; idx<=mid; idx++ )
	deconvals.set( idx, ctimedeconvvals->get( mid-idx ).real() );
    for ( int idx=mid+1; idx<filtersz; idx++ )
	deconvals.set( idx, ctimedeconvvals->get( filtersz-idx+mid ).real() );
    delete ctimedeconvvals;
}


void WellTieGeoCalculator::reverseWavelet( Wavelet& wvlt )
{
    const int wvltsz = wvlt.size();
    Array1DImpl<float> wvltvals (wvltsz);
    memcpy( wvltvals.getData(), wvlt.samples(), wvltsz*sizeof(float) );

    for ( int idx=0; idx<wvltsz/2; idx++ )
    {
	wvlt.samples()[idx] = wvltvals.get(wvltsz-idx-1);
	wvlt.samples()[wvltsz-idx-1] = wvltvals.get( idx );
    }
}


void WellTieGeoCalculator::crosscorr( const Array1DImpl<float>& seisvals, 
				     const Array1DImpl<float>& synthvals,
       				     Array1DImpl<float>& outpvals	)
{
    const int datasz = seisvals.info().getSize(0);
    float* outp = new float[datasz];
    genericCrossCorrelation( datasz, 0, seisvals,
			     datasz, 0, synthvals,
			     datasz, -datasz/2, outp);
    memcpy( outpvals.getData(), outp, datasz*sizeof(float));
    delete outp;
}


void WellTieGeoCalculator::zeroPadd( const Array1DImpl<float_complex>& inp, 
				      Array1DImpl<float_complex>& outp )
{
    const int psz = outp.info().getSize(0); 
    for ( int idx=0; idx<psz; idx++ )
	outp.setValue( idx, idx>=psz/3 && idx<2*psz/3? inp.get(idx-psz/3):0 );
}
