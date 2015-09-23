/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		March 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "volproctrans.h"

#include "volprocchain.h"
#include "ascstream.h"
#include "uistrings.h"

defineTranslatorGroup(VolProcessing,"Volume Processing Setup");
defineTranslator(dgb,VolProcessing,mDGBKey);

uiString VolProcessingTranslatorGroup::sTypeName()
{ return tr("Volume Processing Setup"); }

mDefSimpleTranslatorioContext(VolProcessing,Misc)
mDefSimpleTranslatorSelector(VolProcessing);


bool VolProcessingTranslator::retrieve( VolProc::Chain& vr,
				    const IOObj* ioobj,
				    uiString& bs )
{
    if ( !ioobj )
    {
	bs = uiStrings::phrCannotFindDBEntry(
	   mToUiStringTodo(VolProcessingTranslatorGroup::sGroupName()));
	return false;
    }
    mDynamicCastGet(VolProcessingTranslator*,t,ioobj->createTranslator())
    if ( !t )
    {
	bs = uiStrings::phrCannotOpen( ioobj->uiName() );
	return false;
    }
    PtrMan<VolProcessingTranslator> tr = t;
    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
    { bs = uiStrings::phrCannotOpen( ioobj->uiName() ); return false; }

    bs = mToUiStringTodo(tr->read( vr, *conn ));
    if ( bs.isEmpty() )
    {
	vr.setStorageID( ioobj->key() );
	return true;
    }

    return false;
}


bool VolProcessingTranslator::store( const VolProc::Chain& vr,
				const IOObj* ioobj, uiString& bs )
{
    if ( !ioobj )
    {
	bs = uiStrings::phrCannotFindDBEntry(
		 mToUiStringTodo(VolProcessingTranslatorGroup::sGroupName()));
	return false;
    }
    mDynamicCast(VolProcessingTranslator*,PtrMan<VolProcessingTranslator> tr,
		 ioobj->createTranslator())
    if ( !tr )
    {
	bs = uiStrings::phrCannotOpen( ioobj->uiName() );
	return false;
    }

    bs = uiString::emptyString();
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
    { bs = uiStrings::phrCannotOpen( ioobj->uiName() ); }
    else
	bs = mToUiStringTodo(tr->write( vr, *conn ));

    return bs.isEmpty();
}


const char* dgbVolProcessingTranslator::read( VolProc::Chain& chain,
					      Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    ascistream astrm( ((StreamConn&)conn).iStream() );
    if ( !astrm.isOK() )
	return "Cannot read from input file";
    if ( !astrm.isOfFileType(mTranslGroupName(VolProcessing)) )
	return "Input file is not a Volume Processing setup file";
    if ( atEndOfSection(astrm) ) astrm.next();

    IOPar par;
    par.getFrom( astrm );
    if ( par.isEmpty() )
	return "Input file contains no data";
    if ( !chain.usePar( par ) )
	return chain.errMsg().getFullString();

    return 0;
}


const char* dgbVolProcessingTranslator::write( const VolProc::Chain& chain,
					   Conn& conn )
{
    if ( !conn.forWrite() || !conn.isStream() )
	return "Internal error: bad connection";

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(VolProcessing) );
    if ( !astrm.isOK() )
	return "Cannot write to output Volume Processing setup file";

    IOPar par;
    chain.fillPar( par );
    par.putTo( astrm );

    return astrm.isOK() ? 0
	:  "Error during write to output Volume Processing setup file";
}
