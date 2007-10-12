/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: visdata.cc,v 1.26 2007-10-12 19:14:34 cvskris Exp $";

#include "visdata.h"

#include "errh.h"
#include "iopar.h"
#include "visdataman.h"
#include "visselman.h"

#include <Inventor/nodes/SoNode.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/SoOutput.h>

namespace visBase
{


const char* DataObject::sKeyType()	{ return "Type"; }
const char* DataObject::sKeyName()	{ return "Name"; }


const SoNode* DataObject::getInventorNode() const
{ return const_cast<const SoNode*>(((DataObject*)this)->getInventorNode() ); }


const char* DataObject::name() const
{
    return !name_ || name_->isEmpty() ? 0 : name_->buf();
}


void DataObject::setName( const char* nm )
{
    SoNode* node = getInventorNode();
    if ( node )
	node->setName( nm );

    if ( !name_ ) name_ = new BufferString;
    (*name_) = nm;
}


DataObject::DataObject()
    : id_( -1 )
    , name_( 0 )
{}


DataObject::~DataObject()
{
    DM().removeObject( this );
    delete name_;
}


void DataObject::select() const
{ DM().selMan().select( id() ); }


void DataObject::deSelect() const
{ DM().selMan().deSelect( id() ); }


bool DataObject::isSelected() const
{ return selectable() && DM().selMan().selected().indexOf( id()) != -1; }


void DataObject::setDisplayTransformation( Transformation* )
{   
    pErrMsg("Not implemented");
}   
    

void DataObject::fillPar( IOPar& par, TypeSet<int>& ) const
{
    par.set( sKeyType(), getClassName() );

    const char* nm = name();
    if ( nm )
	par.set( sKeyName(), nm );
}


bool DataObject::dumpOIgraph( const char* filename, bool binary )
{
    SoNode* node = getInventorNode();
    if ( !node ) return false;

    SoWriteAction writeaction;
    if ( !writeaction.getOutput()->openFile(filename) )
	return false;

    writeaction.getOutput()->setBinary(binary);
    writeaction.apply( node );
    writeaction.getOutput()->closeFile();
    return true;
}


int DataObject::usePar( const IOPar& par )
{
    const char* nm = par.find( sKeyName() );
    if ( nm )
	setName( nm );

    return 1;
}


bool DataObject::_init()
{
    DM().addObject( this );
    return true;
}


}; // namespace visBase
