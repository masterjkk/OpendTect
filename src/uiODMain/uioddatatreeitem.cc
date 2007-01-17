/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		Jul 2003
 RCS:		$Id: uioddatatreeitem.cc,v 1.8 2007-01-17 10:33:06 cvshelene Exp $
___________________________________________________________________

-*/

#include "uioddatatreeitem.h"
#include "uioddisplaytreeitem.h"

#include "uimenu.h"
#include "uilistview.h"
#include "uiodapplmgr.h"
#include "uivispartserv.h"

#include "uimenuhandler.h"
#include "uiodscenemgr.h"

#include "viscolortab.h"
#include "colortab.h"
#include "pixmap.h"



TypeSet<uiODDataTreeItem::Creator> uiODDataTreeItem::creators_;


uiODDataTreeItem::uiODDataTreeItem( const char* parenttype )
    : uiTreeItem( "" )
    , parenttype_( parenttype )
    , menu_( 0 )
    , movemnuitem_( "Move ..." )
    , movetotopmnuitem_( "to top" )
    , movetobottommnuitem_( "to bottom" )
    , moveupmnuitem_( "up" )
    , movedownmnuitem_( "down" )
    , removemnuitem_("Remove" )
    , changetransparencyitem_( "Change transparency ..." )
    , addto2dvieweritem_( "Display on a 2D Viewer" )
{}


uiODDataTreeItem::~uiODDataTreeItem()
{
    if ( menu_ )
    {
	menu_->createnotifier.remove( mCB(this,uiODDataTreeItem,createMenuCB) );
	menu_->handlenotifier.remove( mCB(this,uiODDataTreeItem,handleMenuCB) );
	menu_->unRef();
    }
}


uiODDataTreeItem* uiODDataTreeItem::create( const Attrib::SelSpec& as,
					    const char* pt )
{
    for ( int idx=0; idx<creators_.size(); idx++)
    {
	uiODDataTreeItem* res = creators_[idx]( as, pt );
	if ( res )
	    return res;
    }

    return 0;
}


void uiODDataTreeItem::addFactory( uiODDataTreeItem::Creator cr )
{ creators_ += cr; }


int uiODDataTreeItem::uiListViewItemType() const
{
    uiVisPartServer* visserv = applMgr()->visServer();
    if ( visserv->canHaveMultipleAttribs( displayID() ) )
	return uiListViewItem::CheckBox;
    else
	return uiTreeItem::uiListViewItemType();
}


uiODApplMgr* uiODDataTreeItem::applMgr() const
{
    void* res = 0;
    getPropertyPtr( uiODTreeTop::applmgrstr, res );
    return reinterpret_cast<uiODApplMgr*>( res );
}


uiSoViewer* uiODDataTreeItem::viewer() const
{
    void* res = 0;
    getPropertyPtr( uiODTreeTop::viewerptr, res );
    return reinterpret_cast<uiSoViewer*>( res );
}


bool uiODDataTreeItem::init()
{
    uiVisPartServer* visserv = applMgr()->visServer();
    if ( visserv->canHaveMultipleAttribs( displayID() ) )
    {
	checkStatusChange()->notify( mCB( this, uiODDataTreeItem, checkCB ) );
	uilistviewitem_->setChecked( visserv->isAttribEnabled(displayID(),
		    		    attribNr() ) );
    }

    return uiTreeItem::init();
}


void uiODDataTreeItem::checkCB( CallBacker* cb )
{
    uiVisPartServer* visserv = applMgr()->visServer();
    visserv->enableAttrib( displayID(), attribNr(), isChecked() );
}


bool uiODDataTreeItem::shouldSelect( int selid ) const
{
    const uiVisPartServer* visserv = applMgr()->visServer();
    return selid!=-1 && selid==displayID() &&
	   visserv->getSelAttribNr()==attribNr();
}


int uiODDataTreeItem::sceneID() const
{
    int sceneid=-1;
    getProperty<int>( uiODTreeTop::sceneidkey, sceneid );
    return sceneid;
}



int uiODDataTreeItem::displayID() const
{
    mDynamicCastGet( uiODDisplayTreeItem*, odti, parent_ );
    return odti ? odti->displayID() : -1;
}


int uiODDataTreeItem::attribNr() const
{
    const uiVisPartServer* visserv = applMgr()->visServer();
    const int nrattribs = visserv->getNrAttribs( displayID() );
    return nrattribs-siblingIndex()-1;
}


bool uiODDataTreeItem::showSubMenu()
{
    if ( !menu_ )
    {
	menu_ = new uiMenuHandler( getUiParent(), -1 );
	menu_->ref();
	menu_->createnotifier.notify( mCB(this,uiODDataTreeItem,createMenuCB) );
	menu_->handlenotifier.notify( mCB(this,uiODDataTreeItem,handleMenuCB) );
    }

    return menu_->executeMenu(uiMenuHandler::fromTree);
}


