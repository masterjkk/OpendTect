/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vissurvscene.cc,v 1.33 2002-06-25 15:22:39 nanne Exp $";

#include "vissurvscene.h"
#include "visplanedatadisplay.h"
#include "visdataman.h"
#include "visevent.h"
#include "vistransform.h"
#include "position.h"
#include "vissurvpickset.h"
#include "survinfo.h"
#include "linsolv.h"
#include "visannot.h"
#include "vislight.h"
#include "iopar.h"

#include <limits.h>

mCreateFactoryEntry( visSurvey::Scene );

const char* visSurvey::Scene::xyzobjprefixstr = "XYZ Object ";
const char* visSurvey::Scene::noxyzobjstr = "No XYZ Objects";
const char* visSurvey::Scene::xytobjprefixstr = "XYT Object ";
const char* visSurvey::Scene::noxytobjstr = "No XYT Objects";
const char* visSurvey::Scene::inlcrltobjprefixstr = "InlCrl Object ";
const char* visSurvey::Scene::noinlcrltobjstr = "No InlCrl Objects";
const char* visSurvey::Scene::annottxtstr = "Show text";
const char* visSurvey::Scene::annotscalestr = "Show scale";
const char* visSurvey::Scene::annotcubestr = "Show cube";


visSurvey::Scene::Scene()
    : inlcrltransformation( SPM().getInlCrlTransform() )
    , timetransformation( SPM().getAppvelTransform() )
    , annot( 0 )
    , eventcatcher( visBase::EventCatcher::create())
    , mouseposchange( this )
{
    addObject( const_cast<visBase::Transformation*>(
						SPM().getDisplayTransform()));
    addObject( eventcatcher );
    eventcatcher->setEventType( visBase::MouseMovement );
    eventcatcher->eventhappened.notify( mCB( this, Scene, mouseMoveCB ));
    addObject( const_cast<visBase::Transformation*>(timetransformation) );
    addObject( const_cast<visBase::Transformation*>(inlcrltransformation) );

    annot = visBase::Annotation::create();
    setCube();
    annot->setText( 0, "In-line" );
    annot->setText( 1, "Cross-line" );
    annot->setText( 2, SI().zIsTime() ? "TWT" : "Depth" );
    addInlCrlTObject( annot );

    visBase::DirectionalLight* light = visBase::DirectionalLight::create();
    light->setDirection( 0, 0, 1 );
    addXYZObject( light );

    light = visBase::DirectionalLight::create();
    light->setDirection( 0, 0, -1 );
    addXYZObject( light );

    light = visBase::DirectionalLight::create();
    light->setDirection( 0, 1, 0 );
    addXYZObject( light );

    light = visBase::DirectionalLight::create();
    light->setDirection( 0,-1, 0 );
    addXYZObject( light );

    light = visBase::DirectionalLight::create();
    light->setDirection( 1, 0, 0 );
    addXYZObject( light );

    light = visBase::DirectionalLight::create();
    light->setDirection(-1, 0, 0 );
    addXYZObject( light );
}


visSurvey::Scene::~Scene()
{
    eventcatcher->eventhappened.remove( mCB( this, Scene, mouseMoveCB ));
}


void visSurvey::Scene::setCube()
{
    if ( !annot ) return;
    BinIDRange hrg = SI().range();
    StepInterval<double> vrg = SI().zRange();

    BinID c0( hrg.start.inl, hrg.start.crl ); 
    BinID c1( hrg.stop.inl, hrg.start.crl ); 
    BinID c2( hrg.stop.inl, hrg.stop.crl ); 
    BinID c3( hrg.start.inl, hrg.stop.crl );

    annot->setCorner( 0, c0.inl, c0.crl, vrg.start );
    annot->setCorner( 1, c1.inl, c1.crl, vrg.start );
    annot->setCorner( 2, c2.inl, c2.crl, vrg.start );
    annot->setCorner( 3, c3.inl, c3.crl, vrg.start );
    annot->setCorner( 4, c0.inl, c0.crl, vrg.stop );
    annot->setCorner( 5, c1.inl, c1.crl, vrg.stop );
    annot->setCorner( 6, c2.inl, c2.crl, vrg.stop );
    annot->setCorner( 7, c3.inl, c3.crl, vrg.stop );
}


