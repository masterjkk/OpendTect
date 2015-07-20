/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2005
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "basemapimpl.h"
#include "coord.h"

BaseMapObject::BaseMapObject( const char* nm )
    : NamedObject(nm)
    , changed(this)
    , stylechanged(this)
    , depth_(100)
{
    mDefineStaticLocalObject( Threads::Atomic<int>, treeitmid, (1000) );
    id_ = treeitmid++;
}


int BaseMapObject::nrShapes() const
{ return 0; }


const char* BaseMapObject::getShapeName( int ) const
{ return 0; }


void BaseMapObject::getPoints( int, TypeSet<Coord>& ) const
{ }


Alignment BaseMapObject::getAlignment( int shapeidx ) const
{
    return Alignment();
}


Color BaseMapObject::getColor() const
{
    if ( getFillColor(0) != Color::NoColor() )
	return getFillColor( 0 );
    else
	return getLineStyle(0)->color_;
}


const OD::RGBImage* BaseMapObject::createImage( Coord& origin,Coord& p11 ) const
{ return 0; }


const OD::RGBImage* BaseMapObject::createPreview( int approxdiagonal ) const
{ return 0; }


BaseMapMarkers::BaseMapMarkers()
    : BaseMapObject( 0 )
{}


BaseMapMarkers::~BaseMapMarkers()
{ }


void BaseMapMarkers::setMarkerStyle( int, const MarkerStyle2D& ms )
{
    if ( markerstyle_==ms )
	return;

    markerstyle_ = ms;
}


void BaseMapMarkers::getPoints( int shapeidx, TypeSet<Coord>& res) const
{
    res = positions_;
}


void BaseMapMarkers::updateGeometry()
{
    changed.trigger();
}
