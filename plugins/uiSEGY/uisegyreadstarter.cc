/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          July 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id:$";

#include "uisegyreadstarter.h"

#include "uisegyreadstartinfo.h"
#include "uisegyreadfinisher.h"
#include "uisegyimpparsdlg.h"
#include "uisegyimptype.h"
#include "uisegyexamine.h"
#include "uisegymanip.h"
#include "uisegydef.h"
#include "uisegyread.h"
#include "uifileinput.h"
#include "uifiledlg.h"
#include "uiseparator.h"
#include "uisurvmap.h"
#include "uihistogramdisplay.h"
#include "uitaskrunner.h"
#include "uitoolbutton.h"
#include "uispinbox.h"
#include "uilineedit.h"
#include "uimsg.h"
#include "uimenu.h"
#include "uisplitter.h"
#include "segyhdr.h"
#include "segyhdrkeydata.h"
#include "seisinfo.h"
#include "posinfodetector.h"
#include "dataclipper.h"
#include "filepath.h"
#include "dirlist.h"
#include "oddirs.h"
#include "survinfo.h"
#include "od_istream.h"
#include "settings.h"
#include "timer.h"


#define mForSurvSetup forsurvsetup
#define mSurvMapHeight 350
#define mDefSize 250
#define mClipSamplerBufSz 100000

static const char* sKeyClipRatio = "Amplitudes.Clip Ratio";
static const char* sKeyIncludeZeros = "Amplitudes.Include Zeros";


uiSEGYReadStarter::uiSEGYReadStarter( uiParent* p, bool forsurvsetup,
					const SEGY::ImpType* imptyp )
    : uiDialog(p,uiDialog::Setup(tr("Import SEG-Y Data"),
			imptyp ? uiString("Import %1").arg(imptyp->dispText())
				: mNoDlgTitle,
				  mTODOHelpKey ).nrstatusflds(1) )
    , filereadopts_(0)
    , typfld_(0)
    , useicbut_(0)
    , usexybut_(0)
    , coordscalefld_(0)
    , ampldisp_(0)
    , survmap_(0)
    , detectrev0flds_(true)
    , userfilename_("_") // any non-empty non-existing
    , scaninfos_(0)
    , clipsampler_(*new DataClipSampler(100000))
    , survinfo_(0)
    , survinfook_(false)
    , timer_(0)
{
    if ( mForSurvSetup )
	loaddef_.icvsxytype_ = SEGY::FileReadOpts::Both;
    else
    {
	setCtrlStyle( RunAndClose );
	setOkText( tr("Next >>") );
	loaddef_.icvsxytype_ = SEGY::FileReadOpts::ICOnly;
    }

    topgrp_ = new uiGroup( this, "Top group" );
    uiFileInput::Setup fisu( uiFileDialog::Gen, filespec_.fileName() );
    fisu.filter( uiSEGYFileSpec::fileFilter() ).forread( true )
	.objtype( tr("SEG-Y") );
    inpfld_ = new uiFileInput( topgrp_, "Input file(s) (*=wildcard)",
				fisu );
    inpfld_->valuechanged.notify( mCB(this,uiSEGYReadStarter,inpChg) );
    editbut_ = uiButton::getStd( topgrp_, OD::Edit,
			         mCB(this,uiSEGYReadStarter,editFile), false );
    editbut_->attach( rightOf, inpfld_ );
    editbut_->setSensitive( false );

    if ( imptyp )
	fixedimptype_ = *imptyp;
    else
    {
	typfld_ = new uiSEGYImpType( topgrp_, !mForSurvSetup );
	typfld_->typeChanged.notify( mCB(this,uiSEGYReadStarter,typChg) );
	typfld_->attach( alignedBelow, inpfld_ );
    }

    uiSeparator* sep = new uiSeparator( this, "Top sep" );
    sep->attach( stretchedBelow, topgrp_ );

    midgrp_ = new uiGroup( this, "Mid group" );
    midgrp_->attach( ensureBelow, sep );
    infofld_ = new uiSEGYReadStartInfo( midgrp_, loaddef_, imptyp );
    infofld_->loaddefChanged.notify( mCB(this,uiSEGYReadStarter,defChg) );
    infofld_->revChanged.notify( mCB(this,uiSEGYReadStarter,revChg) );

    sep = new uiSeparator( this, "Bot sep" );
    sep->attach( stretchedBelow, midgrp_ );

    botgrp_ = new uiGroup( this, "Bottom group" );
    botgrp_->attach( ensureBelow, sep );
    if ( mForSurvSetup )
    {
	survmap_ = new uiSurveyMap( botgrp_, true );
	survmap_->setSurveyInfo( 0 );
    }
    uiGroup* amplgrp = createAmplDisp();
    if ( survmap_ )
    {
	uiSplitter* spl = new uiSplitter( botgrp_ );
	survmap_->view().setPrefWidth( mDefSize );
	spl->addGroup( survmap_ );
	spl->addGroup( amplgrp );
	spl->attach( ensureBelow, infofld_ );
    }
    createTools();
    botgrp_->setStretch( 2, 1 );

    setToolStates();
    postFinalise().notify( mCB(this,uiSEGYReadStarter,initWin) );
}


