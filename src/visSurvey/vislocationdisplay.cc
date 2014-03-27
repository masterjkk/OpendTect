/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2002
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "vislocationdisplay.h"

#include "ioman.h"

#include "pickset.h"
#include "picksettr.h"
#include "selector.h"

#include "visevent.h"
#include "vismaterial.h"
#include "vissower.h"
#include "vistransform.h"
#include "zaxistransform.h"


namespace visSurvey {

const char* LocationDisplay::sKeyID()		{ return "Location.ID"; }
const char* LocationDisplay::sKeyMgrName()	{ return "Location.Manager"; }
const char* LocationDisplay::sKeyShowAll()	{ return "Show all"; }
const char* LocationDisplay::sKeyMarkerType()	{ return "Shape"; }
const char* LocationDisplay::sKeyMarkerSize()	{ return "Size"; }

static const float cDistEps = 0.1f;

static float findDistance( Coord3 p1, Coord3 p2, Coord3 p )
{
    const Coord3 vec = p2 - p1;
    const Coord3 newvec = p - p1;
    const float prod = (float) vec.dot(newvec);
    const float sq = (float) vec.sqAbs();
    if ( mIsZero(sq,cDistEps) ) return mUdf(float);	// p1 and p2 coincide.

    const float factor = prod / sq;
    if ( factor<0 || factor>1 )		// projected point outside the segment.
	return (float) mMIN( p1.distTo(p), p2.distTo(p) );

    const Coord3 proj = p1 + vec * factor;
    return (float) proj.distTo( p );
}


LocationDisplay::LocationDisplay()
    : VisualObjectImpl( true )
    , eventcatcher_( 0 )
    , transformation_( 0 )
    , showall_( true )
    , set_( 0 )
    , manip_( this )
    , picksetmgr_( 0 )
    , waitsfordirectionid_( -1 )
    , waitsforpositionid_( -1 )
    , datatransform_( 0 )
    , pickedsobjid_(-1)
    , voiidx_(-1)
{
    setSetMgr( &Pick::Mgr() );

    sower_ = new Sower( this );
    addChild( sower_->osgNode() );
}
    

LocationDisplay::~LocationDisplay()
{
    setSceneEventCatcher( 0 );

    if ( transformation_ ) transformation_->unRef();
    setSetMgr( 0 );

    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->remove(
		mCB( this, LocationDisplay, fullRedraw) );
	datatransform_->unRef();
    }

    removeChild( sower_->osgNode() );
    delete sower_;
}


void LocationDisplay::setSet( Pick::Set* s )
{
    if ( set_ )
    {
	if ( set_!=s )
	{
	    pErrMsg("Cannot set set_ twice");
	}
	return;
    }

    set_ = s; 
    setName( set_->name() );

    fullRedraw();

    if ( !showall_ && scene_ )
	scene_->objectMoved( 0 );
}


void LocationDisplay::setSetMgr( Pick::SetMgr* mgr )
{
    if ( picksetmgr_ )
	picksetmgr_->removeCBs( this );

    picksetmgr_ = mgr;

    if ( picksetmgr_ )
    {
	picksetmgr_->locationChanged.notify( mCB(this,LocationDisplay,locChg) );
	picksetmgr_->setChanged.notify( mCB(this,LocationDisplay,setChg) );
	picksetmgr_->setDispChanged.notify( mCB(this,LocationDisplay,dispChg) );
    }
}


