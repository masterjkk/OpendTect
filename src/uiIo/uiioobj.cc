/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2006
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiioobj.h"
#include "uimsg.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "iodir.h"
#include "ioobj.h"
#include "transl.h"
#include "filepath.h"


bool uiIOObj::removeImpl( bool rmentry, bool mustrm, bool doconfirm )
{
    bool dorm = true;
    const bool isoutside = !ioobj_.isInCurrentSurvey();
    if ( !silent_ )
    {
	BufferString mess = "Remove ";
	if ( !ioobj_.isSubdir() )
	{
	    mess.add( "'" ).add( ioobj_.name() ).add( "'?" );
	    mess += isoutside ? "\nFile not in current survey.\n"
				"Specify what you would like to remove" : "";
	}
	else
	{
	    mess.add( "'" ).add( ioobj_.name() ).add( "' with folder\n" );
	    BufferString fullexpr( ioobj_.fullUserExpr(true) );
	    mess += ioobj_.fullUserExpr(true);
	    mess += "\n- and everything in it! - ?";
	}

	if ( isoutside )
	{
	    const int resp = uiMSG().question( mess, "Remove file", 
					       "Remove link", "Cancel",
					       "Remove data" );
	    if ( resp < 0 )
		return false;

	    dorm = resp;
	}
	else if ( doconfirm && !uiMSG().askRemove(mess) )
	{
	    if ( mustrm )
		return false;

	    dorm = false;
	}
    }

    if ( dorm && !fullImplRemove(ioobj_) )
    {
	if ( !silent_ )
	{
	    BufferString mess = "Could not remove data file(s).\n";
	    mess += "Remove entry from list anyway?";
	    if ( !uiMSG().askRemove(mess) )
		return false;
	}
    }

    if ( rmentry )
	IOM().permRemove( ioobj_.key() );

    return true;
}


bool uiIOObj::fillCtio( CtxtIOObj& ctio, bool warnifexist )
{
    if ( ctio.name().isEmpty() )
    {
	if ( !ctio.ioobj )
	    return false;
	ctio.setName( ctio.ioobj->name() );
    }
    const char* nm = ctio.name().buf();

    const IODir iodir( ctio.ctxt.getSelKey() );
    const IOObj* existioobj = iodir.get( nm, ctio.ctxt.trgroup->userName() );
    if ( !existioobj )
    {
	ctio.setObj( 0 );
	IOM().getEntry( ctio );
	return ctio.ioobj;
    }

    if ( warnifexist )
    {
	BufferString msg( "Overwrite existing '" );
	msg += nm; msg += "'?";
	if ( !uiMSG().askOverwrite(msg) )
	    return false;
    }

    ctio.setObj( existioobj->clone() );
    return true;
}
