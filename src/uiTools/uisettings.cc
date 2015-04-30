/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          November 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisettings.h"

#include "dirlist.h"
#include "envvars.h"
#include "hiddenparam.h"
#include "oddirs.h"
#include "od_helpids.h"
#include "posimpexppars.h"
#include "ptrman.h"
#include "settings.h"
#include "survinfo.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitable.h"
#include "uivirtualkeyboard.h"

static const char* sKeyCommon = "<general>";


static void getGrps( BufferStringSet& grps )
{
    grps.add( sKeyCommon );
    BufferString msk( "settings*" );
    const char* dtectuser = GetSoftwareUser();
    const bool needdot = dtectuser && *dtectuser;
    if ( needdot ) msk += ".*";
    DirList dl( GetSettingsDir(), DirList::FilesOnly, msk );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	BufferString fnm( dl.get(idx) );
	char* dotptr = fnm.find( '.' );
	if ( (needdot && !dotptr) || (!needdot && dotptr) )
	    continue;
	if ( dotptr )
	{
	    BufferString usr( dotptr + 1 );
	    if ( usr != dtectuser )
		continue;
	    *dotptr = '\0';
	}
	const char* underscoreptr = firstOcc( fnm.buf(), '_' );
	if ( !underscoreptr || !*underscoreptr )
	    continue;
	grps.add( underscoreptr + 1 );
    }
}


uiSettings::uiSettings( uiParent* p, const char* nm, const char* settskey )
	: uiDialog(p,uiDialog::Setup(nm,tr("Set User Settings value"),
                                     mODHelpKey(mSettingsHelpID)) )
        , issurvdefs_(FixedString(settskey)==sKeySurveyDefs())
	, grpfld_(0)
{
    setCurSetts();
    if ( issurvdefs_ )
    {
	setTitleText( tr("Set Survey default value") );
	setHelpKey( mODHelpKey(mSurveySettingsHelpID) );
    }
    else
    {
	BufferStringSet grps; getGrps( grps );
	grpfld_ = new uiGenInput( this, tr("Settings group"),
				  StringListInpSpec(grps) );
	grpfld_->valuechanged.notify( mCB(this,uiSettings,grpChg) );
    }

    tbl_ = new uiTable( this, uiTable::Setup(10,2).manualresize(true),
				"Settings editor" );
    tbl_->setColumnLabel( 0, "Keyword" );
    tbl_->setColumnLabel( 1, "Value" );
    // tbl_->setColumnResizeMode( uiTable::Interactive );
    tbl_->setStretch( 2, 2 );
    tbl_->setPrefWidth( 400 );
    tbl_->setPrefHeight( 300 );
    if ( grpfld_ )
	tbl_->attach( ensureBelow, grpfld_ );

    postFinalise().notify( mCB(this,uiSettings,dispNewGrp) );
}


uiSettings::~uiSettings()
{
    deepErase( chgdsetts_ );
}


int uiSettings::getChgdSettIdx( const char* nm ) const
{
    for ( int idx=0; idx<chgdsetts_.size(); idx++ )
    {
	if ( chgdsetts_[idx]->name() == nm )
	    return idx;
    }
    return -1;
}


const IOPar& uiSettings::orgPar() const
{
    const IOPar* iop = &SI().getPars();
    if ( !issurvdefs_ )
    {
	const BufferString grp( grpfld_ ? grpfld_->text() : sKeyCommon );
	iop = grp == sKeyCommon ? &Settings::common() : &Settings::fetch(grp);
    }
    return *iop;
}


void uiSettings::setCurSetts()
{
    const IOPar* iop = &orgPar();
    if ( !issurvdefs_ )
    {
	const int chgdidx = getChgdSettIdx( iop->name() );
	if ( chgdidx >= 0 )
	    iop = chgdsetts_[chgdidx];
    }
    cursetts_ = iop;
}


