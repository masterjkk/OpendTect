/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          25/05/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiioobjsel.cc,v 1.126 2009-01-07 06:32:39 cvsnageswara Exp $";

#include "uiioobjsel.h"

#include "ctxtioobj.h"
#include "errh.h"
#include "iodir.h"
#include "iodirentry.h"
#include "iolink.h"
#include "ioman.h"
#include "iopar.h"
#include "iostrm.h"
#include "linekey.h"
#include "transl.h"

#include "uigeninput.h"
#include "uiioobjmanip.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uistatusbar.h"


static const MultiID udfmid( "-1" );


static IOObj* mkEntry( const CtxtIOObj& ctio, const char* nm )
{
    CtxtIOObj newctio( ctio );
    newctio.ioobj = 0; newctio.setName( nm );
    newctio.fillObj();
    return newctio.ioobj;
}


class uiIOObjSelGrpManipSubj : public uiIOObjManipGroupSubj
{
public:

uiIOObjSelGrpManipSubj( uiIOObjSelGrp* sg )
    : uiIOObjManipGroupSubj(sg->listfld_)
    , selgrp_(sg)
    , manipgrp_(0)
{
    selgrp_->selectionChg.notify( mCB(this,uiIOObjSelGrpManipSubj,selChg) );
}

const MultiID* curID() const
{
    const int selidx = selgrp_->listfld_->currentItem();
    return selidx < 0 ? 0 : selgrp_->ioobjids_[selidx];
}

const char* defExt() const
{
    return selgrp_->ctio_.ctxt.trgroup->defExtension();
}

const BufferStringSet& names() const
{
    return selgrp_->ioobjnms_;
}

void chgsOccurred()
{
    selgrp_->fullUpdate( selgrp_->listfld_->currentItem() );
}

void selChg( CallBacker* )
{
    if ( manipgrp_ ) manipgrp_->selChg();
}

void relocStart( const char* msg )
{
    selgrp_->toStatusBar( msg );
}

    uiIOObjSelGrp*	selgrp_;
    uiIOObjManipGroup*	manipgrp_;

};



uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const CtxtIOObj& c,
			      const char* seltxt, bool multisel )
    : uiGroup(p)
    , ctio_(c)
    , ismultisel_(multisel && ctio_.ctxt.forread)
    , nmfld_(0)
    , manipgrpsubj(0)
    , newStatusMsg(this)
    , selectionChg(this)
    , confirmoverwrite_(true)
    , asked2overwrite_(false)
{
    IOM().to( ctio_.ctxt.getSelKey() );

    topgrp_ = new uiGroup( this, "Top group" );
    filtfld_ = new uiGenInput( topgrp_, "Filter", "*" );
    filtfld_->valuechanged.notify( mCB(this,uiIOObjSelGrp,filtChg) );
    if ( !seltxt || !*seltxt )
    {
	listfld_ = new uiListBox( topgrp_ );
	filtfld_->attach( centeredAbove, listfld_ );
	topgrp_->setHAlignObj( listfld_ );
    }
    else
    {
	uiLabeledListBox* llb = new uiLabeledListBox( topgrp_, seltxt );
	llb->attach( alignedBelow, filtfld_ );
	topgrp_->setHAlignObj( llb );
	listfld_ = llb->box();
    }

    listfld_->setName( "Objects list" );
    if ( ismultisel_ )
	listfld_->setMultiSelect( true );
    listfld_->setPrefHeightInChar( 8 );
    fullUpdate( 0 );

    if ( ctio_.ioobj )
        listfld_->setCurrentItem( ctio_.ioobj->name() );

    if ( !ctio_.ctxt.forread )
    {
	nmfld_ = new uiGenInput( this, "Name" );
	nmfld_->attach( alignedBelow, topgrp_ );
	nmfld_->setElemSzPol( uiObject::SmallMax );
	nmfld_->setStretch( 2, 0 );

	LineKey lk( ctio_.name() );
	const BufferString nm( lk.lineName() );
	if ( !nm.isEmpty() )
	{
	    nmfld_->setText( nm );
	    if ( listfld_->isPresent( nm ) )
		listfld_->setCurrentItem( nm );
	    else
		listfld_->clear();
	}
    }

    if ( !ismultisel_ && ctio_.ctxt.maydooper )
    {
	manipgrpsubj = new uiIOObjSelGrpManipSubj( this );
	manipgrpsubj->manipgrp_ = new uiIOObjManipGroup( *manipgrpsubj );
    }

    listfld_->setHSzPol( uiObject::Wide );
    listfld_->selectionChanged.notify( mCB(this,uiIOObjSelGrp,selChg) );
    listfld_->deleteButtonPressed.notify( mCB(this,uiIOObjSelGrp,delPress) );
    if ( (nmfld_ && !*nmfld_->text()) || !nmfld_ )
	selChg( this );
    setHAlignObj( topgrp_ );
    finaliseDone.notify( mCB(this,uiIOObjSelGrp,setInitial) );
}


