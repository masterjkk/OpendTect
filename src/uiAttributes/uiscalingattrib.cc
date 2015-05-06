/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          December 2004
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "uiscalingattrib.h"
#include "scalingattrib.h"

#include "attribengman.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "attribprocessor.h"
#include "attribfactory.h"
#include "binidvalset.h"
#include "cubesampling.h"
#include "datapackbase.h"
#include "ioman.h"
#include "ioobj.h"
#include "linekey.h"
#include "flatposdata.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "seisioobjinfo.h"
#include "seisbufadapters.h"
#include "survinfo.h"
#include "volstatsattrib.h"

#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uigainanalysisdlg.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiselsurvranges.h"
#include "uitable.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

using namespace Attrib;

static const int initnrrows = 5;
static const int startcol = 0;
static const int stopcol = 1;
static const int factcol = 2;


static const char* statstypestr[] =
{
    "RMS",
    "Mean",
    "Max",
    "User-defined",
    "Detrend",
    0
};


static const char* scalingtypestr[] =
{
    "Z^n",
    "Window",
    "AGC",
    "Squeeze",
    "Gain Correction",
    0
};

mInitAttribUI(uiScalingAttrib,Scaling,"Scaling",sKeyBasicGrp())


uiScalingAttrib::uiScalingAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mODHelpKey(mScalingAttribHelpID) )
	, parent_(p)
{
    inpfld = createInpFld( is2d );

    typefld = new uiGenInput( this, "Type", StringListInpSpec(scalingtypestr) );
    typefld->valuechanged.notify( mCB(this,uiScalingAttrib,typeSel) );
    typefld->attach( alignedBelow, inpfld );

    nfld = new uiGenInput( this, "n", FloatInpSpec() );
    nfld->attach( alignedBelow, typefld );

    sqrgfld = new uiGenInput( this, "Value range (empty=unlimited)",
	    			FloatInpIntervalSpec() );
    sqrgfld->attach( alignedBelow, typefld );
    squrgfld = new uiGenInput( this, "Untouched range (empty=all)",
	    			FloatInpIntervalSpec() );
    squrgfld->attach( alignedBelow, sqrgfld );

    statsfld = new uiGenInput( this, "Basis", StringListInpSpec(statstypestr) );
    statsfld->attach( alignedBelow, typefld );
    statsfld->valuechanged.notify( mCB(this,uiScalingAttrib,statsSel) );

    table = new uiTable( this, uiTable::Setup().rowdesc("Gate")
					       .rowgrow(true)
					       .defrowlbl("")
					       .fillcol(true)
					       .maxrowhgt(1)
					       .selmode(uiTable::Multi),
		         "Define Gate limits" );

    BufferString lblstart = "Start "; lblstart += SI().getZUnitString();
    BufferString lblstop = "Stop "; lblstop += SI().getZUnitString();
    const char* collbls[] = { lblstart.buf(), lblstop.buf(), "Scale value", 0 };
    table->setColumnLabels( collbls );
    table->setNrRows( initnrrows );
    table->setColumnStretchable( startcol, true );
    table->setColumnStretchable( stopcol, true );
    table->attach( alignedBelow, statsfld );
    table->setStretch( 2, 0 );
    table->setToolTip( "Right-click to add, insert or remove a gate" );

    // for AGC
    BufferString label = "Window width ";
    label += SI().getZUnitString( true );
    // TODO: make default value dependent on survey type
    windowfld = new uiGenInput( this, label.buf(), FloatInpSpec(200) );
    windowfld->attach( alignedBelow, typefld );

    lowenergymute = new uiGenInput( this, "Low energy mute (%)",
	    			    FloatInpSpec() );
    lowenergymute->setValue( 0 );
    lowenergymute->attach( alignedBelow, windowfld );

    // for Gain Correction
    analysebut_ = new uiPushButton( this, "Analyse",
	    			    mCB(this,uiScalingAttrib,analyseCB), false);
    analysebut_->attach( alignedBelow, typefld );

    typeSel(0);
    statsSel(0);

    setHAlignObj( inpfld );
}


