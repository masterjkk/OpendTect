/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : December 2016
-*/


#include "horzvalueprovider.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "emioobjinfo.h"


ZValueProvider* HorZValueProvider::createFrom(
	const IOPar& par, const TrcKeySampling& , TaskRunner* tr )
{
    MultiID horid;
    if ( !par.get(sKey::ID(),horid) )
	return 0;

    const EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded( horid, tr );
    mDynamicCastGet(const EM::Horizon*,hor,emobj);
    if ( !hor )
	return 0;

    return new HorZValueProvider( hor );
}


HorZValueProvider::HorZValueProvider( const EM::Horizon* hor )
    : hor_(hor)
    , depthid_(-1)
{
}


void HorZValueProvider::fillPar( IOPar& par ) const
{
    par.set( ZValueProvider::sType(), HorZValueProvider::sFactoryKeyword() );
    par.set( sKey::ID(), hor_->multiID() );
}


void HorZValueProvider::setHorizon( const EM::Horizon* hor )
{
    if ( hor_.ptr() == hor )
	return;

    hor_ = hor;
}


uiString HorZValueProvider::getName() const
{ return !hor_.ptr() ? uiString::emptyString() : toUiString( hor_->name() ); }


Color HorZValueProvider::drawColor() const
{ return !hor_.ptr() ? Color::NoColor() : hor_->preferredColor(); }


int HorZValueProvider::drawWidth() const
{ return !hor_.ptr() ? 1 : hor_->preferredLineStyle().width_; }


float HorZValueProvider::getZValue( const TrcKey& pos ) const
{
    if ( !hor_.ptr() )
    {
	pErrMsg( "Horizon not set" );
	return mUdf(float);
    }

    return hor_->getZ( pos );
}


float HorZValueProvider::getZValue( const Coord& pos ) const
{
    if ( !hor_ )
    {
	pErrMsg( "Horizon not set" );
	return mUdf(float);
    }

    return hor_->getZValue( pos );
}


DepthIDSetter* HorZValueProvider::getDepthIDSetter(
	ObjectSet<ZValueProvider>&  zprovs )
{
    return new HorDepthIDSetter( zprovs );
}


void HorDepthIDSetter::go()
{
    TypeSet<MultiID> horids, sortedhorids;
    for ( int iev=0; iev<zvalprovs_.size(); iev++ )
    {
	mDynamicCastGet(HorZValueProvider*,horzprov,zvalprovs_[iev]);
	if ( !horzprov )
	    continue;
	horids.addIfNew( horzprov->horizon()->multiID() );
    }

    EM::IOObjInfo::sortHorizonsOnZValues( horids, sortedhorids );
    for ( int iev=0; iev<zvalprovs_.size(); iev++ )
    {
	mDynamicCastGet(HorZValueProvider*,horzprov,zvalprovs_[iev]);
	if ( !horzprov )
	    continue;
	const int depthidx =
	    sortedhorids.indexOf( horzprov->horizon()->multiID() );
	if ( depthidx>=0 )
	    horzprov->setDepthID( depthidx );
    }
}