uiIOObjSelGrp::~uiIOObjSelGrp()
{
    deepErase( ioobjids_ );
    if ( manipgrpsubj )
	delete manipgrpsubj->manipgrp_;
    delete manipgrpsubj;
}


void uiIOObjSelGrp::setInitial( CallBacker* )
{
    const char* presetnm = getNameField() ? getNameField()->text() : "";
    if ( *presetnm )
    {
	if ( !getListField()->isPresent( presetnm ) )
	    return;
	else
	    getListField()->setCurrentItem( presetnm );
    }

    selChg( 0 );
}


int uiIOObjSelGrp::nrSel() const
{
    int nr = 0;
    for ( int idx=0; idx<listfld_->size(); idx++ )
	if ( listfld_->isSelected(idx) ) nr++;

    return nr;
}


uiIOObjManipGroup* uiIOObjSelGrp::getManipGroup()
{
    return manipgrpsubj ? manipgrpsubj->grp_ : 0;
}


const MultiID& uiIOObjSelGrp::selected( int objnr ) const
{
    for ( int idx=0; idx<listfld_->size(); idx++ )
    {
	if ( listfld_->isSelected(idx) )
	    objnr--;
	if ( objnr < 0 )
	    return *ioobjids_[idx];
    }

    BufferString msg( "Should not reach. objnr=" );
    msg += objnr; msg += " nrsel="; msg += nrSel();
    pErrMsg( msg );
    return udfmid;
}


void uiIOObjSelGrp::delPress( CallBacker* )
{
    if ( manipgrpsubj && manipgrpsubj->manipgrp_ )
	manipgrpsubj->manipgrp_->triggerButton( uiManipButGrp::Remove );
}


void uiIOObjSelGrp::filtChg( CallBacker* )
{
    fullUpdate( 0 );
}


void uiIOObjSelGrp::fullUpdate( const MultiID& ky )
{
    int selidx = indexOf( ioobjids_, ky );
    fullUpdate( selidx );
    // Maybe a new one has been added
    if ( selidx < 0 )
    {
	selidx = indexOf( ioobjids_, ky );
	if ( selidx >= 0 )
	    setCur( selidx );
    }
}


void uiIOObjSelGrp::fullUpdate( int curidx )
{
    IOM().to( ctio_.ctxt.getSelKey() );
    IODirEntryList del( IOM().dirPtr(), ctio_.ctxt );
    BufferString nmflt = filtfld_->text();
    if ( !nmflt.isEmpty() && nmflt != "*" )
	del.fill( IOM().dirPtr(), nmflt );

    ioobjnms_.erase();
    deepErase( ioobjids_ );
    for ( int idx=0; idx<del.size(); idx++ )
    {
	BufferString nm = del[idx]->name();
	char* ptr = nm.buf();
	mSkipBlanks( ptr );
	ioobjnms_.add( ptr );
	ioobjids_ += new MultiID(
			del[idx]->ioobj ? del[idx]->ioobj->key() : udfmid );
    }

    fillListBox();
    setCur( curidx );
}