void uiScalingAttrib::typeSel( CallBacker* )
{
    const int typeval = typefld->getIntValue();
    nfld->display( typeval==0 );

    statsfld->display( typeval==1 );
    table->display( typeval==1 );

    windowfld->display( typeval==2);
    lowenergymute->display( typeval==2);

    sqrgfld->display( typeval==3 );
    squrgfld->display( typeval==3 );
    
    analysebut_->display( typeval==4 );
}


void uiScalingAttrib::statsSel( CallBacker* )
{
    const int statstype = statsfld->getIntValue();
    table->hideColumn( 2, statstype!=3 );
}


bool uiScalingAttrib::setParameters( const Desc& desc )
{
    if ( desc.attribName() != Scaling::attribName() )
	return false;

    mIfGetEnum( Scaling::scalingTypeStr(), scalingtype,
	        typefld->setValue(scalingtype) );
    mIfGetFloat( Scaling::powervalStr(), powerval, nfld->setValue(powerval) );
    mIfGetEnum( Scaling::statsTypeStr(), statstype, 
	    			statsfld->setValue(statstype) );
    mIfGetFloat( Scaling::widthStr(), wndwidthval, 
	    	 windowfld->setValue(wndwidthval) );
    mIfGetFloat( Scaling::mutefractionStr(), mutefactor,
				lowenergymute->setValue(mutefactor*100));    

    const Attrib::ValParam* vp = desc.getValParam(Scaling::sqrangeStr());
    if ( vp )
	sqrgfld->setValue( Interval<float>(vp->getfValue(0),vp->getfValue(1)) );
    vp = desc.getValParam(Scaling::squntouchedStr());
    if ( vp )
	squrgfld->setValue( Interval<float>(vp->getfValue(0),vp->getfValue(1)));

    int nrtgs = 0;
    if ( desc.getParam(Scaling::gateStr()) )
    {
	mDescGetConstParamGroup(ZGateParam,gateset,desc,Scaling::gateStr())
	nrtgs = gateset->size();
    }
    
    table->clearTable();
    while ( nrtgs > table->nrRows() )
	table->insertRows( 0, 1 );
    while ( nrtgs < table->nrRows() && table->nrRows() > initnrrows )
	table->removeRow( 0 );
    if ( desc.getParam(Scaling::gateStr()) )
    {
	mDescGetConstParamGroup(ZGateParam,gateset,desc,Scaling::gateStr());
	
	zvals_.erase();
	for ( int idx=0; idx<gateset->size(); idx++ )
	{
	    const ValParam& param = (ValParam&)(*gateset)[idx];
	    
	    if ( typefld->getIntValue() == 4 )
	    {
		if ( idx==0 )
		    zvals_.addIfNew( param.getfValue(0) );
		zvals_.addIfNew( param.getfValue(1) );
	    }
	    else
	    {
		table->setValue( RowCol(idx,startcol), param.getfValue(0) );
		table->setValue( RowCol(idx,stopcol), param.getfValue(1) );
	    }
	}
    }
    if ( desc.getParam(Scaling::factorStr()) )
    {
	mDescGetConstParamGroup(ValParam,factorset,desc,Scaling::factorStr());
	scalefactors_.erase();

	for ( int idx=0; idx< factorset->size(); idx++ )
	{
	    const ValParam& param = (ValParam&)(*factorset)[idx];
	    if ( typefld->getIntValue() == 4 )
		scalefactors_ += param.getfValue(0);
	    else
		table->setValue( RowCol(idx,factcol), param.getfValue(0) );
	}
    }

    typeSel(0);
    statsSel(0);
    return true;
}


bool uiScalingAttrib::setInput( const Desc& desc )
{
    putInp( inpfld, desc, 0 );
    return true;
}