void LocationDisplay::fullRedraw( CallBacker* )
{
    if ( !set_ ) return;
    

    if ( datatransform_ && datatransform_->needsVolumeOfInterest() )
    {
	CubeSampling cs( false );
	for ( int pidx=0; pidx<set_->size(); pidx++ )
	{
	    Pick::Location loc = (*set_)[pidx];
	    BinID bid = SI().transform( loc.pos_ );
	    const float zval = mCast( float, loc.pos_.z );
	    cs.hrg.include( bid );
	    cs.zrg.include( zval, false );
	}

	if ( set_->size() )
	{
	    if ( voiidx_<0 )
		voiidx_ = datatransform_->addVolumeOfInterest( cs, true );
	    else
		datatransform_->setVolumeOfInterest( voiidx_, cs, true );

	    datatransform_->loadDataIfMissing( voiidx_ );
	}
    }
    
    getMaterial()->setColor( set_->disp_.color_ );
   
    invalidpicks_.erase();
    
    const int nrpicks = set_->size();
    if ( !nrpicks )
    {
	removeAll();
	return;
    }
    int idx=0;
    for ( ; idx<nrpicks; idx++ )
    {
	Pick::Location loc = (*set_)[idx];
	if ( !transformPos( loc ) )
	{
	    invalidpicks_ += idx;
	}
	else
	{
	    invalidpicks_ -= idx;
	}

	setPosition( idx, loc );
    }
}


void LocationDisplay::removeAll()
{
    if ( !set_ )
	return;
}


void LocationDisplay::showAll( bool yn )
{
    showall_ = yn;
    if ( scene_ )
    {
	scene_->objectMoved(0);
	return;
    }
}


void LocationDisplay::pickCB( CallBacker* cb )
{
    if ( !isSelected() || !isOn() || isLocked() ) return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);

    const bool sowerenabled = set_->disp_.connect_ != Pick::Set::Disp::None;

    if ( waitsfordirectionid_!=-1 )
    {
	Coord3 newpos, normal;
	if ( getPickSurface(eventinfo,newpos,normal) )
	{
	    Coord3 dir = newpos - (*set_)[waitsfordirectionid_].pos_;
	    const float zscale = scene_ ? scene_->getZScale(): SI().zScale();
	    dir.z *= -zscale; //convert to right dir-domain
	    if ( dir.sqAbs()>=0 )
	    {
		 (*set_)[waitsfordirectionid_].dir_ =
		     cartesian2Spherical( dir, true );
		Pick::SetMgr::ChangeData cd(
			Pick::SetMgr::ChangeData::Changed,
			set_, waitsfordirectionid_ );
		picksetmgr_->reportChange( 0, cd );
	    }
	}

	eventcatcher_->setHandled();
    }
    else if ( waitsforpositionid_!=-1 )
    {
	Coord3 newpos, normal;
	if ( getPickSurface(eventinfo,newpos,normal) )
	{
	    (*set_)[waitsforpositionid_].pos_ = newpos;
	    Pick::SetMgr::ChangeData cd(
		    Pick::SetMgr::ChangeData::Changed,
		    set_, waitsforpositionid_ );
	    picksetmgr_->reportChange( 0, cd );
	}

	eventcatcher_->setHandled();
    }
    else if ( sowerenabled && sower_->accept(eventinfo) )
	return;

    if ( eventinfo.type != visBase::MouseClick ||
	 !OD::leftMouseButton( eventinfo.buttonstate_ ) )
	return;

    int eventid = -1;
    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	visBase::DataObject* dataobj =
	    		visBase::DM().getObject( eventinfo.pickedobjids[idx] );
	if ( dataobj == this )
	    continue;

	if ( dataobj->isPickable() )
	    eventid = eventinfo.pickedobjids[idx];

	mDynamicCastGet(const SurveyObject*,so,dataobj);
	if ( so && so->allowsPicks() )
	    pickedsobjid_ = eventid;

	if ( eventid!=-1 )
	    break;
    }

    if ( eventid==-1 )
	return;

    if ( waitsforpositionid_!=-1 || waitsfordirectionid_!=-1 )
    {
	setPickable( true );
	waitsforpositionid_ = -1;
	waitsfordirectionid_ = -1;
	mousepressid_ = -1;
    }
    else if ( eventinfo.pressed )
    {
	mousepressid_ = eventid;
	if ( !OD::ctrlKeyboardButton( eventinfo.buttonstate_ ) &&
	     !OD::altKeyboardButton( eventinfo.buttonstate_ ) &&
	     !OD::shiftKeyboardButton( eventinfo.buttonstate_ ) )
	{
	    const int selfpickidx = clickedMarkerIndex( eventinfo );
	    if ( selfpickidx!=-1 )
	    {
		setPickable( false );
		waitsforpositionid_ = selfpickidx;
	    }
	    const int selfdirpickidx = isDirMarkerClick(eventinfo.pickedobjids);
	    if ( selfdirpickidx!=-1 )
	    {
		setPickable( false );
		waitsfordirectionid_ = selfpickidx;
	    }

	    //Only set handled if clicked on marker. Otherwise
	    //we may interfere with draggers.
	    if ( selfdirpickidx!=-1 || selfpickidx!=-1 )
		eventcatcher_->setHandled();
	    else
	    {
		const Color& color = set_->disp_.color_;
		if ( sowerenabled && sower_->activate(color, eventinfo) )
		    return;
	    }
	}
    }
    else
    {
	if ( OD::ctrlKeyboardButton( eventinfo.buttonstate_ ) &&
	     !OD::altKeyboardButton( eventinfo.buttonstate_ ) &&
	     !OD::shiftKeyboardButton( eventinfo.buttonstate_ ) )
	{
	    if ( eventinfo.pickedobjids.size() &&
		 eventid==mousepressid_ )
	    {
		const int removeidx = clickedMarkerIndex( eventinfo );
		if ( removeidx!=-1 ) removePick( removeidx );
	    }

	    eventcatcher_->setHandled();
	}
	else if ( !OD::ctrlKeyboardButton( eventinfo.buttonstate_ ) &&
	          !OD::altKeyboardButton( eventinfo.buttonstate_ ) &&
		  !OD::shiftKeyboardButton( eventinfo.buttonstate_ ) )
	{
	    if ( eventinfo.pickedobjids.size() &&
		 eventid==mousepressid_ )
	    {
		Coord3 newpos, normal;
		if ( getPickSurface(eventinfo,newpos,normal) )
		{
		    const Sphere dir = normal.isDefined()
			? cartesian2Spherical(
				Coord3(normal.y,-normal.x,normal.z), true)
			: Sphere( 1, 0, 0 );

		    if ( addPick( newpos, dir, true ) )
		    {
			if ( hasDirection() )
			{
			    setPickable( false );
			    waitsfordirectionid_ = set_->size()-1;
			}

			eventcatcher_->setHandled();
		    }
		}
	    }
	}
    }
}