#undef mForSurvSetup
#define mForSurvSetup survmap_


void uiSEGYReadStarter::createTools()
{
    uiToolButton* openbut = new uiToolButton( midgrp_, "open",
				tr("Use Saved SEG-Y setup"),
				mCB(this,uiSEGYReadStarter,readParsCB) );
    openbut->attach( rightOf, infofld_ );
    uiToolButton* savebut = new uiToolButton( midgrp_, "save",
				tr("Store this setup"),
				mCB(this,uiSEGYReadStarter,writeParsCB) );
    savebut->attach( rightOf, openbut );

    fullscanbut_ = new uiToolButton( midgrp_, "fullscan",
				    tr("Scan the entire input"),
				    mCB(this,uiSEGYReadStarter,fullScanReq) );
    fullscanbut_->attach( alignedBelow, openbut );

    uiGroup* examinegrp = new uiGroup( midgrp_, "Examine group" );
    examinebut_ = new uiToolButton( examinegrp, "examine",
				    uiString::emptyString(),
				    mCB(this,uiSEGYReadStarter,examineCB) );
    examinenrtrcsfld_ = new uiSpinBox( examinegrp, 0, "Examine traces" );
    examinenrtrcsfld_->setInterval( 0, 1000000, 10 );
    examinenrtrcsfld_->setHSzPol( uiObject::Small );
    examinenrtrcsfld_->setToolTip( tr("Number of traces to examine") );
    examinenrtrcsfld_->attach( alignedBelow, examinebut_ );
    int nrex = 1000; Settings::common().get( sKeySettNrTrcExamine, nrex );
    examinenrtrcsfld_->setInterval( 10, 1000000, 10 );
    examinenrtrcsfld_->setValue( nrex );
    examinegrp->attach( alignedBelow, fullscanbut_ );
    examinegrp->setFrame( true );

    bool needicvsxy = !mForSurvSetup;
    if ( imptypeFixed() && (fixedimptype_.isVSP() || fixedimptype_.is2D()) )
	needicvsxy = false;
    if ( needicvsxy )
    {
	const CallBack icvsxycb( mCB(this,uiSEGYReadStarter,icxyCB) );
	useicbut_ = new uiRadioButton( midgrp_, tr("Use I/C"), icvsxycb );
	useicbut_->setToolTip(
	    tr("Select to use Inline/Crossline for positioning."
		"\nX and Y will be calculated using the survey setup.\nPrefer "
		"this if you have valid In- and Crossline numbers available."));
	useicbut_->attach( alignedBelow, examinegrp );

	usexybut_ = new uiRadioButton( midgrp_, tr("Use (X,Y)"), icvsxycb );
	usexybut_->setToolTip(
	    tr( "Select to use (X,Y) for positioning.\nInline/Crossline "
		"will be calculated using the survey setup.\nOnly use this "
		"option if you do not have valid In- and Crossline numbers.") );
	usexybut_->attach( alignedBelow, useicbut_ );
    }

    if ( !imptypeFixed() || !fixedimptype_.isVSP() )
    {
	coordscalefld_ = new uiLineEdit( midgrp_, FloatInpSpec(), "CoordScale");
	coordscalefld_->setHSzPol( uiObject::Small );
	coordscalefld_->setToolTip( tr( "Enter a value if you want to ignore "
	    "the coordinate scaling in the trace headers."
	    "\nAll coordinates read will then be multiplied by that factor." ));
	coordscalefld_->editingFinished.notify(
				mCB(this,uiSEGYReadStarter,coordscaleChg) );
	if ( usexybut_ )
	    coordscalefld_->attach( alignedBelow, usexybut_ );
	else
	    coordscalefld_->attach( alignedBelow, examinegrp );
    }
}


