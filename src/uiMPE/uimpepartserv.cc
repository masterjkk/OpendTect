/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2004
 RCS:           $Id: uimpepartserv.cc,v 1.16 2005-08-25 14:02:13 cvskris Exp $
________________________________________________________________________

-*/

#include "uimpepartserv.h"

#include "geomelement.h"
#include "mpeengine.h"
#include "emtracker.h"
#include "survinfo.h"
#include "uimpewizard.h"
#include "uimpesetup.h"
#include "uimsg.h"
#include "uicursor.h"
#include "uisurfacerelationdlg.h"
#include "emmanager.h"
#include "emobject.h"
#include "executor.h"

const int uiMPEPartServer::evGetAttribData	= 0;
const int uiMPEPartServer::evStartSeedPick	= 1;
const int uiMPEPartServer::evEndSeedPick	= 2;
const int uiMPEPartServer::evAddTreeObject	= 3;
const int uiMPEPartServer::evShowToolbar	= 4;
const int uiMPEPartServer::evInitFromSession	= 5;


uiMPEPartServer::uiMPEPartServer( uiApplService& a, const Attrib::DescSet* ads )
    : uiApplPartServer(a)
    , attrset( ads )
    , wizard( 0 )
    , activetrackerid(-1)
    , eventattrselspec( 0 )
{
    MPE::initStandardClasses();
}


uiMPEPartServer::~uiMPEPartServer()
{
    delete wizard;
}


void uiMPEPartServer::setCurrentAttribDescSet( const Attrib::DescSet* ads )
{ attrset = ads; }


int uiMPEPartServer::getTrackerID( const MultiID& mid ) const
{
    for ( int idx=0; idx<MPE::engine().highestTrackerID(); idx++ )
    {
	if ( MPE::engine().getTracker(idx) )
	{
	    EM::ObjectID objid = MPE::engine().getTracker(idx)->objectID();
	    if ( objid==-1 )
		continue;

	    EM::EMObject* emobj = EM::EMM().getObject(objid);
	    if ( emobj && emobj->multiID()==mid )
		return idx;
	}
    }

    return -1;
}



int uiMPEPartServer::getTrackerID( const char* name ) const
{
    return MPE::engine().getTrackerByObject(name);
}


void uiMPEPartServer::getTrackerTypes( BufferStringSet& res ) const
{ MPE::engine().getAvaliableTrackerTypes(res); }


int uiMPEPartServer::addTracker( const MultiID& mid, const Coord3& pickedpos )
{
    const EM::ObjectID objid = EM::EMM().multiID2ObjectID( mid );
    EM::EMObject* emobj = EM::EMM().getObject( objid );
    if ( !emobj ) return -1;


    if ( !wizard )
	wizard = new MPE::Wizard( appserv().parent(), this );
    else
	wizard->reset();

    wizard->setRotateMode(false);

    wizard->setObject( mid, emobj->sectionID(0) );
    wizard->displayPage(MPE::Wizard::sNamePage, false );
    wizard->displayPage(MPE::Wizard::sSeedPage, false );
    wizard->displayPage(MPE::Wizard::sFinalizePage, false );
    wizard->go();

    CubeSampling csfromseeds;
    csfromseeds.zrg.start = csfromseeds.zrg.stop = pickedpos.z;
    HorSampling& hrg = csfromseeds.hrg;
    const BinID bid = SI().transform( pickedpos );
    hrg.start.inl = bid.inl - 10 * hrg.step.inl;
    hrg.stop.inl = bid.inl + 10 * hrg.step.inl;
    hrg.start.crl = bid.crl - 10 * hrg.step.crl;
    hrg.stop.crl = bid.crl + 10 * hrg.step.crl;

    expandActiveArea(csfromseeds);

    return activetrackerid;
}


bool uiMPEPartServer::addNewSection( int trackerid )
{
    MPE::EMTracker* tracker = MPE::engine().getTracker(trackerid);
    if ( !tracker )
    {
	pErrMsg("Internal error: Cannot find tracker");
	return false;
    }

    EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );

    if ( !wizard )
	wizard = new MPE::Wizard( appserv().parent(), this );
    else
	wizard->reset();

    wizard->setObject( emobj->multiID(), -1 );
    wizard->setRotateMode(false);
    wizard->displayPage(MPE::Wizard::sNamePage, false );
    wizard->displayPage(MPE::Wizard::sFinalizePage, false );
    wizard->go();

    return true;
}


bool uiMPEPartServer::addTracker( const char* trackertype )
{
    if ( !wizard )
	wizard = new MPE::Wizard( appserv().parent(), this );
    else
	wizard->reset();

    wizard->setTrackingType( trackertype );
    wizard->setRotateMode(true);
    wizard->go();

    return true;
}


MultiID uiMPEPartServer::getTrackerMultiID( int trackerid ) const
{
    const MPE::EMTracker* emt = MPE::engine().getTracker(trackerid);
    if ( emt )
    {
	const EM::EMObject* emo = EM::EMM().getObject(emt->objectID());
	if ( emo ) return emo->multiID();
    }

   return MultiID(-1);
}


bool uiMPEPartServer::canAddSeed( int trackerid ) const
{
    pErrMsg("Not impl");
    return false;
}


