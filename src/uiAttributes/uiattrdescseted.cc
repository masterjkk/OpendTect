/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiattrdescseted.cc,v 1.43 2007-01-11 21:21:07 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrdescseted.h"
#include "uiattrdesced.h"
#include "uiattribfactory.h"
#include "uiattrtypesel.h"
#include "attribfactory.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetman.h"
#include "attribdescsettr.h"
#include "attribparam.h"
#include "attribstorprovider.h"
#include "uiattrinpdlg.h"
#include "uiattrgetfile.h"
#include "attribsel.h"

#include "ctxtioobj.h"
#include "datainpspec.h"
#include "dirlist.h"
#include "filegen.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "iostrm.h"
#include "iopar.h"
#include "iodir.h"
#include "oddirs.h"
#include "ptrman.h"
#include "keystrs.h"

#include "uigeninput.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uilineedit.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiselsimple.h"
#include "uiseparator.h"
#include "uitextedit.h"
#include "uitoolbar.h"
#include "seistype.h"
#include "survinfo.h"
#include "bufstringset.h"
#include "uifileinput.h"
#include "uiattribcrossplot.h"


extern "C" const char* GetBaseDataDir();
static bool prevsavestate = true;
static bool evaldlgpoppedup = false;

using namespace Attrib;

uiAttribDescSetEd::uiAttribDescSetEd( uiParent* p, DescSetMan* adsm )
    : uiDialog(p,uiDialog::Setup( adsm && adsm->is2D() ? "Attribute Set 2D"
					: "Attribute Set 3D","","101.0.0")
	.savebutton(true).savetext("Save on OK  ")
	.menubar(true)
	.modal(false))
    , inoutadsman(adsm)
    , userattrnames(*new BufferStringSet)
    , setctio(*mMkCtxtIOObj(AttribDescSet))
    , prevdesc(0)
    , attrset(0)
    , dirshowcb(this)
    , evalattrcb(this)
    , adsman(0)
{
    createMenuBar();
    createToolBar();
    createGroups();

    init();
}


#define mInsertItem( txt, func ) \
    filemnu->insertItem( new uiMenuItem(txt,mCB(this,uiAttribDescSetEd,func)) )

void uiAttribDescSetEd::createMenuBar()
{
    uiMenuBar* menu = menuBar();
    if( !menu )		{ pErrMsg("huh?"); return; }

    uiPopupMenu* filemnu = new uiPopupMenu( this, "&File" );
    mInsertItem( "&New set ...", newSet );
    mInsertItem( "&Open set ...", openSet );
    mInsertItem( "&Save set ...", savePush );
    mInsertItem( "&Change input ...", changeInput );
    filemnu->insertSeparator();
    mInsertItem( "Open &Default set ...", defaultSet );
    mInsertItem( "&Import set ...", importSet );
    mInsertItem( "Import set from &file ...", importFile );
    mInsertItem( "&Reconstruct set from job file ...", job2Set );

    menu->insertItem( filemnu );
}


#define mAddButton(pm,func,tip) \
    toolbar->addButton( pm, mCB(this,uiAttribDescSetEd,func), tip )

void uiAttribDescSetEd::createToolBar()
{
    toolbar = new uiToolBar( this, "AttributeSet tools" );
    mAddButton( "newset.png", newSet, "New attribute set" );
    mAddButton( "openset.png", openSet, "Open attribute set" );
    mAddButton( "defset.png", defaultSet, "Open default attribute set" );
    mAddButton( "impset.png", importSet, 
	    	"Import attribute set from other survey" );
    mAddButton( "job2set.png", job2Set, "Reconstruct set from job file" );
    mAddButton( "saveset.png", savePush, "Save attribute set" );
    toolbar->addSeparator();
    mAddButton( "showattrnow.png", directShow, 
	    	"Redisplay element with current attribute");
    mAddButton( "evalattr.png", evalAttribute, "Evaluate attribute" );
    mAddButton( "xplot.png", crossPlot, "Cross-Plot attributes" );
    toolbar->display();
}