uiGroup* uiSEGYReadStarter::createAmplDisp()
{
    uiGroup* amplgrp = new uiGroup( botgrp_, "Hist grp" );
    const CallBack adupcb( mCB(this,uiSEGYReadStarter,updateAmplDisplay) );

    uiHistogramDisplay::Setup hdsu;
    hdsu.noyaxis( false ).noygridline(true).annoty( false );
    hdsu.canvaswidth(mDefSize)
	.canvasheight( mForSurvSetup ? mSurvMapHeight : mDefSize );
    ampldisp_ = new uiHistogramDisplay( amplgrp, hdsu );
    ampldisp_->setTitle( tr("Amplitudes") );

    clipfld_ = new uiSpinBox( amplgrp, 1, "Clipping percentage" );
    clipfld_->setInterval( 0.f, 49.9f, 0.1f );
    clipfld_->setValue( 0.1f );
    clipfld_->setToolTip( tr("Percentage clip for display") );
    clipfld_->setSuffix( uiString("%") );
    clipfld_->setHSzPol( uiObject::Small );
    clipfld_->attach( rightOf, ampldisp_ );
    clipfld_->valueChanging.notify( adupcb );

    inc0sbox_ = new uiCheckBox( amplgrp, "Zeros" );
    inc0sbox_->attach( alignedBelow, clipfld_ );
    inc0sbox_->setHSzPol( uiObject::Small );
    inc0sbox_->setToolTip( tr("Include value '0' for histogram display") );
    inc0sbox_->activated.notify( adupcb );

    return amplgrp;
}


uiSEGYReadStarter::~uiSEGYReadStarter()
{
    delete survinfo_;
    delete timer_;
    delete filereadopts_;
    delete scaninfos_;
    delete &clipsampler_;
}


FullSpec uiSEGYReadStarter::fullSpec() const
{
    const SEGY::ImpType& imptyp = impType();
    FullSpec ret( imptyp.geomType(), imptyp.isVSP() );
    ret.rev_ = loaddef_.revision_;
    ret.spec_ = filespec_;
    ret.pars_ = filepars_;
    ret.zinfeet_ = zInFeet();
    if ( filereadopts_ )
	ret.readopts_ = *filereadopts_;
    return ret;
}


void uiSEGYReadStarter::clearDisplay()
{
    infofld_->clearInfo();
    if ( ampldisp_ )
	ampldisp_->setEmpty();
    if ( mForSurvSetup )
	survmap_->setSurveyInfo( 0 );
    setToolStates();
}


void uiSEGYReadStarter::setImpTypIdx( int tidx )
{
    if ( imptypeFixed() )
	{ pErrMsg( "Cannot set type if fixed" ); return; }

    typfld_->setTypIdx( tidx ); // should trigger its callback
}


const SEGY::ImpType& uiSEGYReadStarter::impType() const
{
    return typfld_ ? typfld_->impType() : fixedimptype_;
}


void uiSEGYReadStarter::execNewScan( LoadDefChgType ct, bool full )
{
    delete scaninfos_; scaninfos_ = 0;
    clipsampler_.reset();
    clearDisplay();
    if ( !getFileSpec() )
	return;

    const SEGY::ImpType& imptyp = impType();
    scaninfos_ = new SEGY::ScanInfoSet( imptyp.is2D(), imptyp.isPS() );
    scaninfos_->setName( userfilename_ );

    PtrMan<TaskRunner> trunner;
    if ( full )
	trunner = new uiTaskRunner( this );

    MouseCursorChanger chgr( MouseCursor::Wait );
    const BufferString fnm0 = filespec_.fileName( 0 );
    if ( !scanFile(fnm0,ct,trunner) )
	return;

    if ( !full && detectrev0flds_ )
    {
	detectrev0flds_ = false;
	scaninfos_->scanInfo(0).keyData().setBest( *loaddef_.hdrdef_ );
	scaninfos_->setEmpty();
	if ( !scanFile(fnm0,ct,trunner) )
	    return;
    }

    const int nrfiles = filespec_.nrFiles();
    for ( int idx=1; idx<nrfiles; idx++ )
	scanFile( filespec_.fileName(idx), KeepAll, trunner );

    scaninfos_->finish();
    displayScanResults();
}