bool uiScalingAttrib::getParameters( Desc& desc )
{
    if ( desc.attribName() != Scaling::attribName() )
	return false;

    mSetEnum( Scaling::scalingTypeStr(), typefld->getIntValue() );
    mSetFloat( Scaling::powervalStr(), nfld->getfValue() );
    mSetEnum( Scaling::statsTypeStr(), statsfld->getIntValue() );
    mSetFloat( Scaling::widthStr(), windowfld->getfValue() );
    mSetFloat( Scaling::mutefractionStr(), lowenergymute->getfValue()/100 );
    mSetFloatInterval( Scaling::sqrangeStr(), sqrgfld->getFInterval() );
    mSetFloatInterval( Scaling::squntouchedStr(), squrgfld->getFInterval() );

    TypeSet<ZGate> tgs;
    TypeSet<float> factors;

    for ( int idx=0; idx<table->nrRows(); idx++ )
    {
	int start = table->getIntValue( RowCol(idx,startcol) );
	int stop = table->getIntValue( RowCol(idx,stopcol) );
	if ( mIsUdf(start) && mIsUdf(stop) ) continue;
	
	tgs += ZGate( mCast(float,start), mCast(float,stop) );

	if ( statsfld->getIntValue() == 3 )
	{
	    const char* factstr = table->text( RowCol(idx,factcol) );
	    factors += factstr && *factstr ? toFloat(factstr) : 1;
	}
    }

    CubeSampling cs;
    if ( typefld->getIntValue() == 4 )
    {
	tgs.erase();
	factors.erase();
	for ( int idx=0; idx<zvals_.size()-1; idx++ )
	{
	    float zstart = zvals_[idx];
	    float zstop = zvals_[idx+1];
	    
	    tgs += ZGate( zstart, zstop );
	    if ( scalefactors_.validIdx(idx) )
		factors += scalefactors_[idx];
	}
    }

    mDescGetParamGroup(ZGateParam,gateset,desc,Scaling::gateStr())
    gateset->setSize( tgs.size() );
    for ( int idx=0; idx<tgs.size(); idx++ )
    {
	ZGateParam& zgparam = (ZGateParam&)(*gateset)[idx];
	zgparam.setValue( tgs[idx] );
    }

    mDescGetParamGroup(ValParam,factorset,desc,Scaling::factorStr())
    factorset->setSize( factors.size() );
    for ( int idx=0; idx<factors.size(); idx++ )
    {
	ValParam& valparam = (ValParam&)(*factorset)[idx];
	valparam.setValue(factors[idx] );
    }
    
    return true;
}


bool uiScalingAttrib::getInput( Desc& desc )
{
    fillInp( inpfld, desc, 0 );
    return true;
}


void uiScalingAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    const int typeval = typefld->getIntValue();
    if ( typeval == 0 )
	params += EvalParam( "n", Scaling::powervalStr() );
    else if ( typeval == 2 )
    {
	params += EvalParam( "Width", Scaling::widthStr() );
	params += EvalParam( "Mute fraction", Scaling::mutefractionStr() );
    }
    else if ( typeval == 3 )
	params += EvalParam( "Untouched range", Scaling::squntouchedStr() );
}


bool uiScalingAttrib::areUIParsOK()
{
    const int typeval = typefld->getIntValue();
    if ( typeval == 3 )
    {
	if ( sqrgfld->isUndef(0) && sqrgfld->isUndef(1) )
	{
	    errmsg_ = "Please fill in at least one value range limit\n";
	    return false;
	}
    }

    return true;
}


class uiSelectPositionDlg : public uiDialog
{
public:

