/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          November 2001
 RCS:           $Id: uisettings.cc,v 1.31 2008-08-06 07:50:23 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uisettings.h"

#include "ptrman.h"
#include "settings.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiselsimple.h"


uiSettings::uiSettings( uiParent* p, const char* nm, const char* settskey )
	: uiDialog(p,uiDialog::Setup(nm,"Default User Settings","0.2.1"))
	, setts_(Settings::fetch(settskey))
{
    keyfld_ = new uiGenInput( this, "Settings keyword", StringInpSpec() );
    uiButton* pb = new uiPushButton( this, "&Select",
	    			     mCB(this,uiSettings,selPush), false );
    pb->setName( "Select Keyword" );
    pb->attach( rightOf, keyfld_ );

    valfld_ = new uiGenInput( this, "Value", StringInpSpec() );
    valfld_->attach( alignedBelow, keyfld_ );
}


uiSettings::~uiSettings()
{
}


void uiSettings::selPush( CallBacker* )
{
    PtrMan<IOPar> dtectsetts = setts_.subselect( "dTect" );
    BufferStringSet keys;
    for ( int idx=0; idx<dtectsetts->size(); idx++ )
	keys.add( dtectsetts->getKey(idx) );
    keys.sort();
    uiSelectFromList::Setup listsetup( "Setting selection", keys );
    listsetup.dlgtitle( keyfld_->text() );
    uiSelectFromList dlg( this, listsetup );
    dlg.selFld()->setHSzPol( uiObject::Wide );
    if ( !dlg.go() ) return;
    const int selidx = dlg.selection();
    if ( selidx < 0 ) return;

    const char* key = keys.get( selidx ).buf();
    keyfld_->setText( key );
    valfld_->setText( dtectsetts->find(key) );
}


bool uiSettings::acceptOK( CallBacker* )
{
    const BufferString ky = keyfld_->text();
    if ( ky.isEmpty() )
    {
	uiMSG().error( "Please enter a keyword to set" );
	return false;
    }

    setts_.set( IOPar::compKey("dTect",ky), valfld_->text() );
    if ( !setts_.write() )
    {
	uiMSG().error( "Cannot write settings" );
	return false;
    }

    return true;
}


static int sIconSize = -1;
// TODO: Move these keys to a header file in Basic
#define mIconsKey		"dTect.Icons"
#define mCBarKey		"dTect.ColorBar.show vertical"
#define mShowInlProgress	"dTect.Show inl progress"
#define mShowWheels		"dTect.Show wheels"
#define mNoShading		"dTect.No shading"
#define mVolRenShading		"dTect.Use VolRen shading"

struct LooknFeelSettings
{
    		LooknFeelSettings()
		    : iconsz(sIconSize < 0 ? uiObject::iconSize() : sIconSize)
		    , isvert(true)
		    , showwheels(true)
		    , showinlprogress(true)
		    , noshading(false)
		    , volrenshading(false)		{}

    int		iconsz;
    bool	isvert;
    bool	showwheels;
    bool	showinlprogress;
    bool	noshading;
    bool	volrenshading;
};



uiLooknFeelSettings::uiLooknFeelSettings( uiParent* p, const char* nm )
	: uiDialog(p,uiDialog::Setup(nm,"Look and Feel Settings","0.2.3"))
	, setts_(Settings::common())
    	, lfsetts_(*new LooknFeelSettings)
	, changed_(false)
{
    iconszfld_ = new uiGenInput( this, "Icon Size",
	    			 IntInpSpec(lfsetts_.iconsz) );

    setts_.getYN( mCBarKey, lfsetts_.isvert );
    colbarhvfld_ = new uiGenInput( this, "Color bar orientation",
			BoolInpSpec(lfsetts_.isvert,"Vertical","Horizontal") );
    colbarhvfld_->attach( alignedBelow, iconszfld_ );

    setts_.getYN( mShowInlProgress, lfsetts_.showinlprogress );
    showprogressfld_ = new uiGenInput( this,
	    "Show progress when loading stored data on inlines",
	    BoolInpSpec(lfsetts_.showinlprogress) );
    showprogressfld_->attach( alignedBelow, colbarhvfld_ );

    setts_.getYN( mShowWheels, lfsetts_.showwheels );
    showwheelsfld_ = new uiGenInput( this, "Show Zoom/Rotation tools",
	    			    BoolInpSpec(lfsetts_.showwheels) );
    showwheelsfld_->attach( alignedBelow, showprogressfld_ );

    setts_.getYN( mNoShading, lfsetts_.noshading );
    useshadingfld_ = new uiGenInput( this, "Use OpenGL shading when available",
				    BoolInpSpec(!lfsetts_.noshading) );
    useshadingfld_->attach( alignedBelow, showwheelsfld_ );
    useshadingfld_->valuechanged.notify(
	    		mCB(this,uiLooknFeelSettings,shadingChange) );
    setts_.getYN( mVolRenShading, lfsetts_.volrenshading );
    volrenshadingfld_ = new uiGenInput( this, "Also for volume rendering?",
				    BoolInpSpec(lfsetts_.volrenshading) );
    volrenshadingfld_->attach( alignedBelow, useshadingfld_ );

    shadingChange(0);
}


uiLooknFeelSettings::~uiLooknFeelSettings()
{
    delete &lfsetts_;
}


void uiLooknFeelSettings::shadingChange( CallBacker* )
{
    volrenshadingfld_->display( useshadingfld_->getBoolValue() );
}


void uiLooknFeelSettings::updateSettings( bool oldval, bool newval,
					  const char* key )
{
    if ( oldval != newval )
    {
	changed_ = true;
	setts_.setYN( key, newval );
    }
}


bool uiLooknFeelSettings::acceptOK( CallBacker* )
{
    const int newiconsz = iconszfld_->getIntValue();
    if ( newiconsz < 10 || newiconsz > 64 )
    {
	uiMSG().setNextCaption( "Yeah right" );
	uiMSG().error( "Please specify an icon size in the range 10-64" );
	return false;
    }

    if ( newiconsz != lfsetts_.iconsz )
    {
	IOPar* iopar = setts_.subselect( mIconsKey );
	if ( !iopar ) iopar = new IOPar;
	iopar->set( "size", newiconsz );
	setts_.mergeComp( *iopar, mIconsKey );
	changed_ = true;
	delete iopar;
	sIconSize = newiconsz;
    }

    updateSettings( lfsetts_.isvert, colbarhvfld_->getBoolValue(), mCBarKey );
    const bool newnoshading = !useshadingfld_->getBoolValue();
    updateSettings( lfsetts_.noshading, newnoshading, mNoShading );

    bool newvolrenshading = !newnoshading;
    if ( newvolrenshading )
	newvolrenshading = volrenshadingfld_->getBoolValue();
    updateSettings( lfsetts_.volrenshading, newvolrenshading, mVolRenShading );

    updateSettings( lfsetts_.showwheels, showwheelsfld_->getBoolValue(),
	    	    mShowWheels );
    updateSettings( lfsetts_.showinlprogress, showprogressfld_->getBoolValue(),
	    	    mShowInlProgress );

    if ( changed_ && !setts_.write() )
    {
	changed_ = false;
	uiMSG().error( "Cannot write settings" );
	return false;
    }

    return true;
}