void uiSettings::getChanges()
{
    IOPar* workpar = 0;
    const int chgdidx = getChgdSettIdx( cursetts_->name() );
    if ( chgdidx >= 0 )
	workpar = chgdsetts_[chgdidx];
    const bool alreadyinset = workpar;
    if ( alreadyinset )
	workpar->setEmpty();
    else
	workpar = new IOPar( cursetts_->name() );

    const int sz = tbl_->nrRows();
    for ( int irow=0; irow<sz; irow++ )
    {
	BufferString kybuf = tbl_->text( RowCol(irow,0) );
	kybuf.trimBlanks();
	if ( kybuf.isEmpty() )
	    continue;
	BufferString valbuf = tbl_->text( RowCol(irow,1) );
	valbuf.trimBlanks();
	if ( valbuf.isEmpty() )
	    continue;
	workpar->set( kybuf, valbuf );
    }

    if ( !orgPar().isEqual(*workpar,true) )
    {
	if ( !alreadyinset )
	    chgdsetts_ += workpar;
    }
    else
    {
	if ( alreadyinset )
	    chgdsetts_ -= workpar;
	delete workpar;
    }
}


bool uiSettings::commitSetts( const IOPar& iop )
{
    Settings& setts = Settings::fetch( iop.name() );
    setts.IOPar::operator =( iop );
    if ( !setts.write(false) )
    {
	uiMSG().error( "Cannot write ", setts.name() );
	return false;
    }
    return true;
}


void uiSettings::grpChg( CallBacker* )
{
    getChanges();
    setCurSetts();
    dispNewGrp( 0 );
}


void uiSettings::dispNewGrp( CallBacker* )
{
    BufferStringSet keys, vals;
    for ( int idx=0; idx<cursetts_->size(); idx++ )
    {
	keys.add( cursetts_->getKey(idx) );
	vals.add( cursetts_->getValue(idx) );
    }
    int* idxs = keys.getSortIndexes();
    keys.useIndexes(idxs); vals.useIndexes(idxs);
    delete [] idxs;

    const int sz = keys.size();
    tbl_->clearTable();
    tbl_->setNrRows( sz + 5 );
    for ( int irow=0; irow<sz; irow++ )
    {
	tbl_->setText( RowCol(irow,0), keys.get(irow) );
	tbl_->setText( RowCol(irow,1), vals.get(irow) );
    }

    tbl_->resizeColumnToContents( 1 );
    tbl_->resizeColumnToContents( 2 );
}


bool uiSettings::acceptOK( CallBacker* )
{
    getChanges();
    if ( chgdsetts_.isEmpty() )
	return true;

    if ( issurvdefs_ )
    {
	SI().getPars() = *chgdsetts_[0];
	SI().savePars();
	PosImpExpPars::refresh();
    }
    else
    {
	for ( int idx=0; idx<chgdsetts_.size(); idx++ )
	{
	    IOPar* iop = chgdsetts_[idx];
	    if ( commitSetts(*iop) )
		{ chgdsetts_.removeSingle( idx ); delete iop; idx--; }
	}
	if ( !chgdsetts_.isEmpty() )
	    return false;
    }

    return true;
}


static int theiconsz = -1;
// TODO: Move these keys to a header file in Basic
#define mIconsKey		"dTect.Icons"
#define mCBarKey		"dTect.ColorBar.show vertical"
#define mShowInlProgress	"dTect.Show inl progress"
#define mShowCrlProgress	"dTect.Show crl progress"
#define mTextureResFactor	"dTect.Default texture resolution factor"
#define mUseSurfShaders		"dTect.Use surface shaders"
#define mUseVolShaders		"dTect.Use volume shaders"

HiddenParam<uiSettingsGroup,char> needsrestartgrp( false );
HiddenParam<uiSettingsGroup,char> needsrenewalgrp( false );

mImplFactory2Param( uiSettingsGroup, uiParent*, Settings&,
		    uiSettingsGroup::factory )

uiSettingsGroup::uiSettingsGroup( uiParent* p, const uiString& caption,
				  Settings& setts )
    : uiDlgGroup(p,caption)
    , setts_(setts)
    , changed_(false)
{
    needsrestartgrp.setParam( this, false );
    needsrenewalgrp.setParam( this, false );
}


uiSettingsGroup::~uiSettingsGroup()
{
    needsrestartgrp.removeParam( this );
    needsrenewalgrp.removeParam( this );
}


void uiSettingsGroup::setNeedsRestart( bool yn )
{ needsrestartgrp.setParam( this, yn ); }


void uiSettingsGroup::setNeedsRenewal( bool yn )
{ needsrenewalgrp.setParam( this, yn ); }