void uiIOObjSelGrp::setCur( int curidx )
{
    if ( curidx >= ioobjnms_.size() )
	curidx = ioobjnms_.size() - 1;
    else if ( curidx < 0 )
	curidx = 0;
    if ( ioobjnms_.size() )
	listfld_->setCurrentItem( curidx );
    selectionChg.trigger();
}


void uiIOObjSelGrp::fillListBox()
{
    listfld_->empty();
    listfld_->addItems( ioobjnms_ );
}


void uiIOObjSelGrp::toStatusBar( const char* txt )
{
    CBCapsule<const char*> caps( txt, this );
    newStatusMsg.trigger( &caps );
}


IOObj* uiIOObjSelGrp::getIOObj( int idx )
{
    bool issel = listfld_->isSelected( idx );
    if ( idx < 0 || !issel ) return 0;

    const MultiID& ky = *ioobjids_[idx];
    return IOM().get( ky );
}


void uiIOObjSelGrp::selChg( CallBacker* cb )
{
    BufferString info;
    if ( ismultisel_ && nrSel()>1 )
    {
	info += "Multiple objects selected (";
	info += nrSel(); info += "/"; info += listfld_->size(); info += ")";
    }
    else
    {
	PtrMan<IOObj> ioobj = getIOObj( listfld_->currentItem() );
	ctio_.setObj( ioobj ? ioobj->clone() : 0 );
	if ( cb && nmfld_ )
	    nmfld_->setText( ioobj ? ioobj->name() : "" );
	info = ioobj ? ioobj->fullUserExpr(ctio_.ctxt.forread) : "";
	const int len = info.size();
	if ( len>44 )
	{
	    BufferString tmp( info );
	    info = "....";
	    info += tmp.buf() + len - 40;
	}
    }

    toStatusBar( info );
    selectionChg.trigger();
}


void uiIOObjSelGrp::setContext( const IOObjContext& c )
{
    ctio_.ctxt = c; ctio_.setObj( 0 );
    fullUpdate( 0 );
}


bool uiIOObjSelGrp::processInput()
{
    int curitm = listfld_->currentItem();
    if ( !nmfld_ )
    {
	if ( ismultisel_ )
	{
	    if ( nrSel() > 0 )
		return true;
	    uiMSG().error( "Please select at least one item ",
		           "or press Cancel" );
	    return false;
	}

	PtrMan<IOObj> ioobj = getIOObj( listfld_->currentItem() );
	mDynamicCastGet(IOLink*,iol,ioobj.ptr())
	if ( !ioobj || (iol && ctio_.ctxt.maychdir) )
	{
	    IOM().to( iol );
	    fullUpdate( 0 );
	    return false;
	}
	ctio_.setObj( ioobj->clone() );
	return true;
    }

    LineKey lk( nmfld_->text() );
    const BufferString seltxt( lk.lineName() );
    int itmidx = ioobjnms_.indexOf( seltxt.buf() );
    if ( itmidx < 0 )
	return createEntry( seltxt );

    if ( itmidx != curitm )
	setCur( itmidx );
    IOObj* ioobj = getIOObj( itmidx );
    if ( ioobj && ioobj->implExists(true) )
    {
	bool ret = true;
	if ( ioobj->implReadOnly() )
	{
	    uiMSG().error( "Object is read-only" );
	    ret = false;
	}
	else if ( confirmoverwrite_ && !asked2overwrite_ )
	    ret = uiMSG().askGoOn( "Overwrite existing object?" );

	if ( !ret )
	{
	    asked2overwrite_ = false;
	    delete ioobj;
	    return false;
	}

	asked2overwrite_ = true;
    }

    ctio_.setObj( ioobj );
    return true;
}