void uiAttribDescSetEd::createGroups()
{
//  Left part
    uiGroup* leftgrp = new uiGroup( this, "LeftGroup" );
    attrsetfld = new uiGenInput( leftgrp, "Attribute set", StringInpSpec() );
    attrsetfld->setReadOnly( true );

    attrlistfld = new uiListBox( leftgrp, "Defined Attributes" );
    attrlistfld->attach( ensureBelow, attrsetfld );
    attrlistfld->setStretch( 2, 2 );
    attrlistfld->selectionChanged.notify( mCB(this,uiAttribDescSetEd,selChg) );

    rmbut = new uiPushButton( leftgrp, "Remove selected", true );
    rmbut->attach( centeredBelow, attrlistfld );
    rmbut->activated.notify( mCB(this,uiAttribDescSetEd,rmPush) );

    uiSeparator* vertsep = new uiSeparator( this, "Big vert sep", false );
    vertsep->attach( rightOf, leftgrp );
    vertsep->attach( heightSameAs, leftgrp );

//  Right part
    uiGroup* rightgrp = new uiGroup( this, "RightGroup" );
    rightgrp->setStretch( 0, 1 );
    rightgrp->attach( rightOf, vertsep );

    uiGroup* degrp = new uiGroup( rightgrp, "DescEdGroup" );
    degrp->setStretch( 1, 1 );

    attrtypefld = new uiAttrTypeSel( degrp );
    attrtypefld->selChg.notify( mCB(this,uiAttribDescSetEd,attrTypSel) );
    desceds.allowNull();
    for ( int idx=0; idx<uiAF().size(); idx++ )
    {
	uiAttrDescEd::DomainType dt =
	    	(uiAttrDescEd::DomainType)uiAF().domainType( idx );
	if ( dt == uiAttrDescEd::Depth && SI().zIsTime()
		|| dt == uiAttrDescEd::Time && !SI().zIsTime() )
	    continue;

	const char* attrnm = uiAF().getDisplayName(idx);
	attrtypefld->add( uiAF().getGroupName(idx), attrnm );
	const bool is2d = inoutadsman ? inoutadsman->is2D() : false;
	uiAttrDescEd* de = uiAF().create( degrp, attrnm, is2d, true );
	desceds += de;
	de->attach( alignedBelow, attrtypefld );
    }
    attrtypefld->update();
    degrp->setHAlignObj( attrtypefld );

    attrnmfld = new uiLineEdit( rightgrp );
    attrnmfld->setHSzPol( uiObject::Medium );
    attrnmfld->attach( alignedBelow, degrp );
    attrnmfld->returnPressed.notify( mCB(this,uiAttribDescSetEd,addPush) );
    uiLabel* lbl = new uiLabel( rightgrp, "Attribute name" );
    lbl->attach( leftOf, attrnmfld );

    addbut = new uiPushButton( rightgrp, "Add as new", true );
    addbut->attach( alignedBelow, attrnmfld );
    addbut->activated.notify( mCB(this,uiAttribDescSetEd,addPush) );

    revbut = new uiPushButton( rightgrp, "Revert changes", true );
    revbut->attach( rightTo, addbut );
    revbut->attach( rightBorder );
    revbut->activated.notify( mCB(this,uiAttribDescSetEd,revPush) );
}


void uiAttribDescSetEd::init()
{
    delete attrset;
    attrset = inoutadsman->descSet()->clone();
    adsman = new DescSetMan( inoutadsman->is2D(), attrset );
    adsman->fillHist();
    adsman->setSaved( inoutadsman->isSaved() );

    setid = inoutadsman->attrsetid_;
    IOM().to( setctio.ctxt.getSelKey() );
    setctio.ioobj = IOM().get( setid );
    attrsetfld->setText( setctio.ioobj ? setctio.ioobj->name() : "" );
    cancelsetid = setid;
    newList(0);

    setSaveButtonChecked( prevsavestate );
}


uiAttribDescSetEd::~uiAttribDescSetEd()
{
    delete &userattrnames;
    delete &setctio;
    delete toolbar;
    delete adsman;
}



#define mErrRetFalse(s) { uiMSG().error( s ); return false; }
#define mErrRet(s) { uiMSG().error( s ); return; }


void uiAttribDescSetEd::attrTypSel( CallBacker* )
{
    updateFields( false );
}


