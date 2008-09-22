#ifndef seiseventsnapper_h
#define seiseventsnapper_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: seiseventsnapper.h,v 1.4 2008-09-22 13:11:25 cvskris Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "samplingdata.h"
#include "valseriesevent.h"

class BinIDValueSet;
class IOObj;
class SeisMSCProvider;
class SeisTrc;

class SeisEventSnapper : public Executor
{
public:
				SeisEventSnapper(const IOObj&,BinIDValueSet&);
				~SeisEventSnapper();

    void			setEvent( VSEvent::Type tp )
    				{ eventtype_ = tp; }
    VSEvent::Type		getEvent() const	{ return eventtype_; }

    void			setSearchGate( const Interval<float>& gate )
				{ searchgate_ = gate; }
    const Interval<float>&	getSearchGate() const	{ return searchgate_; }

    virtual od_int64		totalNr() const		{ return totalnr_; }
    virtual od_int64		nrDone() const		{ return nrdone_; }

protected:

    virtual int			nextStep();
    float			findNearestEvent(const SeisTrc&,
	    					 float tarz) const;

    BinIDValueSet&		positions_;
    SeisMSCProvider*		mscprov_;
    Interval<float>		searchgate_;
    VSEvent::Type		eventtype_;
    SamplingData<float>		sd_;
    int				nrsamples_;

    int				totalnr_;
    int				nrdone_;

};

#endif