bool LocationDisplay::getPickSurface( const visBase::EventInfo& evi,
				      Coord3& newpos, Coord3& normal ) const
{
    const int sz = evi.pickedobjids.size();
    bool validpicksurface = false;
    int eventid = -1;

    for ( int idx=0; idx<sz; idx++ )
    {
	const DataObject* pickedobj =
	    visBase::DM().getObject( evi.pickedobjids[idx] );
	if ( pickedobj == this )
	    continue;

	if ( eventid==-1 && pickedobj->isPickable() )
	{
	    eventid = evi.pickedobjids[idx];
	    if ( validpicksurface )
		break;
	}

	mDynamicCastGet(const SurveyObject*,so,pickedobj);
	if ( so && so->allowsPicks() )
	{
	    validpicksurface = true;
	    normal = so->getNormal( evi.displaypickedpos );
	    if ( eventid!=-1 )
		break;
	}
    }

    if ( !validpicksurface )
	return false;

    newpos = evi.worldpickedpos;
    if ( datatransform_ )
    {
	newpos.z = datatransform_->transformBack( newpos );
	if ( mIsUdf(newpos.z) )
	    return false;
    }

    mDynamicCastGet( SurveyObject*,so, visBase::DM().getObject(eventid))
    if ( so ) so->snapToTracePos( newpos );

    return true;
}