    enum DataType		{ Stored2D=0, Stored3D=1, DataPack2D=2,
				  DataPack3D=3 };

uiSelectPositionDlg( uiParent* p,const DataPack::FullID& dpfid )
    : uiDialog(p,uiDialog::Setup("Select data","For gain analysis",mNoHelpKey))
    , attribnm_(0)
    , linesfld_(0)
    , subvolfld_(0)
    , dpfid_(dpfid)
{
    const int dpmid = dpfid.ID( 0 );
    if ( dpmid!=DataPackMgr::FlatID() && dpmid!=DataPackMgr::CubeID() )
    {
	pErrMsg( "Only Flat & Cube DataPacks supported" );
	return;
    }

    const bool is2d = dpmid==DataPackMgr::FlatID();
    createSelFields( is2d ? DataPack2D : DataPack3D );
}

uiSelectPositionDlg( uiParent* p,const MultiID& mid, bool is2d,const char* anm )
    : uiDialog(p,uiDialog::Setup("Select data","For gain analysis",mNoHelpKey))
    , attribnm_(anm)
    , linesfld_(0)
    , subvolfld_(0)
    , mid_(mid)
{
    createSelFields( is2d ? Stored2D : Stored3D );
}

int nrTrcs()
{ return nrtrcfld_->getIntValue(); }

LineKey lineKey() const
{ return LineKey( linesfld_->box()->text(), attribnm_ ); }

CubeSampling subVol() const
{
    CubeSampling cs;
    if ( subvolfld_ )
	cs = subvolfld_->getSampling();
    return cs;
}


protected:

void createSelFields( DataType type )
{
    IntInpSpec nrtrcinpspec( 50 );
    nrtrcfld_ = new uiGenInput( this, "Nr of Traces for Examination",
				nrtrcinpspec );
    
    if ( type==uiSelectPositionDlg::Stored2D )
    {
	SeisIOObjInfo objinfo( mid_ );
	BufferStringSet linenames;
	objinfo.getLineNames( linenames );
	linesfld_ = new uiLabeledComboBox( this, "Gain Analyisis on line:" );
	for ( int idx=0; idx<linenames.size(); idx++ )
	    linesfld_->box()->addItem( linenames.get(idx) );
	
	linesfld_->attach( alignedBelow, nrtrcfld_ );
    }
    else if ( type==uiSelectPositionDlg::DataPack2D )
    {
	DataPack* dp = DPM( dpfid_.ID(0)).obtain( dpfid_.ID(1) );
	mDynamicCastGet(FlatDataPack*,fdp,dp);
	Interval<int> trcrglimits( 0, fdp->size(true) );
	nrtrcinpspec.setLimits( trcrglimits );
	nrtrcfld_->setValue( fdp->size(true) );
	DPM( dpfid_.ID(0)).release( dpfid_.ID(1) );
    }
    else
    {
	subvolfld_ = new uiSelSubvol( this, false );
	subvolfld_->attach( alignedBelow, nrtrcfld_ );
	CubeSampling cs;
	if ( type==uiSelectPositionDlg::DataPack3D )
	{
	    DataPack* dp = DPM( dpfid_.ID(0)).obtain( dpfid_.ID(1) );
	    mDynamicCastGet(CubeDataPack*,cdp,dp);
	    cs = cdp->sampling();
	    DPM( dpfid_.ID(0)).release( dpfid_.ID(1) );
	}
	else if ( type==uiSelectPositionDlg::Stored3D )
	{
	    SeisIOObjInfo seisinfo( mid_ );
	    seisinfo.getRanges( cs );
	}

	subvolfld_->setSampling( cs );
    }
}


bool acceptOK( CallBacker* )
{
    if ( !nrtrcfld_->dataInpSpec()->isInsideLimits() )
    {
	uiMSG().error("Number of traces specified is more than in the dataset");
	return false;
    }

    return true;
}

    BufferString	attribnm_;
    DataPack::FullID	dpfid_;
    MultiID		mid_;
    
    uiGenInput*		nrtrcfld_;
    uiSelSubvol*	subvolfld_;
    uiLabeledComboBox*	linesfld_;
};