void uiAttribDescSetEd::selChg( CallBacker* )
{
    doCommit( true );
    updateFields();
    prevdesc = curDesc();
}


void uiAttribDescSetEd::savePush( CallBacker* )
{
    removeNotUsedAttr();
    doSave( false );
}


bool uiAttribDescSetEd::doSave( bool endsave )
{
    doCommit();
    setctio.ctxt.forread = false;
    IOObj* oldioobj = setctio.ioobj;
    bool needpopup = !oldioobj || !endsave;
    if ( needpopup )
    {
	uiIOObjSelDlg dlg( this, setctio );
	if ( !dlg.go() || !dlg.ioObj() ) return false;

	setctio.ioobj = 0;
	setctio.setObj( dlg.ioObj()->clone() );
    }

    if ( !doSetIO( false ) )
    {
	if ( oldioobj != setctio.ioobj )
	    setctio.setObj( oldioobj );
	return false;
    }

    if ( oldioobj != setctio.ioobj )
	delete oldioobj;
    setid = setctio.ioobj->key();
    if ( !endsave )
	attrsetfld->setText( setctio.ioobj->name() );
    adsman->setSaved( true );
    return true;
}


void uiAttribDescSetEd::revPush( CallBacker* )
{
    updateFields();
}


void uiAttribDescSetEd::addPush( CallBacker* )
{
    BufferString newnmbs( attrnmfld->text() );
    char* newnm = newnmbs.buf();
    skipLeadingBlanks( newnm );
    removeTrailingBlanks( newnm );
    if ( !validName(newnm) ) return;

    uiAttrDescEd* curde = 0;
    if ( !(curde = curDescEd()) )
	mErrRet( "Cannot add without a valid attribute type" )

    BufferString attribname = curde->attribName();
    if ( attribname.isEmpty() )
	attribname = uiAF().attrNameOf( attrtypefld->attr() );

    Desc* newdesc = PF().createDescCopy( attribname );
    if ( !newdesc )
	mErrRet( "Cannot create attribdesc" )

    newdesc->setDescSet( attrset );
    newdesc->ref();
    const char* res = curde->commit( newdesc );
    if ( res )
	mErrRet( res )
    newdesc->setUserRef( newnm );

    if ( attrset->addDesc(newdesc) < 0 )
	{ uiMSG().error( attrset->errMsg() ); newdesc->unRef(); return; }

    newList( attrdescs.size() );
    adsman->setSaved( false );
}


void uiAttribDescSetEd::rmPush( CallBacker* )
{
    Desc* curdesc = curDesc();
    if ( !curdesc ) return;

    if ( attrset->isAttribUsed( curdesc->id() ) )
    {
	uiMSG().error( "Cannot remove this attribute. It is used\n"
		       "as input for another attribute" );
	return;
    }

    const int curidx = attrdescs.indexOf( curdesc );
    attrset->removeDesc( attrset->getID(*curdesc) );
    newList( curidx );
    removeNotUsedAttr();
    adsman->setSaved( false );
}


void uiAttribDescSetEd::handleSensitivity()
{
    bool havedescs = !attrdescs.isEmpty();
    rmbut->setSensitive( havedescs );
    revbut->setSensitive( havedescs );
}


bool uiAttribDescSetEd::acceptOK( CallBacker* )
{
    if ( !doCommit() )
	return false;
    removeNotUsedAttr();
    if ( saveButtonChecked() && !doSave(true) )
	return false;
    
    if ( inoutadsman )
        inoutadsman->setSaved( adsman->isSaved() );

    prevsavestate = saveButtonChecked();
    return true;
}


bool uiAttribDescSetEd::rejectOK( CallBacker* )
{
    setid = cancelsetid;
    return true;
}


void uiAttribDescSetEd::newList( int newcur )
{
    prevdesc = 0;
    updateUserRefs();
    attrlistfld->empty();
    attrlistfld->addItems( userattrnames );
    if ( newcur < 0 ) newcur = 0;
    if ( !userattrnames.isEmpty() )
    {
	attrlistfld->setCurrentItem( newcur );
	prevdesc = curDesc();
    }
    updateFields();
    handleSensitivity();
}