Coord3 LocationDisplay::display2World( const Coord3& pos ) const
{
    Coord3 res = pos;
    if ( scene_ )
	scene_->getTempZStretchTransform()->transformBack( res );
    if ( transformation_ )
	transformation_->transformBack( res );
    return res;
}


Coord3 LocationDisplay::world2Display( const Coord3& pos ) const
{
    Coord3 res;
    mVisTrans::transform( transformation_, pos, res );
    if ( scene_ )
	scene_->getTempZStretchTransform()->transform( res );
    return res;
}


bool LocationDisplay::transformPos( Pick::Location& loc ) const
{
    if ( !datatransform_ ) return true;

    const float newdepth = datatransform_->transform( loc.pos_ );
    if ( mIsUdf(newdepth) )
	return false;

    loc.pos_.z = newdepth;

    if ( hasDirection() )
    {
	pErrMsg("Direction not impl");
    }

    return true;
}


void LocationDisplay::locChg( CallBacker* cb )
{
    mDynamicCastGet(Pick::SetMgr::ChangeData*,cd,cb)
    if ( !cd )
    {
	pErrMsg("Wrong pointer passed");
	return;
    }
    else if ( cd->set_ != set_ )
	return;

    if ( cd->ev_==Pick::SetMgr::ChangeData::Added )
    {
	Pick::Location loc = (*set_)[cd->loc_];
	if ( !transformPos( loc ) )
	{
	    invalidpicks_ += cd->loc_;
	}

	setPosition( cd->loc_,loc );
    }
    else if ( cd->ev_==Pick::SetMgr::ChangeData::ToBeRemoved )
    {
	removePosition( cd->loc_ );
	invalidpicks_ -= cd->loc_;
    }
    else if ( cd->ev_==Pick::SetMgr::ChangeData::Changed )
    {
	Pick::Location loc = (*set_)[cd->loc_];
	if ( !transformPos( loc ) )
	{
	    if ( invalidpicks_.indexOf(cd->loc_)==-1 )
		invalidpicks_ += cd->loc_;
	}
	else
	{
	    invalidpicks_ -= cd->loc_;
	}

	setPosition( cd->loc_, loc );
    }
}


void LocationDisplay::setChg( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb)
    if ( !ps )
    {
	pErrMsg("Wrong pointer passed");
       	return;
    }
    else if ( ps != set_ )
	return;

    manip_.trigger();
    fullRedraw();
}


void LocationDisplay::dispChg( CallBacker* )
{
    getMaterial()->setColor( set_->disp_.color_ );
}


void LocationDisplay::setColor( Color nc )
{
    if ( set_ )
    	set_->disp_.color_ = nc;
}


Color LocationDisplay::getColor() const
{
    if ( set_ )
    	return set_->disp_.color_;

    return Color::DgbColor();
}


bool LocationDisplay::isPicking() const
{
    return isSelected() && !isLocked();
}


