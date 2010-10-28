/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelllogdisplay.cc";



#include "uiwelllogdisplay.h"

#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "coltabsequence.h"
#include "mouseevent.h"
#include "dataclipper.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "welllog.h"
#include "wellmarker.h"
#include "welld2tmodel.h"

#include <iostream>


#define mDefZPos(zpos) \
        if ( zdata_.zistime_ && zdata_.d2tm_ )\
        zpos = zdata_.d2tm_->getTime( zpos )*1000;\
    if ( zdata_.dispzinft_ && !zdata_.zistime_)\
        zpos *= mToFeetFactor;

#define mDefZPosInLoop(val) \
        float zpos = val;\
    mDefZPos(zpos)\
    if ( !zdata_.zrg_.includes( zpos ) )\
        continue;

uiWellLogDisplay::LogData::LogData( uiGraphicsScene& scn, bool isfirst,
				    const uiWellLogDisplay::Setup& s )
    : wl_(0)
    , unitmeas_(0)
    , xax_(&scn,uiAxisHandler::Setup(isfirst?uiRect::Top:uiRect::Bottom)
    .border(s.border_)
    .noborderspace(s.noborder_)
    .ticsz(s.axisticsz_))
    , yax_(&scn,uiAxisHandler::Setup(isfirst?uiRect::Left:uiRect::Right)
    .border(s.border_)
    .noborderspace(s.noborder_))
    , xrev_(false)
    , zrg_(mUdf(float),0)
    , valrg_(mUdf(float),0)
    , curvenmitm_(0)
{
    if ( !isfirst )
	yax_.setup().nogridline(true);
}


uiWellLogDisplay::uiWellLogDisplay( uiParent* p, const Setup& su )
    : uiWellDahDisplay(p,"Well Log display viewer")
    , setup_(su)
    , ld1_(scene(),true,su)
    , ld2_(scene(),false,su)
{
    reSize.notify( mCB(this,uiWellLogDisplay,reSized) );
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );

    finaliseDone.notify( mCB(this,uiWellLogDisplay,init) );
}


uiWellLogDisplay::~uiWellLogDisplay()
{
}


void uiWellLogDisplay::gatherInfo()
{
    setAxisRelations();
    gatherInfo( true );
    gatherInfo( false );

    if ( mIsUdf(zdata_.zrg_.start) && ld1_.wl_ )
    {
	zdata_.zrg_ = ld1_.zrg_;
	if ( ld2_.wl_ )
	    zdata_.zrg_.include( ld2_.zrg_ );
    }
    setAxisRanges( true );
    setAxisRanges( false );

    ld1_.yax_.setup().islog( ld1_.disp_.islogarithmic_ );
    ld1_.xax_.setup().epsaroundzero_ = 1e-5;
    ld1_.xax_.setup().maxnumberdigitsprecision_ = 3;
    ld2_.yax_.setup().islog( ld2_.disp_.islogarithmic_ );
    ld2_.xax_.setup().maxnumberdigitsprecision_ = 3;
    ld2_.xax_.setup().epsaroundzero_ = 1e-5;

    if ( ld1_.wl_ ) ld1_.xax_.setup().name( ld1_.wl_->name() );
    if ( ld2_.wl_ ) ld2_.xax_.setup().name( ld2_.wl_->name() );

    BufferString znm;
    if ( zdata_.zistime_ )
	znm += "TWT (ms)";
    else
	{ znm += "MD "; znm +=zdata_.dispzinft_ ? "(ft)" : "(m)"; }
    ld1_.yax_.setup().name( znm ); ld2_.yax_.setup().name( znm );
}