void visSurvey::Scene::addXYZObject( SceneObject* obj )
{
    int insertpos = getFirstIdx( timetransformation );
    insertObject( insertpos, obj );
}


void visSurvey::Scene::addXYTObject( SceneObject* obj )
{
    int insertpos = getFirstIdx( inlcrltransformation );
    insertObject( insertpos, obj );
}


void visSurvey::Scene::addInlCrlTObject( SceneObject* obj )
{
    addObject( obj );
}


void visSurvey::Scene::insertObject( int idx, SceneObject* obj )
{
    mDynamicCastGet( SurveyObject*, survobj, obj );
    if ( survobj && survobj->getMovementNotification() )
    {
	survobj->getMovementNotification()->notify(
		mCB( this,visSurvey::Scene,filterPicks ));
    }

    SceneObjectGroup::insertObject( idx, obj );
}


void visSurvey::Scene::addObject( SceneObject* obj )
{
    mDynamicCastGet( SurveyObject*, survobj, obj );
    if ( survobj && survobj->getMovementNotification() )
    {
	survobj->getMovementNotification()->notify(
		mCB( this,visSurvey::Scene,filterPicks ));
    }

    SceneObjectGroup::addObject( obj );
}


void visSurvey::Scene::removeObject( int idx )
{
    SceneObject* obj = getObject( idx );
    mDynamicCastGet( SurveyObject*, survobj, obj );
    if ( survobj && survobj->getMovementNotification() )
    {
	survobj->getMovementNotification()->remove(
		mCB( this,visSurvey::Scene,filterPicks ));
    }

    SceneObjectGroup::removeObject( idx );
}


void visSurvey::Scene::showAnnotText( bool yn )
{
    annot->showText( yn );
}


bool  visSurvey::Scene::isAnnotTextShown() const
{
    return annot->isTextShown();
}


void visSurvey::Scene::showAnnotScale( bool yn )
{
    annot->showScale( yn );
}


bool visSurvey::Scene::isAnnotScaleShown() const
{
    return annot->isScaleShown();
}


void visSurvey::Scene::showAnnot( bool yn )
{
    annot->turnOn( yn );
}


bool visSurvey::Scene::isAnnotShown() const
{
    return annot->isOn();
}


Geometry::Pos visSurvey::Scene::getMousePos( bool xyt ) const
{
   if ( xyt ) return xytmousepos;
   
   Geometry::Pos res = xytmousepos;
   BinID binid = SI().transform( Coord( res.x, res.y ));

   res.x = binid.inl;
   res.y = binid.crl;
   return res;
}


void visSurvey::Scene::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::SceneObject::fillPar( par, saveids );

    TypeSet<int> xyzkids;
    TypeSet<int> xytkids;
    TypeSet<int> inlcrltkids;

    int kid = 0;
    while ( getObject(kid)!=timetransformation )
    {
	if ( getObject(kid)==SPM().getDisplayTransform() ||
		getObject(kid)==eventcatcher ||
	        typeid(*getObject(kid))==typeid(visBase::DirectionalLight) )
	{ kid++; continue; }

	xyzkids += getObject(kid)->id();
	if ( saveids.indexOf( getObject(kid)->id()) ==-1 )
	{
	    saveids += getObject(kid)->id();
	}

	kid++;
    }

    kid++;

    while ( getObject(kid)!=inlcrltransformation )
    {
	xytkids += getObject(kid)->id();
	if ( saveids.indexOf( getObject(kid)->id()) ==-1 )
	{
	    saveids += getObject(kid)->id();
	}

	kid++;
    }

    kid++ ;

    for ( ; kid<size(); kid++ )
    {
	if ( getObject(kid)==annot ) continue;

	inlcrltkids += getObject(kid)->id();
	if ( saveids.indexOf( getObject(kid)->id()) ==-1 )
	{
	    saveids += getObject(kid)->id();
	}
    }

    par.set( noxyzobjstr, xyzkids.size() );
    for ( int idx=0; idx<xyzkids.size(); idx++ )
    {
	BufferString key = xyzobjprefixstr; key += idx;
	par.set( key, xyzkids[idx] );
    }

    par.set( noxytobjstr, xytkids.size() );
    for ( int idx=0; idx<xytkids.size(); idx++ )
    {
	BufferString key = xytobjprefixstr; key += idx;
	par.set( key, xytkids[idx] );
    }

    par.set( noinlcrltobjstr, inlcrltkids.size() );
    for ( int idx=0; idx<inlcrltkids.size(); idx++ )
    {
	BufferString key = inlcrltobjprefixstr; key += idx;
	par.set( key, inlcrltkids[idx] );
    }

    bool txtshown = isAnnotTextShown();
    par.setYN( annottxtstr, txtshown );
    bool scaleshown = isAnnotScaleShown();
    par.setYN( annotscalestr, scaleshown );
    bool cubeshown = isAnnotShown();
    par.setYN( annotcubestr, cubeshown );
}