bool uiSEGYReadStarter::needICvsXY() const
{
    const SEGY::ImpType imptyp = impType();
    return !imptyp.is2D() && !imptyp.isVSP();
}


int uiSEGYReadStarter::examineNrTraces() const
{
    return examinenrtrcsfld_->getIntValue();
}


float uiSEGYReadStarter::ratioClip() const
{
    float clipperc = clipfld_->getFValue();
    const bool useclip = !mIsUdf(clipperc) && clipperc > 0.05;
    return useclip ? clipperc * 0.01f : 0.f;
}


bool uiSEGYReadStarter::incZeros() const
{
    return inc0sbox_->isChecked();
}


void uiSEGYReadStarter::setToolStates()
{
    const int nrfiles = filespec_.nrFiles();
    examinebut_->setSensitive( nrfiles > 0 );
    fullscanbut_->setSensitive( nrfiles > 0 );
    examinebut_->setToolTip( nrfiles > 1 ? tr("Examine first input file")
					 : tr("Examine input file") );
    if ( useicbut_ )
    {
	const bool isneeded = needICvsXY();
	useicbut_->display( isneeded );
	usexybut_->display( isneeded );
	if ( isneeded )
	    updateICvsXYButtons();
    }

    if ( coordscalefld_ )
	coordscalefld_->display( loaddef_.needXY() );

    editbut_->setSensitive( nrfiles==1 && File::exists(filespec_.fileName(0)) );
}


void uiSEGYReadStarter::initWin( CallBacker* )
{
    typChg( 0 );
    if ( !mForSurvSetup )
	inpChg( 0 );

    if ( filespec_.isEmpty() )
    {
	timer_ = new Timer( "uiSEGYReadStarter timer" );
	timer_->tick.notify( mCB(this,uiSEGYReadStarter,firstSel));
	timer_->start( 1, true );
    }

    if ( mForSurvSetup || (imptypeFixed() && fixedimptype_.isVSP()) )
	return;

    uiButton* okbut = button( OK );
    const CallBack impcb( mCB(this,uiSEGYReadStarter,runClassicImp) );
    const CallBack linkcb( mCB(this,uiSEGYReadStarter,runClassicLink) );
    uiPushButton* execoldbut = new uiPushButton( okbut->parent(),
					tr("'Classic'"), impcb, false );
    execoldbut->setIcon( "launch" );
    execoldbut->setToolTip( tr("Run the classic SEG-Y loader") );
    execoldbut->attach( leftTo, okbut );
    execoldbut->attach( leftBorder );
    uiMenu* mnu = new uiMenu;
    mnu->insertAction( new uiAction("Import",impcb) );
    mnu->insertAction( new uiAction("Link",linkcb) );
    execoldbut->setMenu( mnu );
}


void uiSEGYReadStarter::firstSel( CallBacker* )
{
    timer_->tick.remove( mCB(this,uiSEGYReadStarter,firstSel));

    uiFileDialog dlg( this, uiFileDialog::ExistingFile, 0,
	    uiSEGYFileSpec::fileFilter(),
	    tr("Select (one of) the SEG-Y file(s)") );
    if ( mForSurvSetup )
	dlg.setDirectory( GetBaseDataDir() );
    else
	dlg.setDirectory( GetDataDir() );

    if ( !dlg.go() )
	done();
    else
    {
	inpfld_->setFileName( dlg.fileName() );
	forceRescan( KeepNone );
    }
}


