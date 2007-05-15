/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: prestackprocessor.cc,v 1.6 2007-05-15 21:28:22 cvskris Exp $";

#include "prestackprocessor.h"

#include "iopar.h"
#include "keystrs.h"
#include "prestackgather.h"

namespace PreStack
{


mImplFactory( Processor, PF );


Processor::Processor()
    : output_( 0 )
    , input_( 0 )
{}


Processor::~Processor()
{
    if ( output_ ) DPM( DataPackMgr::FlatID ).release( output_->id() );
    if ( input_ ) DPM( DataPackMgr::FlatID ).release( input_->id() );
}


void Processor::setInput( DataPack::ID id )
{
    if ( input_ ) DPM( DataPackMgr::FlatID ).release( input_->id() );
    DataPack* dp = DPM( DataPackMgr::FlatID ).obtain( id, false );
    mDynamicCastGet( Gather*, gather, dp );
    if ( gather ) input_ = gather;
    else if ( dp ) DPM( DataPackMgr::FlatID ).release( dp->id() );
}


DataPack::ID Processor::getOutput() const
{ return output_ ? output_->id() : -1; }


bool Processor::prepareWork()
{
    if ( !input_ ) return false;

    if ( output_ )
    {
	DPM( DataPackMgr::FlatID ).release( output_->id() );
	output_ = 0;
    }

    output_ = new Gather(*input_);
    DPM( DataPackMgr::FlatID ).add( output_ );
    DPM( DataPackMgr::FlatID ).obtain( output_->id() );

    return true;
}



int Processor::nrOffsets() const
{
    return input_->data().info().getSize( Gather::offsetDim() );
}


ProcessManager::ProcessManager()
    : input_( 0 )
    , output_( 0 )
{}


ProcessManager::~ProcessManager()
{
    deepErase( processors_ );
    if ( output_ ) DPM( DataPackMgr::FlatID ).release( output_->id() );
    if ( input_ ) DPM( DataPackMgr::FlatID ).release( input_->id() );
}


void ProcessManager::setInput( DataPack::ID id )
{
    if ( input_ ) DPM( DataPackMgr::FlatID ).release( input_->id() );
    input_ = 0;
    DataPack* dp = DPM( DataPackMgr::FlatID ).obtain( id, false );
    mDynamicCastGet( Gather*, gather, dp );
    if ( gather ) input_ = gather;
    else if ( dp ) DPM( DataPackMgr::FlatID ).release( dp->id() );
}


bool ProcessManager::process(bool forceall)
{
    DataPack::ID curinput = input_->id();
    for ( int idx=0; idx<processors_.size(); idx++ )
    {
	processors_[idx]->setInput( curinput );
	if ( !processors_[idx]->prepareWork() || !processors_[idx]->execute() )
	    return false;

	curinput = processors_[idx]->getOutput();
    }

    if ( output_ ) DPM( DataPackMgr::FlatID ).release( output_->id() );
    output_ = 0;
    DataPack* dp = DPM( DataPackMgr::FlatID ).obtain( curinput, false );
    mDynamicCastGet( Gather*, gather, dp );
    if ( gather ) output_ = gather;
    else if ( dp ) DPM( DataPackMgr::FlatID ).release( dp->id() );

    return true;
}


DataPack::ID ProcessManager::getOutput() const
{ return output_ ? output_->id() : -1; }


void ProcessManager::addProcessor( Processor* sgp )
{ processors_ += sgp; }


int ProcessManager::nrProcessors() const
{ return processors_.size(); }


void ProcessManager::removeProcessor( int idx )
{ delete processors_.remove( idx ); }


void ProcessManager::swapProcessors( int i0, int i1 )
{ processors_.swap( i0, i1 ); }


Processor* ProcessManager::getProcessor( int idx )
{ return processors_[idx]; }


const Processor*
ProcessManager::getProcessor( int idx ) const
{ return processors_[idx]; }


void ProcessManager::fillPar( IOPar& par ) const
{
    par.set( sKeyNrProcessors(), processors_.size() );
    for ( int idx=0; idx<processors_.size(); idx++ )
    {
	IOPar procpar;
	procpar.set( sKey::Name, processors_[idx]->name() );
	processors_[idx]->fillPar( procpar );

	const BufferString idxstr = idx;
	par.mergeComp( procpar, idxstr.buf() );
    }
}


bool ProcessManager::usePar( const IOPar& par )
{
    deepErase( processors_ );

    int nrprocessors;
    if ( !par.get( sKeyNrProcessors(), nrprocessors ) )
	return false;

    for ( int idx=0; idx<nrprocessors; idx++ )
    {
	const BufferString idxstr = idx;
	PtrMan<IOPar> steppar = par.subselect( idxstr.buf() );
	if ( !steppar )
	    continue;

	BufferString name;
	if ( !steppar->get( sKey::Name, name ) )
	    return false;

	Processor* proc = PF().create( name.buf() );
	if ( !proc )
	    return false;

	if ( !proc->usePar( *steppar ) )
	{
	    delete proc;
	    return false;
	}

	addProcessor( proc );
    }

    return true;
}


}; //namespace


#include "prestackagc.h"
#include "prestackmute.h"

void PreStack::initBuiltinClasses()
{
    PreStack::Mute::initClass();
    PreStack::AGC::initClass();
}
