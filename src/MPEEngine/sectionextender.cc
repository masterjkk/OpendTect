/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: sectionextender.cc,v 1.6 2005-09-27 19:26:03 cvskris Exp $";

#include "sectionextender.h"

#include "position.h"

namespace MPE 
{


SectionExtender::SectionExtender( const EM::SectionID& si)
    : sid( si )
    , trkstattbl(0)
{}


EM::SectionID SectionExtender::sectionID() const { return sid; }


void SectionExtender::reset()
{
    addedpos.erase();
    addedpossrc.erase();
    trkstattbl = 0;
}


void SectionExtender::setDirection( const BinIDValue& ) {}


const BinIDValue* SectionExtender::getDirection() const { return 0; }


void SectionExtender::setStartPositions( const TypeSet<EM::SubID> ns )
{ startpos = ns; }


int SectionExtender::nextStep() { return 0; }


#define mExtendDirection(inl,crl,z) \
setDirection(BinIDValue(inl,crl,z)); \
while ( (res=nextStep())>0 )\
    ;\
\
if ( res==-1 ) return 

void SectionExtender::extendInVolume(const BinID& bidstep, float zstep)
{
    int res;
    mExtendDirection(bidstep.inl, 0, 0);
    mExtendDirection(-bidstep.inl, 0, 0);
    mExtendDirection(0, bidstep.crl, 0);
    mExtendDirection(0, -bidstep.crl, 0);
    mExtendDirection(bidstep.inl, bidstep.crl,0);
    mExtendDirection(bidstep.inl, -bidstep.crl,0);
    mExtendDirection(-bidstep.inl, bidstep.crl,0);
    mExtendDirection(-bidstep.inl, -bidstep.crl,0);
    mExtendDirection(0,0,zstep);
    mExtendDirection(0,0,-zstep);
}


const char* SectionExtender::errMsg() const { return errmsg[0] ? errmsg : 0; }


const TypeSet<EM::SubID>& SectionExtender::getAddedPositions() const
{ return addedpos; }


const TypeSet<EM::SubID>& SectionExtender::getAddedPositionsSource() const
{ return addedpossrc; }


void SectionExtender::addTarget( const EM::SubID& target,
			         const EM::SubID& src )
{
    if ( addedpos.indexOf(target)!=-1 )
	return;

    addedpossrc += src;
    addedpos += target;
}


};