bool uiSettingsGroup::needsRestart() const
{ return needsrestartgrp.getParam( this ); }


bool uiSettingsGroup::needsRenewal() const
{ return needsrenewalgrp.getParam( this ); }


const char* uiSettingsGroup::errMsg() const
{ return errmsg_.buf(); }


#define mUpdateSettings( type, setfunc ) \
void uiSettingsGroup::updateSettings( type oldval, type newval, \
				      const char* key ) \
{ \
    if ( oldval != newval ) \
    { \
	changed_ = true; \
	setts_.setfunc( key, newval ); \
    } \
}

mUpdateSettings( bool, setYN )
mUpdateSettings( int, set )
mUpdateSettings( const OD::String&, set )


HiddenParam<uiGeneralSettingsGroup,char> enabvirtkeybgrp( false );
HiddenParam<uiGeneralSettingsGroup,uiGenInput*> virtkeybfldgrp( 0 );

// uiGeneralSettingsGroup
uiGeneralSettingsGroup::uiGeneralSettingsGroup( uiParent* p, Settings& setts )
    : uiSettingsGroup(p,"General",setts)
    , iconsz_(theiconsz < 0 ? uiObject::iconSize() : theiconsz)
    , vertcoltab_(true)
    , showinlprogress_(true)
    , showcrlprogress_(true)
{
    iconszfld_ = new uiGenInput( this, "Icon Size",
				 IntInpSpec(iconsz_,10,64) );

    setts_.getYN( mCBarKey, vertcoltab_ );
    colbarhvfld_ = new uiGenInput( this, "Color bar orientation",
		BoolInpSpec(vertcoltab_,"Vertical","Horizontal") );
    colbarhvfld_->attach( alignedBelow, iconszfld_ );

    setts_.getYN( mShowInlProgress, showinlprogress_ );
    showinlprogressfld_ = new uiGenInput( this,
	    "Show progress when loading stored data on in-lines",
	    BoolInpSpec(showinlprogress_) );
    showinlprogressfld_->attach( alignedBelow, colbarhvfld_ );

    setts_.getYN( mShowCrlProgress, showcrlprogress_ );
    showcrlprogressfld_ = new uiGenInput( this,
	    "Show progress when loading stored data on cross-lines",
	    BoolInpSpec(showcrlprogress_) );
    showcrlprogressfld_->attach( alignedBelow, showinlprogressfld_ );

    bool enabvirtualkeyboard = false;
    setts_.getYN( uiVirtualKeyboard::sKeyEnabVirtualKeyboard(),
		  enabvirtualkeyboard );
    enabvirtkeybgrp.setParam( this, enabvirtualkeyboard );

    uiGenInput* virtualkeyboardfld = new uiGenInput( this,
		"Enable Virtual Keyboard",
		BoolInpSpec(enabvirtualkeyboard) );
    virtualkeyboardfld->attach( alignedBelow, showcrlprogressfld_ );
    virtkeybfldgrp.setParam( this, virtualkeyboardfld );
}


bool uiGeneralSettingsGroup::acceptOK()
{
    const int newiconsz = iconszfld_->getIntValue();
    if ( newiconsz < 10 || newiconsz > 64 )
    {
	errmsg_.set( "Please specify an icon size in the range 10-64" );
	return false;
    }

    if ( newiconsz != iconsz_ )
    {
	IOPar* iopar = setts_.subselect( mIconsKey );
	if ( !iopar ) iopar = new IOPar;
	iopar->set( "size", newiconsz );
	setts_.mergeComp( *iopar, mIconsKey );
	changed_ = true;
	setNeedsRestart( true );
	delete iopar;
	theiconsz = newiconsz;
    }

    updateSettings( vertcoltab_, colbarhvfld_->getBoolValue(), mCBarKey );
    if ( changed_ ) setNeedsRestart( true );

    updateSettings( showinlprogress_, showinlprogressfld_->getBoolValue(),
		    mShowInlProgress );
    updateSettings( showcrlprogress_, showcrlprogressfld_->getBoolValue(),
		    mShowCrlProgress );

    const bool enabvirtualkeyboard = enabvirtkeybgrp.getParam( this );
    uiGenInput* virtualkeyboardfld = virtkeybfldgrp.getParam( this );
    const bool newval = virtualkeyboardfld ? virtualkeyboardfld->getBoolValue()
					   : enabvirtualkeyboard;
    updateSettings( enabvirtualkeyboard, newval,
		    uiVirtualKeyboard::sKeyEnabVirtualKeyboard() );

    return true;
}