int visSurvey::Scene::usePar( const IOPar& par )
{
    eventcatcher->eventhappened.remove( mCB( this, Scene, mouseMoveCB ));
    removeAll();

    inlcrltransformation = SPM().getInlCrlTransform();
    timetransformation = SPM().getAppvelTransform();
    eventcatcher = visBase::EventCatcher::create();

    addObject( const_cast<visBase::Transformation*>(
						SPM().getDisplayTransform()));
    addObject( eventcatcher );
    eventcatcher->setEventType( visBase::MouseMovement );
    eventcatcher->eventhappened.notify( mCB( this, Scene, mouseMoveCB ));
    addObject( const_cast<visBase::Transformation*>(timetransformation) );
    addObject( const_cast<visBase::Transformation*>(inlcrltransformation) );

    BinIDRange hrg = SI().range();
    StepInterval<double> vrg = SI().zRange();

    annot = visBase::Annotation::create();
    BinID c0( hrg.start.inl, hrg.start.crl ); 
    BinID c1( hrg.stop.inl, hrg.start.crl ); 
    BinID c2( hrg.stop.inl, hrg.stop.crl ); 
    BinID c3( hrg.start.inl, hrg.stop.crl );

    annot->setCorner( 0, c0.inl, c0.crl, vrg.start );
    annot->setCorner( 1, c1.inl, c1.crl, vrg.start );
    annot->setCorner( 2, c2.inl, c2.crl, vrg.start );
    annot->setCorner( 3, c3.inl, c3.crl, vrg.start );
    annot->setCorner( 4, c0.inl, c0.crl, vrg.stop );
    annot->setCorner( 5, c1.inl, c1.crl, vrg.stop );
    annot->setCorner( 6, c2.inl, c2.crl, vrg.stop );
    annot->setCorner( 7, c3.inl, c3.crl, vrg.stop );

    annot->setText( 0, "In-line" );
    annot->setText( 1, "Cross-line" );
    annot->setText( 2, SI().zIsTime() ? "TWT" : "Depth" );
    addInlCrlTObject( annot );

    visBase::DirectionalLight* light = visBase::DirectionalLight::create();
    light->setDirection( 0, 0, 1 );
    addXYZObject( light );

    light = visBase::DirectionalLight::create();
    light->setDirection( 0, 0, -1 );
    addXYZObject( light );

    light = visBase::DirectionalLight::create();
    light->setDirection( 0, 1, 0 );
    addXYZObject( light );

    light = visBase::DirectionalLight::create();
    light->setDirection( 0,-1, 0 );
    addXYZObject( light );

    light = visBase::DirectionalLight::create();
    light->setDirection( 1, 0, 0 );
    addXYZObject( light );

    light = visBase::DirectionalLight::create();
    light->setDirection(-1, 0, 0 );
    addXYZObject( light );

    int res = visBase::SceneObject::usePar( par );

    if ( res!= 1 ) return res;

    int nrxyzobj;
    if ( !par.get( noxyzobjstr, nrxyzobj )) return -1;

    TypeSet<int> xyzobjids( nrxyzobj, -1 );
    for ( int idx=0; idx<xyzobjids.size(); idx++ )
    {
	BufferString key = xyzobjprefixstr;
	key += idx;

	if ( !par.get( key, xyzobjids[idx] )) return -1;
	if ( !visBase::DM().getObj( xyzobjids[idx] ) ) return 0;
    }

    int nrxytobj;
    if ( !par.get( noxytobjstr, nrxytobj )) return -1;

    TypeSet<int> xytobjids( nrxytobj, -1 );
    for ( int idx=0; idx<xytobjids.size(); idx++ )
    {
	BufferString key = xytobjprefixstr;
	key += idx;

	if ( !par.get( key, xytobjids[idx] )) return -1;
	if ( !visBase::DM().getObj( xytobjids[idx] ) ) return 0;
    }

    int noinlcrltobj;
    if ( !par.get( noinlcrltobjstr, noinlcrltobj )) return -1;

    TypeSet<int> inlcrlobjids( noinlcrltobj, -1 );
    for ( int idx=0; idx<inlcrlobjids.size(); idx++ )
    {
	BufferString key = inlcrltobjprefixstr;
	key += idx;

	if ( !par.get( key, inlcrlobjids[idx] )) return -1;
	if ( !visBase::DM().getObj( inlcrlobjids[idx] ) ) return 0;
    }

    for ( int idx=0; idx<xyzobjids.size(); idx++ )
    {
	mDynamicCastGet( visBase::SceneObject*, so,
				    visBase::DM().getObj( xyzobjids[idx] ));
	if ( so ) addXYZObject( so );
    }

    for ( int idx=0; idx<xytobjids.size(); idx++ )
    {
	mDynamicCastGet( visBase::SceneObject*, so,
				visBase::DM().getObj( xytobjids[idx] ));
	if ( so ) addXYTObject( so );
    }

    for ( int idx=0; idx<inlcrlobjids.size(); idx++ )
    {
	mDynamicCastGet( visBase::SceneObject*, so,
			visBase::DM().getObj( inlcrlobjids[idx] ));
	if ( so ) addInlCrlTObject( so );
    }

    bool txtshown;
    if ( !par.getYN( annottxtstr, txtshown ) ) return -1;
    showAnnotText( txtshown );
    bool scaleshown;
    if ( !par.getYN( annotscalestr, scaleshown ) ) return -1;
    showAnnotScale( scaleshown );
    bool cubeshown;
    if ( !par.getYN( annotcubestr, cubeshown ) ) return -1;
    showAnnot( cubeshown );

    return 1;
}