void uiMPEPartServer::addSeed( int trackerid )
{
    /*
    const EM::ObjectID objid = EM::EMM().multiID2ObjectID( mid );
    mDynamicCastGet(EM::Surface*,surface,EM::EMM().getObject(objid))
    if ( !surface ) return false;

    curemid = mid;
    cursectionid = surface->geometry.addSection( "", true );
    setObjectType( mid );
    startWizard( 1 );
    wizard->setSurfaceColor( surface->preferredColor() );

    int trackidx = getTrackerID( curemid );
    if ( trackidx < 0 )
	trackidx = trackman.addTracker( curemid );

    wizard->setTracker( trackman.getTracker(trackidx) );

    return true;
    */
}


bool uiMPEPartServer::isTrackingEnabled( int trackerid ) const
{ return MPE::engine().getTracker(trackerid)->isEnabled(); }


void uiMPEPartServer::enableTracking( int trackerid, bool yn )
{ return MPE::engine().getTracker(trackerid)->enable(yn); }


int uiMPEPartServer::activeTrackerID() const
{ return activetrackerid; }


const Attrib::SelSpec* uiMPEPartServer::getAttribSelSpec() const
{ return eventattrselspec; }



void uiMPEPartServer::showSetupDlg( const MultiID& mid, EM::SectionID sid )
{
    EM::ObjectID objid = EM::EMM().multiID2ObjectID( mid );
    uiDialog dlg( appserv().parent(), uiDialog::Setup("Tracking Setup") );
    dlg.setCtrlStyle( uiDialog::LeaveOnly );
    dlg.setHelpID( "108.0.1" );
    MPE::uiSetupSel* grp = new MPE::uiSetupSel( &dlg, attrset );
    grp->setType( objid, sid );
    if ( dlg.go() )
    {
	loadAttribData();
	sendEvent( evShowToolbar );
    }
}


void uiMPEPartServer::showRelationsDlg( const MultiID& mid, EM::SectionID sid )
{
    EM::ObjectID objid = EM::EMM().multiID2ObjectID( mid );
    bool allowhorsel = false;
    bool allowfltsel = true;
    uiSurfaceRelationDlg dlg( appserv().parent(), objid,
			      allowhorsel, allowfltsel );
    dlg.go();
}


void uiMPEPartServer::setAttribData( const Attrib::SelSpec& spec,
					Attrib::SliceSet* slcset )
{
    MPE::engine().setAttribData( spec, slcset );
}


void uiMPEPartServer::loadAttribData()
{
    uiCursorChanger changer( uiCursor::Wait );

    ObjectSet<const Attrib::SelSpec> attribselspecs;
    MPE::engine().getNeededAttribs(attribselspecs);
    for ( int idx=0; idx<attribselspecs.size(); idx++ )
    {
	eventattrselspec = attribselspecs[idx];
	sendEvent( evGetAttribData );
    }
    sendEvent( evShowToolbar );
}


const Attrib::SliceSet*
    uiMPEPartServer::getAttribCache( const Attrib::SelSpec& spec ) const
{ return MPE::engine().getAttribCache( spec ); }


CubeSampling uiMPEPartServer::getActiveVolume() const
{ return MPE::engine().activeVolume(); }


void uiMPEPartServer::expandActiveArea(const CubeSampling& seedcs)
{
    const CubeSampling activecs = MPE::engine().activeVolume();
    const bool isdefault = activecs==MPE::engine().getDefaultActiveVolume();

    CubeSampling newcube = isdefault ? seedcs : activecs;
    if ( !isdefault )
    {
	newcube.hrg.include( seedcs.hrg.start );
	newcube.hrg.include( seedcs.hrg.stop );
	newcube.zrg.include( seedcs.zrg.start );
	newcube.zrg.include( seedcs.zrg.stop );
    }

    const int minnr = 20;
    if ( newcube.nrInl() < minnr )
    {
	newcube.hrg.start.inl -= minnr*newcube.hrg.step.inl;
	newcube.hrg.stop.inl += minnr*newcube.hrg.step.inl;
    }

    if ( newcube.nrCrl() < minnr )
    {
	newcube.hrg.start.crl -= minnr*newcube.hrg.step.crl;
	newcube.hrg.stop.crl += minnr*newcube.hrg.step.crl;
    }

    if ( isdefault )
	newcube.zrg.widen( 0.05 );

    newcube.snapToSurvey();
    newcube.limitTo( SI().sampling(true) );
    MPE::engine().setActiveVolume( newcube );

    //sendEvent( evShowToolbar );
}


void uiMPEPartServer::setSeed( ObjectSet<Geometry::Element>& newseeds )
{
    deepErase( MPE::engine().interactionseeds );
    MPE::engine().interactionseeds = newseeds;
    newseeds.erase();
}


void uiMPEPartServer::fillPar( IOPar& par ) const
{
    MPE::engine().fillPar( par );
}


bool uiMPEPartServer::usePar( const IOPar& par )
{
    bool res = MPE::engine().usePar( par );
    if ( res )
    {
	if ( !sendEvent(evInitFromSession) )
	    return false;

	loadAttribData();
	sendEvent( evShowToolbar );
    }
    return res;
}