HiddenParam<uiSettingsGroup,uiGenInput*>	enablemipmappingfld_(0);
HiddenParam<uiSettingsGroup,uiLabeledComboBox*> anisotropicpowerfld_(0);
HiddenParam<uiSettingsGroup,char>		enablemipmapping_(0);
HiddenParam<uiSettingsGroup,int>		anisotropicpower_(0);


// uiVisSettingsGroup
uiVisSettingsGroup::uiVisSettingsGroup( uiParent* p, Settings& setts )
    : uiSettingsGroup(p,"Visualisation",setts)
    , textureresfactor_(0)
    , usesurfshaders_(true)
    , usevolshaders_(true)
{
    uiLabel* shadinglbl = new uiLabel( this,
				       "Use OpenGL shading when available:" );
    setts_.getYN( mUseSurfShaders, usesurfshaders_ );
    usesurfshadersfld_ = new uiGenInput( this, "for surface rendering",
					 BoolInpSpec(usesurfshaders_) );
    usesurfshadersfld_->attach( leftAlignedBelow, shadinglbl );
    setts_.getYN( mUseVolShaders, usevolshaders_ );
    usevolshadersfld_ = new uiGenInput( this, "for volume rendering",
					BoolInpSpec(usevolshaders_) );
    usevolshadersfld_->attach( leftAlignedBelow, usesurfshadersfld_, 0 );

    setts_.get( mTextureResFactor, textureresfactor_ );
    uiLabeledComboBox* lcb =
	new uiLabeledComboBox( this, "Default texture resolution" );
    lcb->attach( alignedBelow, usevolshadersfld_ );
    textureresfactorfld_ = lcb->box();
    textureresfactorfld_->addItem( "Standard" );
    textureresfactorfld_->addItem( "Higher" );
    textureresfactorfld_->addItem( "Highest" );

    int selection = 0;

    if ( textureresfactor_ >= 0 && textureresfactor_ <= 2 )
	    selection = textureresfactor_;

    // add the System default option if the environment variable is set
    const char* envvar = GetEnvVar( "OD_DEFAULT_TEXTURE_RESOLUTION_FACTOR" );
    if ( envvar && iswdigit(*envvar) )
    {
	textureresfactorfld_->addItem( "System default" );
	if ( textureresfactor_ == -1 )
	    selection = 3;
    }

    textureresfactorfld_->setCurrentItem( selection );

    bool enablemipmapping = true;
    setts_.getYN( "dTect.Enable mipmapping", enablemipmapping );
    enablemipmapping_.setParam( this, enablemipmapping );
    enablemipmappingfld_.setParam( this,
	new uiGenInput( this, "Mipmap anti-aliasing",
			BoolInpSpec(enablemipmapping) ) );

    enablemipmappingfld_.getParam(this)->attach( alignedBelow, lcb );
    enablemipmappingfld_.getParam(this)->valuechanged.notify(
			    mCB(this,uiVisSettingsGroup,mipmappingToggled) );

    anisotropicpowerfld_.setParam( this,
		    new uiLabeledComboBox(this, "Sharpen oblique textures") );
    anisotropicpowerfld_.getParam(this)->attach( alignedBelow,
					enablemipmappingfld_.getParam(this) );

    anisotropicpowerfld_.getParam(this)->box()->addItem( " 0 x" );
    anisotropicpowerfld_.getParam(this)->box()->addItem( " 1 x" );
    anisotropicpowerfld_.getParam(this)->box()->addItem( " 2 x" );
    anisotropicpowerfld_.getParam(this)->box()->addItem( " 4 x" );
    anisotropicpowerfld_.getParam(this)->box()->addItem( " 8 x" );
    anisotropicpowerfld_.getParam(this)->box()->addItem( "16 x" );
    anisotropicpowerfld_.getParam(this)->box()->addItem( "32 x" );

    int anisotropicpower = 4;
    setts_.get( "dTect.Anisotropic power", anisotropicpower );
    if ( anisotropicpower < -1 )
	anisotropicpower = -1;
    if ( anisotropicpower > 5 )
	anisotropicpower = 5;
    anisotropicpower_.setParam( this, anisotropicpower );
    anisotropicpowerfld_.getParam(this)->box()->setCurrentItem(
							anisotropicpower+1 );

    mipmappingToggled( 0 );
}