void uiSEGYReadStarter::typChg( CallBacker* )
{
    const SEGY::ImpType& imptyp = impType();

    if ( imptyp.is2D() )
	loaddef_.icvsxytype_ = SEGY::FileReadOpts::XYOnly;

    infofld_->setImpTypIdx( imptyp.tidx_, false );
    detectrev0flds_ = true;
    forceRescan( KeepBasic );
    setToolStates();
}


void uiSEGYReadStarter::inpChg( CallBacker* )
{
    detectrev0flds_ = true;
    handleNewInputSpec( KeepNone );
    setToolStates();
}


void uiSEGYReadStarter::fullScanReq( CallBacker* cb )
{
    forceRescan( KeepAll, true );
}


void uiSEGYReadStarter::runClassic( bool imp )
{
    const Seis::GeomType gt = impType().geomType();
    uiSEGYRead::Setup su( mForSurvSetup ? uiSEGYRead::SurvSetup
				 : (imp ? uiSEGYRead::Import
					: uiSEGYRead::DirectDef) );
    if ( imptypeFixed() )
	{ su.geoms_.erase(); su.geoms_ += gt; }
    else if ( !imp )
	su.geoms_ -= Seis::Line;

    commit( true );
    const FullSpec fullspec = fullSpec();
    IOPar iop; fullspec.fillPar( iop );
    classicrdr_ = new uiSEGYRead( this, su, &iop );
    if ( !timer_ )
	timer_ = new Timer( "uiSEGYReadStarter timer" );
    timer_->tick.notify( mCB(this,uiSEGYReadStarter,initClassic));
    timer_->start( 1, true );
}


void uiSEGYReadStarter::editFile( CallBacker* )
{
    const BufferString fnm( filespec_.fileName(0) );
    if ( !File::exists(fnm) )
	return;

    uiSEGYFileManip dlg( this, fnm );
    if ( dlg.go() )
    {
	inpfld_->setFileName( dlg.fileName() );
	forceRescan( KeepNone );
    }
}


void uiSEGYReadStarter::forceRescan( LoadDefChgType ct, bool fullscan )
{
    userfilename_.setEmpty();
    handleNewInputSpec( ct, fullscan );
}


void uiSEGYReadStarter::handleNewInputSpec( LoadDefChgType ct, bool fullscan )
{
    const BufferString newusrfnm( inpfld_->fileName() );
    if ( newusrfnm.isEmpty() )
	{ clearDisplay(); return; }

    if ( fullscan || newusrfnm != userfilename_ )
    {
	userfilename_ = newusrfnm;
	execNewScan( ct, fullscan );
    }
}


void uiSEGYReadStarter::examineCB( CallBacker* )
{
    if ( !commit() )
	return;

    MouseCursorChanger chgr( MouseCursor::Wait );
    uiSEGYExamine::Setup su( examineNrTraces() );
    su.fs_ = filespec_; su.fp_ = filepars_;
    uiSEGYExamine* dlg = new uiSEGYExamine( this, su );
    dlg->setDeleteOnClose( true );
    dlg->go();
}


void uiSEGYReadStarter::usePar( const IOPar& iop )
{
    if ( typfld_ )
	typfld_->usePar( iop );

    int nrtrcs = examineNrTraces();
    iop.get( uiSEGYExamine::Setup::sKeyNrTrcs, nrtrcs );
    examinenrtrcsfld_->setValue( nrtrcs );

    float clipratio = ratioClip();
    iop.get( sKeyClipRatio, clipratio );
    if ( mIsUdf(clipratio) || clipratio < 0.f )
	clipratio = 0.f;
    else if ( clipratio > 1.f )
	clipratio = 1.f;
    clipfld_->setValue( clipratio*100.f );

    bool inczeros = incZeros();
    iop.getYN( sKeyIncludeZeros, inczeros );
    inc0sbox_->setChecked( inczeros );

    loaddef_.usePar( iop );
    forceRescan( KeepAll, false );
    infofld_->useLoadDef();
    updateICvsXYButtons();
}