void uiAttribDescSetEd::updateFields( bool set_type )
{
    updateAttrName();
    Desc* desc = curDesc();
    uiAttrDescEd* curde = 0;
    if ( set_type )
    {
	BufferString typenm = desc ? desc->attribName() : "RefTime";
	attrtypefld->setAttr( uiAF().dispNameOf(typenm) );
	curde = curDescEd();
    }
    if ( !curde )
	curde = curDescEd();

    BufferString attribname = curde ? curde->attribName() : "";
    if ( attribname.isEmpty() )
	attribname = uiAF().attrNameOf( attrtypefld->attr() );
    const bool isrightdesc = desc && attribname == desc->attribName();

    Desc* dummydesc = PF().createDescCopy( attribname );
    if ( !dummydesc )
	dummydesc = new Desc( "Dummy" );
    
    dummydesc->ref();
    dummydesc->setDescSet( attrset );
    const bool is2d = adsman ? adsman->is2D() : attrset->is2D();
    for ( int idx=0; idx<desceds.size(); idx++ )
    {
	uiAttrDescEd* de = desceds[idx];
	if ( !de ) continue;

	if ( curde == de )
	{
	    if ( !isrightdesc )
		dummydesc->ref();
	    de->setDesc( isrightdesc ? desc : dummydesc, adsman );
	    de->setDescSet( attrset );
	}
	const bool dodisp = de == curde;
	de->display( dodisp );
    }
    dummydesc->unRef();
}


bool uiAttribDescSetEd::doCommit( bool useprev )
{
    Desc* usedesc = useprev ? prevdesc : curDesc();
    if ( !usedesc )
	return false;
    
    BufferString newattr = uiAF().attrNameOf( attrtypefld->attr() );
    BufferString oldattr = usedesc->attribName();
    if ( oldattr != newattr )
    {
	uiMSG().error( "Attribute type cannot be changed.\n"
		       "Please 'Add as new' first." );
	updateFields();
	return false;
    }

    if ( !setUserRef(usedesc) )
	return false;

    uiAttrDescEd* curdesced = curDescEd();
    if ( !curdesced )
	return false;

    const char* res = curdesced->commit( 0 );
    if ( res )
	mErrRetFalse( res )

    return true;
}


void uiAttribDescSetEd::updateUserRefs()
{
    BufferString selnm;
    if ( attrlistfld )
	selnm = attrlistfld->getText();
    userattrnames.deepErase();
    attrdescs.erase();

    for ( int iattr=0; iattr<attrset->nrDescs(); iattr++ )
    {
	const DescID descid = attrset->getID( iattr );
	Desc* desc = attrset->getDesc( descid );
	if ( !desc || desc->isHidden() || desc->isStored() ) continue;

	attrdescs += desc;
	userattrnames.add( desc->userRef() );
    }

    int newselidx = userattrnames.indexOf( selnm );
    if ( newselidx < 0 ) newselidx = 0;
}


Desc* uiAttribDescSetEd::curDesc() const
{
    int selidx = attrlistfld->currentItem();
    uiAttribDescSetEd* nct = const_cast<uiAttribDescSetEd*>( this );
    return selidx < 0 ? 0 : nct->attrdescs[selidx];
}


uiAttrDescEd* uiAttribDescSetEd::curDescEd()
{
    const char* attrstr = attrtypefld->attr();
    BufferString attrnm = uiAF().attrNameOf( attrstr );
    if ( !attrnm ) attrnm = attrstr;

    uiAttrDescEd* ret = 0;
    for ( int idx=0; idx<desceds.size(); idx++ )
    {
	uiAttrDescEd* de = desceds[idx];
	if ( !de ) continue;

	if ( attrnm == de->attribName() )
	    return de;
    }
    return ret;
}


void uiAttribDescSetEd::updateCurDescEd()
{
    curDescEd()->setDesc( curDesc(), adsman );
}