bool uiIOObjSelGrp::createEntry( const char* seltxt )
{
    PtrMan<IOObj> ioobj = mkEntry( ctio_, seltxt );
    if ( !ioobj )
    {
	uiMSG().error( "Cannot create object with this name" );
	return false;
    }

    ioobjnms_.add( ioobj->name() );
    ioobjids_ += new MultiID( ioobj->key() );
    fillListBox();
    listfld_->setCurrentItem( ioobj->name() );
    if ( nmfld_ && ioobj->name() != seltxt )
	nmfld_->setText( ioobj->name() );

    ctio_.setObj( ioobj->clone() );
    selectionChg.trigger();
    return true;
}


bool uiIOObjSelGrp::fillPar( IOPar& iop ) const
{
    if ( !const_cast<uiIOObjSelGrp*>(this)->processInput() || !ctio_.ioobj )
	return false;
    iop.set( "ID", ctio_.ioobj->key() );

    return true;
}


void uiIOObjSelGrp::usePar( const IOPar& iop )
{
    const char* res = iop.find( "ID" );
    if ( !res || !*res ) return;
    const int selidx = indexOf( ioobjids_, MultiID(res) );
    if ( selidx >= 0 )
	setCur( selidx );
}


uiIOObjSelDlg::uiIOObjSelDlg( uiParent* p, const CtxtIOObj& c,
			      const char* seltxt, bool multisel )
	: uiIOObjRetDlg(p,
		Setup(c.ctxt.forread?"Input selection":"Output selection",
		    	mNoDlgTitle,"8.1.1")
		.nrstatusflds(1))
	, selgrp_( 0 )
{
    const bool ismultisel = multisel && c.ctxt.forread;
    selgrp_ = new uiIOObjSelGrp( this, c, 0, multisel );
    selgrp_->getListField()->setHSzPol( uiObject::Wide );
    statusBar()->setTxtAlign( 0, OD::AlignRight );
    selgrp_->newStatusMsg.notify( mCB(this,uiIOObjSelDlg,statusMsgCB));

    BufferString nm( seltxt );
    if ( nm.isEmpty() )
    {
	nm = "Select ";
	nm += c.ctxt.forread ? "input " : "output ";
	if ( c.ctxt.name().isEmpty() )
	    nm += c.ctxt.trgroup->userName();
	else
	    nm += c.ctxt.name();
	if ( ismultisel ) nm += "(s)";
    }
    setTitleText( nm );

    BufferString caption( seltxt );
    if ( caption.isEmpty() )
    {
	caption = c.ctxt.forread ? "Load " : "Save ";
	if ( c.ctxt.name().isEmpty() )
	    caption += c.ctxt.trgroup->userName();
	else
	    caption += c.ctxt.name();
	if ( !c.ctxt.forread ) caption += " as";
	else if ( ismultisel ) caption += "(s)";
    }
    setCaption( caption );

    setOkText( "&Ok (Select)" );
    selgrp_->getListField()->doubleClicked.notify(
	    mCB(this,uiDialog,accept) );
}


void uiIOObjSelDlg::statusMsgCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const char*,msg,cb);
    toStatusBar( msg );
}


uiIOObjSel::uiIOObjSel( uiParent* p, CtxtIOObj& c, const char* txt,
			bool wclr, const char* st, const char* buttxt, 
			bool keepmytxt )
    : uiIOSelect( p, mCB(this,uiIOObjSel,doObjSel),
		  txt ? txt :
		     (c.ctxt.name().isEmpty()
			 ? (const char*)c.ctxt.trgroup->userName() 
			 : (const char*) c.ctxt.name() ),
		  wclr, buttxt, keepmytxt )
    , ctio_(c)
    , forread_(c.ctxt.forread)
    , seltxt_(st)
    , confirmovw_(true)
{
    updateInput();
}


uiIOObjSel::~uiIOObjSel()
{
}


bool uiIOObjSel::fillPar( IOPar& iopar, const char* ck ) const
{
    iopar.set( IOPar::compKey(ck,"ID"),
	       ctio_.ioobj ? ctio_.ioobj->key() : MultiID("") );
    return true;
}