bool LocationDisplay::addPick( const Coord3& pos, const Sphere& dir,
			       bool notif )
{
    mDefineStaticLocalObject( TypeSet<Coord3>, sowinghistory, );

    int locidx = -1;
    bool insertpick = false;
    if ( set_->disp_.connect_ == Pick::Set::Disp::Close )
    { 
	sower_->alternateSowingOrder( true );
	Coord3 displaypos = world2Display( pos );
	if ( sower_->mode() == Sower::FirstSowing )
	{
	    displaypos = sower_->pivotPos();
	    sowinghistory.erase();
	}

	float mindist = mUdf(float);
	for ( int idx=0; idx<set_->size(); idx++ )
	{
	    int pidx = idx>0 ? idx-1 : set_->size()-1;

	    int nrmatches = sowinghistory.indexOf( (*set_)[idx].pos_ ) >= 0;
	    nrmatches += sowinghistory.indexOf( (*set_)[pidx].pos_ ) >= 0;
	    if ( nrmatches != sowinghistory.size() )
		continue;

	    const float dist = findDistance( world2Display((*set_)[pidx].pos_),
		    			     world2Display((*set_)[idx].pos_),
					     displaypos );
	    if ( mIsUdf(dist) ) continue;

	    if ( mIsUdf(mindist) || dist<mindist )
	    {
		mindist = dist;
		locidx = idx;
	    }
	}
	insertpick = locidx >= 0;

	sowinghistory.insert( 0, pos );
	sowinghistory.removeSingle( 2 );
    }
    else
	sower_->alternateSowingOrder( false );

    if ( insertpick )
	set_->insert( locidx, Pick::Location(pos,dir) );
    else
    {
	*set_ += Pick::Location( pos, dir );
	locidx = set_->size()-1;
    }

    if ( notif && picksetmgr_ )
    {
	if ( picksetmgr_->indexOf(*set_)==-1 )
	    picksetmgr_->set( MultiID(), set_ );

	Pick::SetMgr::ChangeData cd( Pick::SetMgr::ChangeData::Added,
				     set_, locidx );
	picksetmgr_->reportChange( 0, cd );
    }

    if ( !hasText() ) return true;

    if ( !(*set_)[locidx].text_ || !(*set_)[locidx].text_->size() )
    {
	removePick( locidx );
	return false;
    }

    return true;
}


void LocationDisplay::removePick( int removeidx )
{
    if ( !picksetmgr_ )
	return;

    Pick::SetMgr::ChangeData cd( Pick::SetMgr::ChangeData::ToBeRemoved,
				 set_, removeidx );
    set_->removeSingle( removeidx );
    picksetmgr_->reportChange( 0, cd );
}


BufferString LocationDisplay::getManipulationString() const
{
    BufferString str = "PickSet: "; str += name();
    return str;
}


void LocationDisplay::getMousePosInfo( const visBase::EventInfo&,
				      Coord3& pos, BufferString& val,
				      BufferString& info ) const
{
    val = "";
    info = getManipulationString();
}


void LocationDisplay::otherObjectsMoved(
			const ObjectSet<const SurveyObject>& objs, int )
{
    if ( showall_ && invalidpicks_.isEmpty() ) return;

}


void LocationDisplay::setPosition(int idx, const Pick::Location& nl )
{
    if ( !set_ || idx<0 || idx>=(*set_).size() )
	return;
    
    (*set_)[idx] = nl;
}


void LocationDisplay::removePosition( int idx )
{
    if ( !set_ || idx<0 || idx>=(*set_).size() )
	return;
}


void LocationDisplay::setDisplayTransformation( const mVisTrans* newtr )
{
    if ( transformation_==newtr )
	return;

    if ( transformation_ )
	transformation_->unRef();

    transformation_ = newtr;

    if ( transformation_ )
	transformation_->ref();
    
    sower_->setDisplayTransformation( newtr );
}


const mVisTrans* LocationDisplay::getDisplayTransformation() const
{
    return transformation_;
}


void LocationDisplay::setRightHandSystem( bool yn )
{
    visBase::VisualObjectImpl::setRightHandSystem( yn );
}


void LocationDisplay::setSceneEventCatcher( visBase::EventCatcher* nevc )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(mCB(this,LocationDisplay,pickCB));
	eventcatcher_->unRef();
    }

    eventcatcher_ = nevc;
    sower_->setEventCatcher( nevc );

    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.notify(mCB(this,LocationDisplay,pickCB));
	eventcatcher_->ref();
    }

}


int LocationDisplay::getPickIdx( visBase::DataObject* dataobj ) const
{
    return 0; // to be implemented
}