void uiWellLogDisplay::setAxisRelations()
{
    ld1_.xax_.setBegin( &ld1_.yax_ );
    ld1_.yax_.setBegin( &ld2_.xax_ );
    ld2_.xax_.setBegin( &ld1_.yax_ );
    ld2_.yax_.setBegin( &ld2_.xax_ );
    ld1_.xax_.setEnd( &ld2_.yax_ );
    ld1_.yax_.setEnd( &ld1_.xax_ );
    ld2_.xax_.setEnd( &ld2_.yax_ );
    ld2_.yax_.setEnd( &ld1_.xax_ );

    ld1_.xax_.setNewDevSize( width(), height() );
    ld1_.yax_.setNewDevSize( height(), width() );
    ld2_.xax_.setNewDevSize( width(), height() );
    ld2_.yax_.setNewDevSize( height(), width() );
}


void uiWellLogDisplay::gatherInfo( bool first )
{
    uiWellLogDisplay::LogData& ld = first ? ld1_ : ld2_;

    const int sz = ld.wl_ ? ld.wl_->size() : 0;
    if ( sz < 2 )
    {
	if ( !first )
	{
	    ld2_.copySetupFrom( ld1_ );
	    ld2_.zrg_ = ld1_.zrg_;
	    ld2_.valrg_ = ld1_.valrg_;
	}
	return;
    }

    if ( ld.disp_.cliprate_ || mIsUdf( ld.disp_.range_.start ) )
    {
	DataClipSampler dcs( sz );
	dcs.add( ld.wl_->valArr(), sz );
	ld.valrg_ = dcs.getRange( ld.disp_.cliprate_ );
    }
    else
	ld.valrg_ = ld.disp_.range_;

    float startpos = ld.zrg_.start = ld.wl_->dah( 0 );
    float stoppos = ld.zrg_.stop = ld.wl_->dah( sz-1 );
    if ( zdata_.zistime_ && zdata_.d2tm_ && zdata_.d2tm_->size() > 1  )
    {
	startpos = zdata_.d2tm_->getTime( startpos )*1000;
	stoppos = zdata_.d2tm_->getTime( stoppos )*1000;
    }
    ld.zrg_.start = startpos;
    ld.zrg_.stop = stoppos;
}


void uiWellLogDisplay::setAxisRanges( bool first )
{
    uiWellLogDisplay::LogData& ld = first ? ld1_ : ld2_;

    Interval<float> dispvalrg( ld.valrg_ );
    if ( ld.xrev_ ) Swap( dispvalrg.start, dispvalrg.stop );
	ld.xax_.setBounds( dispvalrg );

    Interval<float> dispzrg( zdata_.zrg_.stop, zdata_.zrg_.start );
    if ( zdata_.dispzinft_ )
	dispzrg.scale( mToFeetFactor );
    ld.yax_.setBounds( dispzrg );

    if ( first )
    {
    // Set default for 2nd
	ld2_.xax_.setBounds( dispvalrg );
	ld2_.yax_.setBounds( dispzrg );
    }
}


void uiWellLogDisplay::draw()
{
    setAxisRelations();
    if ( mIsUdf(zdata_.zrg_.start) ) return;

    ld1_.xax_.plotAxis(); ld1_.yax_.plotAxis();
    ld2_.xax_.plotAxis(); ld2_.yax_.plotAxis();

    drawMarkers();

    drawCurve( true );
    drawCurve( false );

    drawFilledCurve( true );
    drawFilledCurve( false );

    drawZPicks();
}