bool uiAttribDescSetEd::validName( const char* newnm ) const
{
    if ( !isalnum(newnm[0]) )
	mErrRetFalse( "Please start attribute name with a letter or number" );

    if ( strchr(newnm,'!') || strchr(newnm,':') || strchr(newnm,';') ||
	 strchr(newnm,'#') )
	mErrRetFalse( "Attribute name may not contain '!', '#', ';' or ':'." );

    if ( strlen(newnm) < 2 )
	mErrRetFalse( "Please enter a name of at least 2 characters." );

    TypeSet<DescID> ids;
    attrset->getIds( ids );
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	const Desc& ad = *attrset->getDesc( ids[idx] );
	if ( !strcmp(ad.userRef(),newnm) )
	{
	    uiMSG().error( "The name you entered for the attribute already"
			  " exists.\nPlease choose another name." );
	    return false;
	}
    }

    return true;
}


bool uiAttribDescSetEd::setUserRef( Desc* attrdesc )
{
    BufferString newnm( attrnmfld->text() );

    while ( newnm[(int)(strlen(newnm)-1)] == ' ' )
	newnm[(int)(strlen(newnm)-1)] = 0;

    if ( !strcmp(newnm,attrdesc->userRef()) ) return true;
    else if ( !validName(newnm) ) return false;

    const char* res = curDescEd()->commit();
    if ( res ) uiMSG().error( res );

    attrdesc->setUserRef( newnm );
    int selidx = userattrnames.indexOf( attrlistfld->getText() );
    newList( selidx );
    return true;
}


void uiAttribDescSetEd::updateAttrName()
{
    Desc* curdesc = curDesc();
    attrnmfld->setText( curdesc ? curdesc->userRef() : "" );
}


bool uiAttribDescSetEd::doSetIO( bool forread )
{
    if ( !setctio.ioobj )
    {
	if ( setid.isEmpty() ) return false;

	setctio.ioobj = IOM().get( setid );
	if ( !setctio.ioobj ) 
	    mErrRetFalse("Cannot find attribute set in data base")
    }

    BufferString bs;
    if ( forread && 
	 !AttribDescSetTranslator::retrieve(*attrset,setctio.ioobj,bs)
    ||  !forread && 
	!AttribDescSetTranslator::store(*attrset,setctio.ioobj,bs) )
	mErrRetFalse(bs)
    if ( !bs.isEmpty() )
	pErrMsg( bs );

    if ( forread )
    {
	adsman->setDescSet( attrset );
	adsman->fillHist();
    }	
    setid = setctio.ioobj->key();
    return true;
}


void uiAttribDescSetEd::newSet( CallBacker* )
{
    if ( !offerSetSave() ) return;
    adsman->inputHistory().clear();
    updateFields();

    attrset->removeAll();
    setctio.ioobj = 0;
    updateUserRefs();
    newList( -1 );
    attrsetfld->setText( "" );
    adsman->setSaved( true );
}


void uiAttribDescSetEd::openSet( CallBacker* )
{
    if ( !offerSetSave() ) return;
    setctio.ctxt.forread = true;
    uiIOObjSelDlg dlg( this, setctio );
    if ( dlg.go() && dlg.ioObj() )
    {
	IOObj* oldioobj = setctio.ioobj; setctio.ioobj = 0;
        setctio.setObj( dlg.ioObj()->clone() );
	if ( !doSetIO( true ) )
	    setctio.setObj( oldioobj );
	else
	{
	    delete oldioobj;
	    setid = setctio.ioobj->key();
	    newList( -1 );
	    attrsetfld->setText( setctio.ioobj->name() );
	    adsman->setSaved( true );
	    TypeSet<DescID> ids;
	    attrset->getIds( ids );
	    for ( int idx=0; idx<attrset->nrDescs(); idx++ )
	    {
		Desc* ad = attrset->getDesc( ids[idx] );
		if ( !ad ) continue;
		if ( ad->isStored() && ad->isSatisfied()==2 )
		{
		    BufferString msg = "The attribute: '";
		    msg += ad->userRef();
		    msg += "' will be removed\n";
		    msg += "Storage ID is no longer valid";
		    uiMSG().message( msg );
		    attrset->removeDesc( ad->id() );
		    idx--;
		}
	    }
	}
    }
}