void uiScalingAttrib::analyseCB( CallBacker* )
{
    Attrib::Desc* inpdesc = !ads_ ? getInputDescFromDP( inpfld )
				  : ads_->getDesc( inpfld->attribID() );
    Attrib::Desc* voldesc = PF().createDescCopy( VolStats::attribName() );
    if ( !inpdesc || !voldesc )
	return;

    PtrMan<Attrib::DescSet> descset =
	ads_ ? ads_->optimizeClone( inpfld->attribID() )
	     : new DescSet( is2D() );
    if ( !descset )
	return;
    
    Attrib::Desc& desc = *voldesc;
    desc.setInput( 0, inpdesc );
    Interval<float> timegate(-28,28);
    mSetFloatInterval( VolStats::gateStr(), timegate );
    mSetBinID( VolStats::stepoutStr(), BinID(descset->is2D() ? 0 : 5,5) );
    mSetEnum( VolStats::shapeStr(), 0 );
    mSetBool( VolStats::steeringStr(), false );
    mSetInt( VolStats::nrtrcsStr(), 1 );
    mSetInt( VolStats::optstackstepStr(), 8 );
    desc.selectOutput( 8 );
    desc.setUserRef( "Examine-GainCorrection" );
    desc.updateParams();

    inpdesc->setDescSet( descset );
    descset->addDesc( inpdesc );
    Attrib::DescID attribid = descset->addDesc( voldesc );
    PtrMan<Attrib::EngineMan> aem = new Attrib::EngineMan;
    TypeSet<SelSpec> attribspecs;
    SelSpec sp( 0, attribid );
    sp.set( desc );
    attribspecs += sp;
    aem->setAttribSet( descset );
    aem->setAttribSpecs( attribspecs );
    
    CubeSampling cs( false );
    const bool isinpindp = dpfids_.size();
    TypeSet<BinID> bidset;
    int nrtrcs = 0;
    if ( !isinpindp )
    {
	LineKey lk( inpdesc->getStoredID(true) );
	PtrMan<IOObj> ioobj = IOM().get( MultiID(lk.lineName()) );
	if ( !ioobj )
	    return uiMSG().error( "Select a valid input" );

	uiSelectPositionDlg subseldlg( this, ioobj->key(), is2D(),
				       lk.attrName() );
	if ( !subseldlg.go() )
	    return;

	if ( is2D() )
	{
	    SeisIOObjInfo seisinfo( ioobj );
	    StepInterval<int> trcrg;
	    StepInterval<float> zrg;
	    Pos::GeomID geomid = Survey::GM().getGeomID( 
					    subseldlg.lineKey().lineName() );
	    seisinfo.getRanges( geomid, trcrg, zrg );
	    cs.hrg.setCrlRange( trcrg );
	    cs.hrg.setInlRange( Interval<int>(0,0) );
	    cs.zrg = zrg;
	    aem->setLineKey( subseldlg.lineKey() );
	}
	else
	    cs = subseldlg.subVol();
	nrtrcs = subseldlg.nrTrcs();
    }
    else
    {
	DataPack::FullID dpfid;
	getInputDPID( inpfld, dpfid );
	uiSelectPositionDlg subseldlg( this, dpfid );
	if ( !subseldlg.go() )
	    return;
	if ( dpfid.ID(0)==DataPackMgr::CubeID() )
	    cs = subseldlg.subVol();
	else
	{
	    DataPack* dp = DPM(dpfid.ID(0)).obtain( dpfid.ID(1) );
	    mDynamicCastGet(FlatDataPack*,fdp,dp);
	    if ( !fdp )
	    {
		pErrMsg( "No FlatDataPack found" );
		return;
	    }

	    StepInterval<double> dtrcrg = fdp->posData().range( true );
	    Interval<int> trcrg( mCast(int,dtrcrg.start),
				 mCast(int,dtrcrg.stop) );
	    cs.hrg.setCrlRange( trcrg );
	    cs.hrg.setInlRange( Interval<int>(0,0) );

	    StepInterval<double> dzrg = fdp->posData().range( false );
	    StepInterval<float> zrg( mCast(float,dzrg.start),
				     mCast(float,dzrg.stop),
				     mCast(float,dzrg.step) );
	    cs.zrg = zrg;
	    DPM(dpfid.ID(0)).release( dpfid.ID(1) );
	}
	nrtrcs = subseldlg.nrTrcs();
    }

    if ( nrtrcs <= 0 )
	return uiMSG().error( "Number of traces cannot be zero or negative" );

    cs.hrg.getRandomSet( nrtrcs, bidset );
    aem->setCubeSampling( cs );

    BinIDValueSet bidvals( 0, false );
    for ( int idx=0; idx<bidset.size(); idx++ )
       bidvals.add( bidset[idx] );	

    uiString errmsg;
    SeisTrcBuf bufs( true );
    Interval<float> zrg( cs.zrg );
    PtrMan<Processor> proc =
	aem->createTrcSelOutput( errmsg, bidvals, bufs, 0, &zrg );

    if ( !proc )
    {
	uiMSG().error( errmsg );
	return;
    }

    uiTaskRunner dlg( parent_ );
    if ( !TaskRunner::execute( &dlg, *proc ) )
	return;

    uiGainAnalysisDlg analdlg( this, bufs, zvals_, scalefactors_);
    analdlg.go();
}

