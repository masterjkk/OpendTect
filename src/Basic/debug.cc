/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          June 2003
 RCS:           $Id: debug.cc,v 1.22 2006-09-11 09:19:00 cvsbert Exp $
________________________________________________________________________

-*/

static const char* rcsID = "$Id: debug.cc,v 1.22 2006-09-11 09:19:00 cvsbert Exp $";

#include "debug.h"
#include "debugmasks.h"
#include "genc.h"
#include "bufstring.h"
#include "timefun.h"
#include "filepath.h"
#include "envvars.h"

#include <iostream>
#include <fstream>

static std::ostream* dbglogstrm = 0;


namespace DBG
{

static int getMask()
{
    static bool maskgot = false;
    static int themask = 0;
    if ( maskgot ) return themask;
    maskgot = true;

    BufferString envmask = GetEnvVar( "DTECT_DEBUG" );
    const char* buf = envmask.buf();
    themask = atoi( buf );
    if ( buf[0] == 'y' || buf[0] == 'Y' ) themask = 0xffff;

    const char* dbglogfnm = GetEnvVar( "DTECT_DEBUG_LOGFILE" );
    if ( dbglogfnm && !themask )
	themask = 0xffff;

    if ( themask )
    {
	BufferString msg;
	if ( dbglogfnm )
	{
	    dbglogstrm = new std::ofstream( dbglogfnm );
	    if ( dbglogstrm && !dbglogstrm->good() )
	    {
		delete dbglogstrm; dbglogstrm = 0;
		msg = "Cannot open debug log file '";
		msg += dbglogfnm; msg == "': reverting to stdout";
		message( msg );
	    }
	}
    }

    return themask;
}


bool isOn( int flag )
{
    const int mask = getMask();
    return flag & mask;
}


void message( const char* msg )
{
    if ( !isOn() ) return;

    BufferString msg_;
    static bool wantpid = GetEnvVarYN("DTECT_ADD_DBG_PID");
    if ( wantpid )
    {
	msg_ = "[";
	msg_ += GetPID();
	msg_ += "] ";
    }
    msg_ += msg;

    if ( dbglogstrm )
	*dbglogstrm << msg_ << std::endl;
    else
	std::cerr << msg_ << std::endl;
}


void message( int flag, const char* msg )
{
    if ( isOn(flag) ) message(msg);
}

void putProgInfo( int argc, char** argv )
{
    if ( GetEnvVarYN("OD_NO_PROGINFO") ) return;

    const bool ison = isOn( DBG_PROGSTART );

    BufferString msg;
    if ( ison )
    {
	msg = "\n---------\n";
	msg += argv[0];
    }
    else
    {
	FilePath fp( argv[0] );
	msg = fp.fileName();
    }
    msg += " started on "; msg += GetHostName();
    msg += " at "; msg += Time_getFullDateString();
    if ( !ison ) msg += "\n";
    message( msg );
    if ( !ison ) return;

    msg = "PID: "; msg += GetPID();
    msg += "; Platform: ";
#ifdef lux
    msg += "Linux";
# ifdef lux64
    msg += " (64 bits)";
# endif
#endif
#ifdef sun5
    msg += "Solaris";
# ifdef sol64
    msg += " (64 bits)";
# endif
#endif
#ifdef mac
    msg += "Mac OS/X";
#endif
#ifdef win
    msg += "M$ Windows";
#endif

#ifdef __GNUC__
    msg += "; gcc ";
    msg += __GNUC__; msg += "."; msg += __GNUC_MINOR__;
    msg += "."; msg += __GNUC_PATCHLEVEL__;
#endif
    message( msg );

    if ( argc > 1 )
    {
	msg = "Program args:";
	for ( int idx=1; idx<argc; idx++ )
	    { msg += " '"; msg += argv[idx]; msg += "'"; }
	message( msg );
    }
    message( "---------\n" );
}

};


extern "C" int od_debug_isOn( int flag )
    { return DBG::isOn(flag); }
extern "C" void od_debug_message( const char* msg )
    { DBG::message(msg); }
extern "C" void od_debug_messagef( int flag, const char* msg )
    { DBG::message(flag,msg); }
extern "C" void od_debug_putProgInfo( int argc, char** argv )
    { DBG::putProgInfo(argc,argv); }
extern "C" void od_putProgInfo( int argc, char** argv )
    { od_debug_putProgInfo(argc,argv); }