void uiSEGYReadStarter::fillPar( IOPar& iop ) const
{
    const_cast<uiSEGYReadStarter*>(this)->commit( true );
    const FullSpec fullspec = fullSpec();
    fullspec.fillPar( iop );

    iop.set( FilePars::sKeyRevision(), loaddef_.revision_ );
    impType().fillPar( iop );

    iop.set( uiSEGYExamine::Setup::sKeyNrTrcs, examineNrTraces() );
    iop.set( sKeyClipRatio, ratioClip() );
    iop.setYN( sKeyIncludeZeros, incZeros() );
}


void uiSEGYReadStarter::readParsCB( CallBacker* )
{
    uiSEGYReadImpParsDlg dlg( this, lastparname_ );
    if ( dlg.go() )
    {
	usePar( *dlg.pars() );
	lastparname_ = dlg.parName();
    }
}


void uiSEGYReadStarter::writeParsCB( CallBacker* )
{
    IOPar iop; fillPar( iop );
    uiSEGYStoreImpParsDlg dlg( this, iop, lastparname_ );
    if ( dlg.go() )
	lastparname_ = dlg.parName();
}


void uiSEGYReadStarter::icxyCB( CallBacker* cb )
{
    if ( !useicbut_ ) return;

    const bool iccl = cb == useicbut_;
    const bool useic = iccl ? useicbut_->isChecked() : !usexybut_->isChecked();
    loaddef_.icvsxytype_ = useic ? SEGY::FileReadOpts::ICOnly
				 : SEGY::FileReadOpts::XYOnly;
    if ( iccl )
    {
	NotifyStopper ns( usexybut_->activated );
	usexybut_->setChecked( !useic );
    }
    else
    {
	NotifyStopper ns( useicbut_->activated );
	useicbut_->setChecked( !useic );
    }

    forceRescan();
}


void uiSEGYReadStarter::updateCoordScale()
{
    loaddef_.coordscale_ = mUdf(float);
    if ( !coordscalefld_ )
	return;

    BufferString edtxt = coordscalefld_->text();
    edtxt.trimBlanks();
    if ( edtxt.isEmpty() )
	return;

    loaddef_.coordscale_ = edtxt.toFloat();
}


void uiSEGYReadStarter::coordscaleChg( CallBacker* cb )
{
    updateCoordScale();
    forceRescan();
}


void uiSEGYReadStarter::updateICvsXYButtons()
{
    if ( !useicbut_ ) return;

    const bool useic = loaddef_.icvsxytype_ != SEGY::FileReadOpts::XYOnly;
    NotifyStopper nsic( useicbut_->activated );
    useicbut_->setChecked( useic );
    NotifyStopper nsxy( usexybut_->activated );
    usexybut_->setChecked( !useic );
}


void uiSEGYReadStarter::updateAmplDisplay( CallBacker* )
{
    if ( !ampldisp_ )
	return;

    int nrvals = (int)clipsampler_.nrVals();
    if ( nrvals < 1 )
	{ ampldisp_->setEmpty(); return; }

    const float* csvals = clipsampler_.vals();
    float clipratio = ratioClip();
    const bool useclip = clipratio > 0.0005;
    const bool rm0 = !incZeros();
    if ( !useclip && !rm0 )
	{ ampldisp_->setData( csvals, nrvals ); return; }

    TypeSet<float> vals;
    if ( !rm0 )
	vals.append( csvals, nrvals );
    else
    {
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    const float val = csvals[idx];
	    if ( val != 0.f )
		vals += val;
	}
	nrvals = vals.size();
    }

    if ( nrvals < 1 )
	{ ampldisp_->setEmpty(); return; }

    if ( useclip )
    {
	DataClipper clipper;
	clipper.putData( vals.arr(), nrvals );
	Interval<float> rg;
	clipper.calculateRange( clipratio, rg );
	TypeSet<float> oldvals( vals );
	vals.setEmpty();
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    const float val = oldvals[idx];
	    if ( rg.includes(val,false) )
		vals += val;
	}

	nrvals = vals.size();
	if ( nrvals < 1 )
	    { ampldisp_->setEmpty(); return; }
    }

    ampldisp_->setData( vals.arr(), nrvals );
}