void visSurvey::Scene::filterPicks(CallBacker* cb)
{
    ObjectSet<SurveyObject> objects;
    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet( SurveyObject*, survobj, getObject( idx ) );
	if ( !survobj ) continue;
	if ( !survobj->getMovementNotification() ) continue;

	mDynamicCastGet( visBase::VisualObject*, visobj, getObject( idx ) );
	if ( !visobj ) continue;
	if ( !visobj->isOn() ) continue;

	objects += survobj;
    }

    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet( PickSetDisplay*, pickset, getObject( idx ) );
	if ( pickset ) pickset->filterPicks( objects, 10 );
    }
}


void visSurvey::Scene::mouseMoveCB(CallBacker* cb )
{
    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb );

    if ( eventinfo.type != visBase::MouseMovement ) return;

    const int sz = eventinfo.pickedobjids.size();
    bool validpicksurface = false;
    const visSurvey::PlaneDataDisplay* sd = 0;

    for ( int idx=0; idx<sz; idx++ )
    {
	const DataObject* pickedobj =
			    visBase::DM().getObj(eventinfo.pickedobjids[idx]);

	if ( typeid(*pickedobj)==typeid(visSurvey::PlaneDataDisplay) )
	{
	    validpicksurface = true;
	    sd = (const visSurvey::PlaneDataDisplay*) pickedobj;
	    break;
	}
    }

    if ( !validpicksurface ) return;

    xytmousepos = SPM().coordDispl2XYT(eventinfo.pickedpos);

    Geometry::Pos inlcrl = xytmousepos;
    BinID binid = SI().transform( Coord( xytmousepos.x, xytmousepos.y ));
    inlcrl.x = binid.inl;
    inlcrl.y = binid.crl;

    mouseposval = sd->getValue( inlcrl );
    mouseposchange.trigger();
}