void uiWellLogDisplay::drawCurve( bool first )
{
    uiWellLogDisplay::LogData& ld = first ? ld1_ : ld2_;
    deepErase( ld.curveitms_ );
    delete ld.curvenmitm_; ld.curvenmitm_ = 0;
    const int sz = ld.wl_ ? ld.wl_->size() : 0;
    if ( sz < 2 ) return;

    ObjectSet< TypeSet<uiPoint> > pts;
    TypeSet<uiPoint>* curpts = new TypeSet<uiPoint>;
    for ( int idx=0; idx<sz; idx++ )
    {
	mDefZPosInLoop( ld.wl_->dah( idx ) );

	float val = ld.wl_->value( idx );
	if ( mIsUdf(val) )
	{
	    if ( !curpts->isEmpty() )
	    {
		pts += curpts;
		curpts = new TypeSet<uiPoint>;
	    }
	    continue;
	}

	*curpts += uiPoint( ld.xax_.getPix(val), ld.yax_.getPix(zpos) );
    }
    if ( curpts->isEmpty() )
	delete curpts;
    else
	pts += curpts;
    if ( pts.isEmpty() ) return;

    LineStyle ls(LineStyle::Solid);
    ls.width_ = ld.disp_.size_;
    ls.color_ = ld.disp_.color_;

    for ( int idx=0; idx<pts.size(); idx++ )
    {
	uiPolyLineItem* pli = scene().addItem( new uiPolyLineItem(*pts[idx]) );
	pli->setPenStyle( ls );
	pli->setZValue( 2 );
	ld.curveitms_ += pli;
    }

    Alignment al( Alignment::HCenter,
    first ? Alignment::Top : Alignment::Bottom );
    ld.curvenmitm_ = scene().addItem( new uiTextItem(ld.wl_->name(),al) );
    ld.curvenmitm_->setTextColor( ls.color_ );
    uiPoint txtpt;
    if ( first )
	txtpt = uiPoint( (*pts[0])[0] );
    else
    {
	TypeSet<uiPoint>& lastpts( *pts[pts.size()-1] );
	txtpt = lastpts[lastpts.size()-1];
    }
    ld.curvenmitm_->setPos( txtpt );

    deepErase( pts );
    if ( first )
	ld.yax_.annotAtEnd( zdata_.dispzinft_ ? "(ft)" : "(m)" );
    if ( ld.unitmeas_ )
	ld.xax_.annotAtEnd( BufferString("(",ld.unitmeas_->symbol(),")") );
}


static const int cMaxNrLogSamples = 2000;
#define mGetLoopSize(nrsamp,step)\
    {\
	if ( nrsamp > cMaxNrLogSamples )\
	{\
	    step = (float)nrsamp/cMaxNrLogSamples;\
	    nrsamp = cMaxNrLogSamples;\
	}\
    }

void uiWellLogDisplay::drawFilledCurve( bool first )
{
    uiWellLogDisplay::LogData& ld = first ? ld1_ : ld2_;
    deepErase( ld.curvepolyitms_ );

    if ( !ld.disp_.islogfill_ ) return;

    float colstep = ( ld.xax_.range().stop - ld.xax_.range().start ) / 255;
    int sz = ld.wl_ ? ld.wl_->size() : 0;
    if ( sz < 2 ) return;
    float step = 1;
    mGetLoopSize( sz, step );

    ObjectSet< TypeSet<uiPoint> > pts;
    TypeSet<int> colorintset;
    uiPoint closept;

    float zfirst = ld.wl_->dah(0);
    mDefZPos( zfirst )
    closept.x = ( first || ld.xrev_ ) ? ld.xax_.getPix(ld.xax_.range().stop)
				      : ld.xax_.getPix(ld.xax_.range().start);
    closept.y = ld.yax_.getPix( zfirst );
    int prevcolidx = 0;
    TypeSet<uiPoint>* curpts = new TypeSet<uiPoint>;
    *curpts += closept;

    uiPoint pt;
    for ( int idx=0; idx<sz; idx++ )
    {
	const int index = mNINT(idx*step);
	float dah = ld.wl_->dah( index );
	if ( index && index < sz-1 )
	{
	    if ( dah >= ld.wl_->dah(index+1) || dah <= ld.wl_->dah(index-1) )
	    continue;
	}
	mDefZPosInLoop( dah )
	float val = ld.wl_->value( index );

	if ( mIsUdf(val) )
	{
	    if ( !curpts->isEmpty() )
	    {
		pts += curpts;
		curpts = new TypeSet<uiPoint>;
		colorintset += 0;
	    }
	    continue;
	}

    pt.x = ld.xax_.getPix(val);
    pt.y = ld.yax_.getPix(zpos);

    *curpts += pt;

    float valdiff = ld.xrev_ ? ld.xax_.range().stop-val
    : val-ld.xax_.range().start;
    int colindex = (int)( valdiff/colstep );
    if ( colindex != prevcolidx )
    {
	*curpts += uiPoint( closept.x, pt.y );
	colorintset += colindex;
	prevcolidx = colindex;
	pts += curpts;
	curpts = new TypeSet<uiPoint>;
	*curpts += uiPoint( closept.x, pt.y );
    }
    }
    if ( pts.isEmpty() ) return;
    *pts[pts.size()-1] += uiPoint( closept.x, pt.y );

    const int tabidx = ColTab::SM().indexOf( ld.disp_.seqname_ );
    const ColTab::Sequence* seq = ColTab::SM().get( tabidx<0 ? 0 : tabidx );
    for ( int idx=0; idx<pts.size(); idx++ )
    {
	uiPolygonItem* pli = scene().addPolygon( *pts[idx], true );
	ld.curvepolyitms_ += pli;
	Color color =
	ld.disp_.issinglecol_ ? ld.disp_.seiscolor_
			      : seq->color( (float)colorintset[idx]/255 );
	pli->setFillColor( color );
	LineStyle ls;
	ls.width_ = 1;
	ls.color_ = color;
	pli->setPenStyle( ls );
	pli->setZValue( 1 );
    }
    deepErase( pts );
}