void uiAttribDescSetEd::defaultSet( CallBacker* )
{
    if ( !offerSetSave() ) return;

    const char* ptr = GetDataFileName(0);
    DirList attrdl( ptr, DirList::DirsOnly, "*Attribs" );

    BufferStringSet attribfiles;
    BufferStringSet attribnames;
    for ( int idx=0; idx<attrdl.size(); idx++ )
    {
	FilePath fp( ptr );
	fp.add( attrdl[idx]->buf() ).add( "index" );
	IOPar iopar("AttributeSet Table");
	iopar.read( fp.fullPath(), sKey::Pars, false );
	for ( int idy=0; idy<iopar.size(); idy++ )
	{
	    BufferString attrfnm = iopar.getKey(idy);
	    if ( !FilePath(attrfnm).isAbsolute() )
	    {
		fp.setFileName(attrfnm);
		attrfnm = fp.fullPath();
	    }

	    if ( !File_exists(attrfnm) ) continue;
		
	    attribnames.add( iopar.getValue(idy) );
	    attribfiles.add( attrfnm );
	}
    }

    uiSelectFromList::Setup setup( "Default Attribute Sets", attribnames );
    setup.dlgtitle( "Select attribute set" );
    uiSelectFromList dlg( this, setup );
    if ( !dlg.go() ) return;

    const int selitm = dlg.selfld_->currentItem();
    const char* filenm = attribfiles[selitm]->buf();

    importFromFile( filenm );
}


void uiAttribDescSetEd::importFromFile( const char* filenm )
{
    IOStream* iostrm = new IOStream( "tmp" );
    iostrm->setGroup( setctio.ctxt.trgroup->userName() );
    iostrm->setTranslator( "dGB" );
    iostrm->setFileName( filenm );
    IOObj* oldioobj = setctio.ioobj ? setctio.ioobj->clone() : 0;
    setctio.setObj( iostrm );
    if ( !doSetIO(true) )
	setctio.setObj( oldioobj );
    else
    {
	delete oldioobj;
	setid = setctio.ioobj->key();
	replaceStoredAttr();
	newList( -1 );
	attrsetfld->setText( "" );
	setctio.ioobj = 0;
    }
}


void uiAttribDescSetEd::importSet( CallBacker* )
{
    if ( !offerSetSave() ) return;

    const char* ptr = GetBaseDataDir();
    if ( !ptr ) return;

    PtrMan<DirList> dirlist = new DirList( ptr, DirList::DirsOnly );
    dirlist->sort();

    uiSelectFromList::Setup setup( "Survey", *dirlist );
    setup.dlgtitle( "Select survey" );
    uiSelectFromList dlg( this, setup );
    if ( dlg.go() )
    {
	FilePath fp( ptr ); fp.add( dlg.selfld_->getText() );
	IOM().setRootDir( fp.fullPath() );
	setctio.ctxt.forread = true;
        uiIOObjSelDlg objdlg( this, setctio );
	if ( objdlg.go() && objdlg.ioObj() )
	{
	    IOObj* oldioobj = setctio.ioobj; setctio.ioobj = 0;
	    setctio.setObj( objdlg.ioObj()->clone() );
	    if ( !doSetIO( true ) )
		setctio.setObj( oldioobj );
	    else
	    {
		delete oldioobj;
		setid = setctio.ioobj->key();
		replaceStoredAttr();
		newList( -1 );
		attrsetfld->setText( "" );
		setctio.ioobj = 0;
	    }
	}
    }
    
    IOM().setRootDir( GetDataDir() );
}


void uiAttribDescSetEd::importFile( CallBacker* )
{
    if ( !offerSetSave() ) return;

    uiGetFileForAttrSet dlg( this, true, inoutadsman->is2D() );
    if ( dlg.go() )
	importFromFile( dlg.fileName() );
}


void uiAttribDescSetEd::job2Set( CallBacker* )
{
    if ( !offerSetSave() ) return;
    uiGetFileForAttrSet dlg( this, false, inoutadsman->is2D() );
    if ( dlg.go() )
    {
	if ( dlg.attrSet().nrDescs(false,false) < 1 )
	    mErrRet( "No usable attributes in file" )

	IOPar iop; dlg.attrSet().fillPar( iop );
	attrset->removeAll();
	attrset->usePar( iop );
	adsman->setSaved( false );

	setctio.setObj( 0 );
	newList( -1 ); attrsetfld->setText( "" );
    }
}


