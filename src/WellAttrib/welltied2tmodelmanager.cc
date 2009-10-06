/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltied2tmodelmanager.cc,v 1.15 2009-10-06 09:19:54 cvsbruno Exp $";

#include "welltied2tmodelmanager.h"

#include "filegen.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "welllog.h"
#include "wellman.h"
#include "welllogset.h"
#include "welltrack.h"

#include "welltiegeocalculator.h"
#include "welltiedata.h"
#include "welltiesetup.h"

#define mMinNrTimeSamples 5
namespace WellTie
{

D2TModelMGR::D2TModelMGR( WellTie::DataHolder& dh )
	: wd_(dh.wd())
	, geocalc_(*dh.geoCalc())
	, orgd2t_(0)					    
	, prvd2t_(0)
	, emptyoninit_(false)
	, wtsetup_(dh.setup())	
	, datawriter_(new WellTie::DataWriter(&dh))			
{
    if ( !wd_ ) return;
    if ( !wd_->d2TModel() || wd_->d2TModel()->size() <= mMinNrTimeSamples )
    {
	emptyoninit_ = true;
	wd_->setD2TModel( new Well::D2TModel );
    }
    orgd2t_ = emptyoninit_ ? 0 : new Well::D2TModel( *wd_->d2TModel() );

    if ( wd_->haveCheckShotModel() 
	    		|| wd_->d2TModel()->size() <= mMinNrTimeSamples )
	setFromVelLog( dh.params()->dpms_.currvellognm_ );
} 


D2TModelMGR::~D2TModelMGR()
{
    delete datawriter_;
    if ( prvd2t_ ) delete prvd2t_;
}


Well::D2TModel& D2TModelMGR::d2T()
{
    return *wd_->d2TModel();
}


void D2TModelMGR::setFromVelLog( const char* lognm,  bool docln )
{setAsCurrent( geocalc_.getModelFromVelLog(lognm,docln) );}


void D2TModelMGR::shiftModel( float shift)
{
    TypeSet<float> dah, time;

    Well::D2TModel* d2t =  new Well::D2TModel( d2T() );
    //copy old d2t
    for (int idx = 0; idx<d2t->size(); idx++)
    {
	time += d2t->value( idx );
	dah  += d2t->dah( idx );
    }

    //replace by shifted one
    d2t->erase();
    //set KB depth
    d2t->add ( wd_->track().dah(0)-wd_->track().value(0) , 0 );
    for ( int dahidx=1; dahidx<dah.size(); dahidx++ )
	d2t->add( dah[dahidx], time[dahidx] + shift );

    setAsCurrent( d2t );
}


void D2TModelMGR::replaceTime( const Array1DImpl<float>& timevals )
{
    TypeSet<float> dah, time;
    Well::D2TModel* d2t =  new Well::D2TModel( d2T() );
    //copy old d2t
    for (int idx = 0; idx<d2t->size(); idx++)
    {
	time += d2t->value( idx );
	dah  += d2t->dah( idx );
    }
    d2t->erase();
    //set KB depth
    d2t->add ( wd_->track().dah(0)-wd_->track().value(0) , 0 );
    for ( int dahidx=1; dahidx<dah.size(); dahidx++ )
	d2t->add( dah[dahidx], timevals[dahidx]);

    setAsCurrent( d2t );
}


void D2TModelMGR::setAsCurrent( Well::D2TModel* d2t )
{
    if ( !d2t || d2t->size() < 1 || d2t->value(1)<0 )
    { pErrMsg("Bad D2TMdl: ignoring"); delete d2t; return; }

    if ( prvd2t_ )
	delete prvd2t_;
    prvd2t_ =  new Well::D2TModel( d2T() );
    wd_->setD2TModel( d2t );
}


bool D2TModelMGR::undo()
{
    if ( !prvd2t_ ) return false; 
    Well::D2TModel* tmpd2t =  new Well::D2TModel( *prvd2t_ );
    setAsCurrent( tmpd2t );
    return true;
}


bool D2TModelMGR::cancel()
{
    if ( emptyoninit_ )
	wd_->d2TModel()->erase();	
    else
	setAsCurrent( orgd2t_ );
    wd_->d2tchanged.trigger();
    return true;
}


bool D2TModelMGR::updateFromWD()
{
    if ( !wd_->d2TModel() || wd_->d2TModel()->size()<1 )
       return false;	
    setAsCurrent( wd_->d2TModel() );
    return true;
}


bool D2TModelMGR::commitToWD()
{
    if ( !datawriter_->writeD2TM() ) 
	return false;

    wd_->d2tchanged.trigger();
    if ( orgd2t_ && !emptyoninit_ )
	delete orgd2t_;

    return true;
}


}; //namespace WellTie