void uiSEGYReadStarter::updateSurvMap()
{
    survinfook_ = false;
    if ( !survmap_ )
	return;

    survinfo_ = survmap_->getEmptySurvInfo();
    survinfo_->setName( "No valid scan available" );
    const char* stbarmsg = "";
    if ( scaninfos_ && !scaninfos_->isEmpty() )
    {
	Coord crd[3]; TrcKeyZSampling cs;
	stbarmsg = scaninfos_->piDetector().getSurvInfo( cs.hsamp_, crd );
	if ( !stbarmsg )
	{
	    cs.zsamp_ = loaddef_.getZRange();
	    survinfo_->setRange( cs, false );
	    BinID bid[2];
	    bid[0].inl() = cs.hsamp_.start_.inl();
	    bid[0].crl() = cs.hsamp_.start_.crl();
	    bid[1].inl() = cs.hsamp_.stop_.inl();
	    bid[1].crl() = cs.hsamp_.stop_.crl();
	    stbarmsg = survinfo_->set3Pts( crd, bid, cs.hsamp_.stop_.crl() );
	}
	if ( stbarmsg )
	    survinfo_->setName( "<Inadequate data>" );
	else
	{
	    survinfook_ = true;
	    survinfo_->setName( "Resulting survey setup" );
	}
    }

    toStatusBar( stbarmsg );
    survmap_->setSurveyInfo( survinfo_ );
}


bool uiSEGYReadStarter::getInfo4SI( TrcKeyZSampling& cs, Coord crd[3] ) const
{
    if ( !survinfook_ )
	return false;

    BinID bids[2]; int xline;
    survinfo_->get3Pts( crd, bids, xline );
    cs = survinfo_->sampling( false );
    return true;
}


void uiSEGYReadStarter::initClassic( CallBacker* )
{
    if ( !classicrdr_ )
	return;

    classicrdr_->raiseCurrent();
}


#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiSEGYReadStarter::getFileSpec()
{
    if ( userfilename_.isEmpty() )
	return false;

    filespec_.setEmpty();
    if ( !userfilename_.find('*') )
    {
	if ( !getExistingFileName(userfilename_) )
	    return false;
	filespec_.setFileName( userfilename_ );
    }
    else
    {
	FilePath fp( userfilename_ );
	if ( !fp.isAbsolute() )
	    mErrRet(
	    tr("Please specify the absolute file name when using a wildcard.") )

	DirList dl( fp.pathOnly(), DirList::FilesOnly, fp.fileName() );
	for ( int idx=0; idx<dl.size(); idx++ )
	    filespec_.fnames_.add( dl.fullPath(idx) );

	if ( filespec_.isEmpty() )
	    mErrRet( tr("No file names matching your wildcard(s).") )

	filespec_.setUsrStr( userfilename_ );
    }

    return true;
}


bool uiSEGYReadStarter::getExistingFileName( BufferString& fnm, bool emiterr )
{
    FilePath fp( fnm );
    if ( fp.isAbsolute() )
    {
	if ( !File::exists(fnm) )
	{
	    if ( emiterr )
		uiMSG().error( uiString(
			    "SEG-Y file does not exist:\n%1").arg(fnm) );
	    return false;
	}
    }
    else
    {
	FilePath newfp( GetDataDir(), fnm );
	if ( !File::exists(newfp.fullPath()) )
	{
	    newfp.set( GetDataDir() ).add( "Seismics" );
	    if ( !File::exists(newfp.fullPath()) )
	    {
		if ( emiterr )
		    uiMSG().error(
			    tr("SEG-Y file not found in survey directory") );
		return false;
	    }
	}
	fnm = newfp.fullPath();
    }

    return true;
}


#define mErrRetFileName(s) \
{ \
    if ( isfirst ) \
	uiMSG().error( uiString(s).arg(strm.fileName()) ); \
    return false; \
}