bool uiVisSettingsGroup::acceptOK()
{
    const bool usesurfshaders = usesurfshadersfld_->getBoolValue();
    updateSettings( usesurfshaders_, usesurfshaders, mUseSurfShaders );

    const bool usevolshaders = usevolshadersfld_->getBoolValue();
    updateSettings( usevolshaders_, usevolshaders, mUseVolShaders );

    bool textureresfacchanged = false;
    // track this change separately as this will be applied with immediate
    // effect, unlike other settings
    int val = ( textureresfactorfld_->currentItem() == 3 ) ? -1 :
		textureresfactorfld_->currentItem();
    if ( textureresfactor_ != val )
    {
	textureresfacchanged = true;
	setts_.set( mTextureResFactor, val );
    }

    if ( textureresfacchanged ) changed_ = true;

    bool enablemipmapping = enablemipmapping_.getParam(this);
    updateSettings( enablemipmapping,
		    enablemipmappingfld_.getParam(this)->getBoolValue(),
		    "dTect.Enable mipmapping" );
    enablemipmapping_.setParam( this, enablemipmapping );

    int anisotropicpower = anisotropicpower_.getParam(this);
    updateSettings( anisotropicpower,
		    anisotropicpowerfld_.getParam(this)->box()->currentItem()-1,
		    "dTect.Anisotropic power" );
    anisotropicpower_.setParam( this, anisotropicpower );

    if ( changed_ )
	setNeedsRenewal( true );

    return true;
}


void uiVisSettingsGroup::mipmappingToggled( CallBacker* )
{
    anisotropicpowerfld_.getParam(this)->setSensitive(
			enablemipmappingfld_.getParam(this)->getBoolValue() );
}


// uiSettingsDlg
HiddenParam<uiSettingsDlg,char> needsrestartdlg( false );
HiddenParam<uiSettingsDlg,char> needsrenewaldlg( false );

uiSettingsDlg::uiSettingsDlg( uiParent* p )
    : uiTabStackDlg(p,uiDialog::Setup("OpendTect Settings",mNoDlgTitle,
				      mODHelpKey(mLooknFeelSettingsHelpID)))
    , setts_(Settings::common())
    , changed_(false)
{
    const BufferStringSet& nms = uiSettingsGroup::factory().getNames();
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	uiSettingsGroup* grp =
		uiSettingsGroup::factory().create( nms.get(idx),
						   tabstack_->tabGroup(),
						   setts_ );
	grp->attach( hCentered );
	addGroup( grp );
	grps_ += grp;
    }

    needsrestartdlg.setParam( this, false );
    needsrenewaldlg.setParam( this, false );
}


uiSettingsDlg::~uiSettingsDlg()
{
    needsrestartdlg.removeParam( this );
    needsrenewaldlg.removeParam( this );
}


bool uiSettingsDlg::needsRestart() const
{ return needsrestartdlg.getParam( this ); }


bool uiSettingsDlg::needsRenewal() const
{ return needsrenewaldlg.getParam( this ); }


bool uiSettingsDlg::acceptOK( CallBacker* cb )
{
    if ( !uiTabStackDlg::acceptOK(cb) )
	return false;

    changed_ = false;
    for ( int idx=0; idx<grps_.size(); idx++ )
	changed_ = changed_ || grps_[idx]->isChanged();

    if ( changed_ && !setts_.write() )
    {
	uiMSG().error( "Cannot write settings" );
	return false;
    }

    bool needsrestart = false;
    bool needsrenewal = false;
    for ( int idx=0; idx<grps_.size(); idx++ )
    {
	needsrestart = needsrestart || grps_[idx]->needsRestart();
	needsrenewal = needsrenewal || grps_[idx]->needsRenewal();
    }

    needsrestartdlg.setParam( this, needsrestart );
    needsrenewaldlg.setParam( this, needsrenewal );
    return true;
}