void uiAttribDescSetEd::crossPlot( CallBacker* )
{
    if ( !adsman || !adsman->descSet() ) return;

    uiAttribCrossPlot dlg( this, *adsman->descSet() );
    dlg.go();
}


void uiAttribDescSetEd::directShow( CallBacker* )
{
    if ( !curDesc() )
	mErrRet( "Please add this attribute first" )

    if ( doCommit() )
	dirshowcb.trigger();
    updateFields();
}


void uiAttribDescSetEd::evalAttribute( CallBacker* )
{
    if ( !doCommit() ) return;
    evalattrcb.trigger();
}


bool uiAttribDescSetEd::offerSetSave()
{
    doCommit( true );
    bool saved = adsman->isSaved();
    BufferString msg( "Attribute set is not saved.\nSave now?" );
    if ( !saved && uiMSG().askGoOn( msg ) )
	return doSave(false);
    return true;
}


void uiAttribDescSetEd::changeInput( CallBacker* )
{
    attrlistfld->empty();
    updateFields();
    replaceStoredAttr();
    newList(-1);
    adsman->setSaved( false );
}


bool uiAttribDescSetEd::hasInput( const Desc& desc, const DescID& id )
{
    for ( int idx=0; idx<desc.nrInputs(); idx++ )
    {
	const Desc* inp = desc.getInput( idx );
	if ( !inp ) return false;

	if ( inp->id() == id ) return true;
    }

    for ( int idx=0; idx<desc.nrInputs(); idx++ )
    {
	const Desc* inp = desc.getInput( idx );
	if ( inp->isHidden() && inp->nrInputs() )
	    return hasInput( *inp, id );
    }

    return false;
}


void uiAttribDescSetEd::replaceStoredAttr()
{
    TypeSet<DescID> storedids;
    TypeSet<LineKey> linekeys;
    for ( int idx=0; idx<attrset->nrDescs(); idx++ )
    {
	const DescID descid = attrset->getID( idx );
        Desc* ad = attrset->getDesc( descid );
        if ( !ad || !ad->isStored() ) continue;

	const ValParam* keypar = ad->getValParam( StorageProvider::keyStr() );
	LineKey lk( keypar->getStringValue() );
	if ( !linekeys.addIfNew(lk) ) continue;

	storedids += descid;
    }

    BufferStringSet usrrefs;
    bool found2d = false;
    for ( int idnr=0; idnr<storedids.size(); idnr++ )
    {
	usrrefs.erase();
	const DescID storedid = storedids[idnr];

	for ( int idx=0; idx<attrset->nrDescs(); idx++ )
	{
	    const DescID descid = attrset->getID( idx );
	    Desc* ad = attrset->getDesc( descid );
            if ( !ad || ad->isStored() || ad->isHidden() ) continue;

	    if ( hasInput(*ad,storedid) )
		usrrefs.addIfNew( ad->userRef() );
        }

	if ( usrrefs.isEmpty() )
	    continue;

	Desc* ad = attrset->getDesc( storedid );
	const bool issteer = ad->dataType() == Seis::Dip;
        uiAttrInpDlg dlg( this, usrrefs, issteer );
        if ( dlg.go() )
        {
            ad->changeStoredID( dlg.getKey() );
            ad->setUserRef( dlg.getUserRef() );
	    if ( issteer )
	    {
		Desc* adcrld = 
		    	attrset->getDesc( DescID(storedid.asInt()+1, true) );
		if ( ad && ad->isStored() )
		{
		    adcrld->changeStoredID( dlg.getKey() );
		    BufferString bfstr = dlg.getUserRef();
		    bfstr += "_crline_dip";
		    adcrld->setUserRef( bfstr.buf() );
		}
	    }
	    if ( !found2d && dlg.is2D() )
		found2d = true;
	}
    }

    attrset->removeUnused( true );
    if ( found2d )
    {
	for ( int idx=0; idx< attrset->nrDescs(); idx++ )
	    attrset->desc(idx)->set2D(true);
    }
}


void uiAttribDescSetEd::removeNotUsedAttr()
{
     if ( attrset ) attrset->removeUnused();
}


bool uiAttribDescSetEd::is2D() const
{
    if ( adsman ) 
	return adsman->is2D();
    else if ( attrset ) 
	return attrset->is2D();
    else 
	return false;
}