void uiIOObjSel::usePar( const IOPar& iopar, const char* ck )
{
    const char* res = iopar.find( IOPar::compKey(ck,"ID") );
    if ( res && *res )
    {
	ctio_.setObj( MultiID(res) );
	setInput( res );
    }
}


void uiIOObjSel::updateInput()
{
    setInput( ctio_.ioobj ? (const char*)ctio_.ioobj->key() : "" );
}


const char* uiIOObjSel::userNameFromKey( const char* ky ) const
{
    static BufferString nm;
    nm = "";
    if ( ky && *ky )
	nm = IOM().nameOf( ky, false );
    return (const char*)nm;
}


void uiIOObjSel::obtainIOObj()
{
    LineKey lk( getInput() );
    const BufferString inp( lk.lineName() );
    if ( specialitems.findKeyFor(inp) )
    {
	ctio_.setObj( 0 );
	return;
    }

    int selidx = getCurrentItem();
    if ( selidx >= 0 )
    {
	const char* itemusrnm = userNameFromKey( getItem(selidx) );
	if ( inp == itemusrnm && ctio_.ioobj 
			      && !strcmp(ctio_.ioobj->name(), inp.buf()) )
	    return;
    }

    IOM().to( ctio_.ctxt.getSelKey() );
    const IOObj* ioobj = (*IOM().dirPtr())[inp.buf()];
    ctio_.setObj( ioobj && ctio_.ctxt.validIOObj(*ioobj) ? ioobj->clone() : 0 );
}


void uiIOObjSel::processInput()
{
    obtainIOObj();
    if ( ctio_.ioobj || forread_ )
	updateInput();
}


bool uiIOObjSel::existingUsrName( const char* nm ) const
{
    IOM().to( ctio_.ctxt.getSelKey() );
    return (*IOM().dirPtr())[nm];
}


bool uiIOObjSel::commitInput( bool mknew )
{
    LineKey lk( getInput() );
    const BufferString inp( lk.lineName() );
    if ( specialitems.findKeyFor(inp) )
    {
	ctio_.setObj( 0 );
	return true;
    }
    if ( inp.isEmpty() )
	return false;

    processInput();
    if ( existingTyped() )
    {
	if ( ctio_.ioobj ) return true;
	BufferString msg( "'" ); msg += getInput();
	msg += "' already exists.\nPlease enter another name.";
	uiMSG().error( msg );
	return false;
    }
    if ( !mknew ) return false;

    ctio_.setObj( createEntry( getInput() ) );
    return ctio_.ioobj;
}


void uiIOObjSel::doObjSel( CallBacker* )
{
    ctio_.ctxt.forread = forread_;
    if ( !ctio_.ctxt.forread )
	ctio_.setName( getInput() );
    uiIOObjRetDlg* dlg = mkDlg();
    if ( !dlg ) return;
    uiIOObjSelGrp* selgrp_ = dlg->selGrp();
    if ( selgrp_ ) selgrp_->setConfirmOverwrite( confirmovw_ );

    if ( !helpid_.isEmpty() )
	dlg->setHelpID( helpid_ );
    if ( dlg->go() && dlg->ioObj() )
    {
	ctio_.setObj( dlg->ioObj()->clone() );
	updateInput();
	newSelection( dlg );
	selok_ = true;
    }

    delete dlg;
}


void uiIOObjSel::objSel()
{
    const char* key = getKey();
    if ( specialitems.find(key) )
	ctio_.setObj( 0 );
    else
	ctio_.setObj( IOM().get(getKey()) );
}


uiIOObjRetDlg* uiIOObjSel::mkDlg()
{
    return new uiIOObjSelDlg( this, ctio_, seltxt_ );
}


IOObj* uiIOObjSel::createEntry( const char* nm )
{
    return mkEntry( ctio_, nm );
}
