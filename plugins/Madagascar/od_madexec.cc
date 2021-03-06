/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	R. K. Singh
 Date:		March 2008
________________________________________________________________________

-*/

#include "batchprog.h"
#include "iopar.h"
#include "madprocexec.h"
#include "moddepmgr.h"

bool BatchProgram::go( od_ostream& strm )
{
    OD::ModDeps().ensureLoaded( "AttributeEngine" );

    ODMad::ProcExec exec( pars(), strm );
    if ( !exec.init() || !exec.execute() )
    {
	BufferString cmd = "od_DispMsg --err ";
	cmd += toString( exec.errMsg() );
	system( cmd );
	return false;
    }

    return true;
}