void uiODDataTreeItem::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);

    uiVisPartServer* visserv = applMgr()->visServer();
    const bool isfirst = !siblingIndex();
    const bool islast = siblingIndex()==visserv->getNrAttribs( displayID())-1;

    const bool islocked = visserv->isLocked( displayID() );

    if ( !islocked && (!isfirst || !islast) )
    {
	mAddMenuItem( &movemnuitem_, &movetotopmnuitem_,
		      !islocked && !isfirst, false );
	mAddMenuItem( &movemnuitem_, &moveupmnuitem_,
		      !islocked && !isfirst, false );
	mAddMenuItem( &movemnuitem_, &movedownmnuitem_,
		      !islocked && !islast, false );
	mAddMenuItem( &movemnuitem_, &movetobottommnuitem_,
		      !islocked && !islast, false );

	mAddMenuItem( menu, &movemnuitem_, true, false );
    }
    else
    {
	mResetMenuItem( &changetransparencyitem_ );
	mResetMenuItem( &movetotopmnuitem_ );
	mResetMenuItem( &moveupmnuitem_ );
	mResetMenuItem( &movedownmnuitem_ );
	mResetMenuItem( &movetobottommnuitem_ );

	mResetMenuItem( &movemnuitem_ );
    }

    mAddMenuItem( menu, &removemnuitem_,
		  !islocked && visserv->getNrAttribs( displayID())>1, false );
    if ( visserv->canHaveMultipleAttribs(displayID()) && hasTransparencyMenu() )
	mAddMenuItem( menu, &changetransparencyitem_, true, false )
    else
	mResetMenuItem( &changetransparencyitem_ );

    if ( visserv->canBDispOn2DViewer(displayID()) )
	mAddMenuItem( menu, &addto2dvieweritem_, true, false )
    else
	mResetMenuItem( &addto2dvieweritem_ );
}


bool uiODDataTreeItem::select()
{
    applMgr()->visServer()->setSelObjectId( displayID(), attribNr() );
    return true;
}


void uiODDataTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    uiVisPartServer* visserv = applMgr()->visServer();

    if ( mnuid==movetotopmnuitem_.id )
    {
	const int nrattribs = visserv->getNrAttribs( displayID() );
	for ( int idx=attribNr(); idx<nrattribs-1; idx++ )
	    visserv->swapAttribs( displayID(), idx, idx+1 );

	moveItemToTop();

	menu->setIsHandled( true );
    }
    else if ( mnuid==movetobottommnuitem_.id )
    {
	const int nrattribs = visserv->getNrAttribs( displayID() );
	for ( int idx=attribNr(); idx; idx-- )
	    visserv->swapAttribs( displayID(), idx, idx-1 );

	while ( siblingIndex()<nrattribs-1 )
	    moveItem( siblingBelow() );

	menu->setIsHandled( true );
    }
    else if ( mnuid==moveupmnuitem_.id )
    {
	const int attribnr = attribNr();
	if ( attribnr<visserv->getNrAttribs( displayID() )-1 )
	{
	    const int targetattribnr = attribnr+1;
	    visserv->swapAttribs( displayID(), attribnr, targetattribnr );
	}

	moveItem( siblingAbove() );

	menu->setIsHandled(true);
    }
    else if ( mnuid==movedownmnuitem_.id )
    {
	const int attribnr = attribNr();
	if ( attribnr )
	{
	    const int targetattribnr = attribnr-1;
	    visserv->swapAttribs( displayID(), attribnr, targetattribnr );
	}

	moveItem( siblingBelow() );
	menu->setIsHandled( true );
    }
    else if ( mnuid==changetransparencyitem_.id )
    {
	menu->setIsHandled( true );
	visserv->showAttribTransparencyDlg( displayID(), attribNr() );
    }
    //TODO handle addto2dvieweritem_
    else if ( mnuid==removemnuitem_.id )
    {
	const int attribnr = attribNr();
	visserv->removeAttrib( displayID(), attribNr() );

	prepareForShutdown();
	parent_->removeChild( this );
	menu->setIsHandled( true );
    }
}


void uiODDataTreeItem::updateColumnText( int col )
{
    if ( applMgr()->visServer()->isLocked(displayID()) )
	return;

    if ( col==uiODSceneMgr::cNameColumn() )
	name_ = createDisplayName();

    uiTreeItem::updateColumnText( col );
}


void uiODDataTreeItem::displayMiniCtab( int ctabid )
{
    uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet( const visBase::VisColorTab*,coltab,
	    	     visserv->getObject( ctabid ) );
    if ( !coltab )
    {
	uiTreeItem::updateColumnText( uiODSceneMgr::cColorColumn() );
	return;
    }

    PtrMan<ioPixmap> pixmap = new ioPixmap( coltab->colorSeq().colors(),
	    				    cPixmapWidth(), cPixmapHeight() );
    uilistviewitem_->setPixmap( uiODSceneMgr::cColorColumn(), *pixmap );
}