bool uiSEGYReadStarter::scanFile( const char* fnm, LoadDefChgType ct,
				  TaskRunner* trunner )
{
    const bool isfirst = scaninfos_->isEmpty();
    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRetFileName( "Cannot open file: %1" )

    SEGY::TxtHeader txthdr; SEGY::BinHeader binhdr;
    strm.getBin( txthdr.txt_, SegyTxtHeaderLength );
    if ( !strm.isOK() )
	mErrRetFileName( "File:\n%1\nhas no textual header" )
    strm.getBin( binhdr.buf(), SegyBinHeaderLength );
    if ( strm.isBad() )
	mErrRetFileName( "File:\n%1\nhas no binary header" )

    SEGY::ScanInfo& si = scaninfos_->add( fnm );
    SEGY::BasicFileInfo& bfi = si.basicInfo();

    const bool useloaddef = ct != KeepNone;
    if ( !useloaddef )
	binhdr.guessIsSwapped();
    bfi.hdrsswapped_ = bfi.dataswapped_ = binhdr.isSwapped();
    if ( (useloaddef && loaddef_.hdrsswapped_)
	|| (!useloaddef && bfi.hdrsswapped_) )
	binhdr.unSwap();
    if ( !binhdr.isRev0() )
	binhdr.skipRev1Stanzas( strm );
    scaninfos_->setInFeet( binhdr.isInFeet() );

    bfi.ns_ = binhdr.nrSamples();
    if ( bfi.ns_ < 1 || bfi.ns_ > mMaxReasonableNS )
	bfi.ns_ = -1;
    bfi.revision_ = binhdr.revision();
    short fmt = binhdr.format();
    if ( fmt != 1 && fmt != 2 && fmt != 3 && fmt != 5 && fmt != 8 )
	fmt = 1;
    bfi.format_ = fmt;
    if ( !completeFileInfo(strm,bfi,isfirst) )
	{ scaninfos_->removeLast(); return false; }

    if ( isfirst && ct == KeepNone )
	static_cast<SEGY::BasicFileInfo&>(loaddef_) = bfi;

    si.getFromSEGYBody( strm, loaddef_, mForSurvSetup, clipsampler_, trunner );
    return true;
}


#define mErrRetResetStream(str) { \
    strm.setPosition( firsttrcpos ); \
    if ( emiterr ) \
	mErrRetFileName( str ) \
    return false; }

bool uiSEGYReadStarter::completeFileInfo( od_istream& strm,
				      SEGY::BasicFileInfo& bfi, bool emiterr )
{
    const bool isfirst = true; // for mErrRetFileName
    const od_stream::Pos firsttrcpos = strm.position();

    PtrMan<SEGY::TrcHeader> thdr = loaddef_.getTrcHdr( strm );
    if ( !thdr )
	mErrRetResetStream( "File:\n%1\nNo traces found" )

    if ( bfi.ns_ < 1 )
    {
	bfi.ns_ = (int)thdr->nrSamples();
	if ( bfi.ns_ > mMaxReasonableNS )
	    mErrRetResetStream(
		    "File:\n%1\nNo proper 'number of samples per trace' found" )
    }

    if ( mIsUdf(bfi.sampling_.step) )
    {
	SeisTrcInfo ti; thdr->fill( ti, 1.0f );
	bfi.sampling_ = ti.sampling;
    }

    strm.setPosition( firsttrcpos );
    return true;
}


void uiSEGYReadStarter::displayScanResults()
{
    if ( !scaninfos_ || scaninfos_->isEmpty() )
	{ clearDisplay(); return; }

    setToolStates();
    if ( ampldisp_ )
	updateAmplDisplay( 0 );

    infofld_->setScanInfo( *scaninfos_ );
    if ( mForSurvSetup )
	updateSurvMap();
}


bool uiSEGYReadStarter::commit( bool permissive )
{
    if ( !permissive && filespec_.isEmpty() )
    {
	uiMSG().error( uiStrings::phrSelect(uiStrings::sInputFile()) );
	return false;
    }

    loaddef_.getFilePars( filepars_ );

    delete filereadopts_;
    filereadopts_ = new FileReadOpts( impType().geomType() );
    loaddef_.getFileReadOpts( *filereadopts_ );

    updateCoordScale();
    return true;
}


bool uiSEGYReadStarter::acceptOK( CallBacker* )
{
    if ( !commit(false) )
	return false;

    if ( mForSurvSetup )
    {
	if ( !survinfook_ )
	    mErrRet( tr("No valid survey setup found" ) )
	return true;
    }

    const FullSpec fullspec = fullSpec();
    uiSEGYReadFinisher dlg( this, fullspec, userfilename_ );
    dlg.go();

    return false;
}