bool LocationDisplay::setZAxisTransform( ZAxisTransform* zat, TaskRunner* tr )
{
    if ( datatransform_==zat )
	return true;

    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->remove(
		mCB( this, LocationDisplay, fullRedraw) );
	datatransform_->unRef();
    }

    datatransform_ = zat;

    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->notify(
		mCB( this, LocationDisplay, fullRedraw) );

	datatransform_->ref();
    }

    
    fullRedraw();
    showAll( datatransform_ && datatransform_->needsVolumeOfInterest() ); 
    return true;
}


const ZAxisTransform* LocationDisplay::getZAxisTransform() const
{
    return datatransform_;
}


int LocationDisplay::clickedMarkerIndex(const visBase::EventInfo& evi) const
{
    return -1;
}


bool LocationDisplay::isMarkerClick(const visBase::EventInfo& evi) const
{ 
    return false;
}


int LocationDisplay::isDirMarkerClick(const TypeSet<int>&) const
{ return -1; }


void LocationDisplay::triggerDeSel()
{
    setPickable( true );
    waitsfordirectionid_ = -1;
    waitsforpositionid_ = -1;
    VisualObject::triggerDeSel();
}


const SurveyObject* LocationDisplay::getPickedSurveyObject() const
{
    const DataObject* pickedobj = visBase::DM().getObject( pickedsobjid_ );
    mDynamicCastGet(const SurveyObject*,so,pickedobj);
    return so;
}


void LocationDisplay::removeSelection( const Selector<Coord3>& selector,
	TaskRunner* tr )
{
    if ( !selector.isOK() )
	return;

    for ( int idx=set_->size()-1; idx>=0; idx-- )
    {
	const Pick::Location& loc = (*set_)[idx];
	if ( selector.includes( loc.pos_ ) )
	    removePick( idx );
    }
}


void LocationDisplay::fillPar( IOPar& par ) const
{
    visBase::VisualObjectImpl::fillPar( par );
    visSurvey::SurveyObject::fillPar( par );

    const int setidx = picksetmgr_->indexOf( *set_ );
    par.set( sKeyID(), setidx>=0 ? picksetmgr_->get(*set_) : "" );
    par.set( sKeyMgrName(), picksetmgr_->name() );
    par.setYN( sKeyShowAll(), showall_ );
    par.set( sKeyMarkerType(), set_->disp_.markertype_ );
    par.set( sKeyMarkerSize(), set_->disp_.pixsize_ );

}


bool LocationDisplay::usePar( const IOPar& par )
{
    if ( !visBase::VisualObjectImpl::usePar( par ) ||
	 !visSurvey::SurveyObject::usePar( par ) )
	 return false;

    int markertype = 0;
    int pixsize = 3;
    par.get( sKeyMarkerType(), markertype );
    par.get( sKeyMarkerSize(), pixsize );

    bool shwallpicks = true;
    par.getYN( sKeyShowAll(), shwallpicks );
    showAll( shwallpicks );

    BufferString setmgr;
    if ( par.get(sKeyMgrName(),setmgr) )
	setSetMgr( &Pick::SetMgr::getMgr(setmgr.buf()) );

    if ( !par.get(sKeyID(),storedmid_) )
	return false;

    const int setidx = picksetmgr_ ? picksetmgr_->indexOf( storedmid_ ) : -1;
    if ( setidx==-1 )
    {
	mDeclareAndTryAlloc( Pick::Set*, newps, Pick::Set );

	BufferString bs;
	PtrMan<IOObj> ioobj = IOM().get( storedmid_ );
	if ( ioobj )
	    PickSetTranslator::retrieve( *newps, ioobj, true, bs );

	if ( !newps->name() || !*newps->name() )
	    newps->setName( name() );

	newps->disp_.markertype_ = markertype;
	newps->disp_.pixsize_ = pixsize;

	if ( picksetmgr_ ) picksetmgr_->set( storedmid_, newps );
	setSet( newps );
    }
    else
	setSet( &picksetmgr_->get( storedmid_ ) );

    return true;
}



}; // namespace visSurvey