#define mDefHorLineX1X2Y() \
const int x1 = ld1_.xax_.getRelPosPix( 0 ); \
const int x2 = ld1_.xax_.getRelPosPix( 1 ); \
const int y = ld1_.yax_.getPix( zpos )

void uiWellLogDisplay::drawMarkers()
{
    deepErase( markerdraws_ );

    if ( !zdata_.markers_ ) return;

    for ( int idx=0; idx<zdata_.markers_->size(); idx++ )
    {
	const Well::Marker& mrkr = *((*zdata_.markers_)[idx]);
	const Color& col = mrkr.color();
	if ( col == Color::NoColor() || col == Color::White() ) continue;

	mDefZPosInLoop( mrkr.dah() );
	mDefHorLineX1X2Y();

	MarkerDraw* mrkdraw = new MarkerDraw( mrkr );
	markerdraws_ += mrkdraw;

	uiLineItem* li = scene().addItem( new uiLineItem(x1,y,x2,y,true) );
	LineStyle ls = LineStyle(setup_.markerls_.type_,
	setup_.markerls_.width_,col);
	li->setPenStyle( ls );
	li->setZValue( 2 );
	mrkdraw->lineitm_ = li;

	BufferString mtxt( mrkr.name() );
	if ( setup_.nrmarkerchars_ < mtxt.size() )
	mtxt[setup_.nrmarkerchars_] = '\0';
	uiTextItem* ti = scene().addItem(
	new uiTextItem(mtxt,mAlignment(Right,VCenter)) );
	ti->setPos( uiPoint(x1-1,y) );
	ti->setTextColor( col );
	mrkdraw->txtitm_ = ti;
    }
}


uiWellLogDisplay::MarkerDraw* uiWellLogDisplay::getMarkerDraw(
						const Well::Marker& mrk )
{
    for ( int idx=0; idx<markerdraws_.size(); idx++)
    {
	if ( &(markerdraws_[idx]->mrk_) == &mrk )
	    return markerdraws_[idx];
    }
    return 0;
}


void uiWellLogDisplay::drawZPicks()
{
    deepErase( zpickitms_ );

    for ( int idx=0; idx<zpicks_.size(); idx++ )
    {
	const PickData& pd = zpicks_[idx];
	mDefZPosInLoop( pd.dah_ );
	mDefHorLineX1X2Y();

	uiLineItem* li = scene().addItem( new uiLineItem(x1,y,x2,y,true) );
	Color lcol( setup_.pickls_.color_ );
	if ( pd.color_ != Color::NoColor() )
	lcol = pd.color_;
	li->setPenStyle( LineStyle(setup_.pickls_.type_,setup_.pickls_.width_,
			lcol) );
	li->setZValue( 2 );
	zpickitms_ += li;
    }
}

