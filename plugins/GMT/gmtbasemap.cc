/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		Jube 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: gmtbasemap.cc,v 1.15 2009-01-30 07:06:12 cvsraman Exp $";

#include "bufstringset.h"
#include "color.h"
#include "draw.h"
#include "filepath.h"
#include "gmtbasemap.h"
#include "keystrs.h"
#include "strmdata.h"
#include "strmprov.h"
#include "survinfo.h"
#include <iostream>


static const int cTitleBoxHeight = 4;
static const int cTitleBoxWidth = 8;

int GMTBaseMap::factoryid_ = -1;

void GMTBaseMap::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Basemap", GMTBaseMap::createInstance );
}

GMTPar* GMTBaseMap::createInstance( const IOPar& iop )
{
    return new GMTBaseMap( iop );
}


bool GMTBaseMap::execute( std::ostream& strm, const char* fnm )
{
    strm << "Creating the Basemap ...  ";
    BufferString maptitle = find( ODGMT::sKeyMapTitle );
    Interval<float> lblintv;
    if ( !get(ODGMT::sKeyLabelIntv,lblintv) )
	mErrStrmRet("Incomplete data for basemap creation")

    bool closeps = false, dogrid = false;
    getYN( ODGMT::sKeyClosePS, closeps );
    getYN( ODGMT::sKeyDrawGridLines, dogrid );

    BufferString comm = "psbasemap ";
    BufferString str; mGetRangeProjString( str, "X" );
    comm += str; comm += " -Ba";
    comm += lblintv.start;
    if ( dogrid ) { comm += "g"; comm += lblintv.start; }
    comm += "/a"; comm += lblintv.stop;
    if ( dogrid ) { comm += "g"; comm += lblintv.stop; }
    comm += ":\"."; comm += maptitle; comm += "\":";
    comm += " --Y_AXIS_TYPE=ver_text";
    comm += " --HEADER_FONT_SIZE=24";
    comm += " --X_ORIGIN="; comm += xmargin;
    comm += "c --Y_ORIGIN="; comm += ymargin;
    comm += "c --PAPER_MEDIA=Custom_";
    float pagewidth = mapdim.start + 5 * xmargin;
    const float pageheight = mapdim.stop + 3 * ymargin;
    comm += pageheight < 21 ? 21 : pageheight; comm += "cx";
    comm += pagewidth < 21 ? 21 : pagewidth; comm += "c -K ";

    comm += "> "; comm += fileName( fnm );
    if ( system(comm) )
	mErrStrmRet("Failed to create Basemap")

    strm << "Done" << std::endl;

    strm << "Posting title box ...  ";
    comm = "@pslegend -R -J -F -O -Dx";
    comm += mapdim.start + xmargin; comm += "c/";
    comm += 0; comm += "c/";
    comm += cTitleBoxWidth; comm += "c/";
    comm += cTitleBoxHeight; comm += "c/BL ";
    if ( !closeps ) comm += "-K ";

    comm += "-UBL/0/0 ";    
    comm += ">> "; comm += fileName( fnm );
    StreamData sd = StreamProvider(comm).makeOStream();
    if ( !sd.usable() ) mErrStrmRet("Failed to overlay title box")

    *sd.ostrm << "H 16 4 " << maptitle << std::endl;
    *sd.ostrm << "G 0.5l" << std::endl;
    int scaleval = 1;
    get( ODGMT::sKeyMapScale, scaleval );
    *sd.ostrm << "L 10 4 C Scale  1:" << scaleval << std::endl;
    *sd.ostrm << "D 0 1p" << std::endl;
    BufferStringSet remset;
    get( ODGMT::sKeyRemarks, remset );
    for ( int idx=0; idx<remset.size(); idx++ )
	*sd.ostrm << "L 12 4 C " << remset.get(idx) << std::endl;

    *sd.ostrm << std::endl;
    sd.close();
    strm << "Done" << std::endl;
    return true;
}


int GMTLegend::factoryid_ = -1;

void GMTLegend::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Legend", GMTLegend::createInstance );
}

GMTPar* GMTLegend::createInstance( const IOPar& iop )
{
    return new GMTLegend( iop );
}


bool GMTLegend::execute( std::ostream& strm, const char* fnm )
{
    strm << "Posting legends ...  ";
    Interval<float> mapdim;
    get( ODGMT::sKeyMapDim, mapdim );
    const float xmargin = mapdim.start > 30 ? mapdim.start/10 : 3;
    const float ymargin = mapdim.stop > 30 ? mapdim.stop/10 : 3;
    bool hascolbar = false;
    ObjectSet<IOPar> parset;
    for ( int idx=0; idx<100; idx++ )
    {
	IOPar* par = subselect( idx );
	if ( !par ) break;

	if ( par->find(ODGMT::sKeyPostColorBar) )
	{
	    hascolbar = true;
	    StepInterval<float> rg;
	    par->get( ODGMT::sKeyDataRange, rg );
	    FilePath fp( fnm );
	    fp.setExtension( "cpt" );
	    BufferString colbarcomm = "psscale --LABEL_FONT_SIZE=14 -D";
	    colbarcomm += mapdim.start + xmargin; colbarcomm += "c/";
	    colbarcomm += 1.2 * ymargin + cTitleBoxHeight; colbarcomm += "c/";
	    colbarcomm += 2 * ymargin; colbarcomm += "c/";
	    colbarcomm += xmargin / 2; colbarcomm += "c -O -C";
	    colbarcomm += fileName( fp.fullPath() ); colbarcomm += " -B";
	    colbarcomm += rg.step * 5; colbarcomm += ":\"";
	    colbarcomm += par->find( sKey::Name ); colbarcomm += "\":/:";
	    colbarcomm += par->find( ODGMT::sKeyAttribName );
	    colbarcomm += ": -K >> "; colbarcomm += fileName( fnm );
	    if ( system(colbarcomm) )
		mErrStrmRet("Failed to post color bar")

            StreamProvider( fp.fullPath() ).remove();
	    if ( !par->find(ODGMT::sKeyLineStyle) )
		continue;
	}

	parset += par;
    }

    const int nritems = parset.size();
    BufferString comm = "@pslegend -R -J -O -Dx";
    comm += mapdim.start + xmargin; comm += "c/";
    comm += ymargin / 2 + cTitleBoxHeight + ( hascolbar ? 2 * ymargin : 0 );
    comm += "c/"; comm += 10; comm += "c/";
    comm += nritems ? nritems : 1; comm += "c/BL ";
    
    comm += ">> "; comm += fileName( fnm );
    StreamData sd = StreamProvider(comm).makeOStream();
    if ( !sd.usable() ) mErrStrmRet("Failed to overlay legend")

    for ( int idx=0; idx<nritems; idx++ )
    {
	IOPar* par = parset[idx];
	BufferString namestr = par->find( sKey::Name );
	if ( namestr.isEmpty() )
	    continue;

	float size = 1;
	BufferString symbstr, penstr;
	const char* shapestr = par->find( ODGMT::sKeyShape );
	const ODGMT::Shape shape = eEnum( ODGMT::Shape, shapestr );
	if ( shape < 0 ) continue;

	symbstr = ODGMT::sShapeKeys[(int)shape];
	par->get( sKey::Size, size );
	if ( shape == ODGMT::Polygon || shape == ODGMT::Line )
	{
	    const char* lsstr = par->find( ODGMT::sKeyLineStyle );
	    if ( !lsstr ) continue;

	    LineStyle ls;
	    ls.fromString( lsstr );
	    if ( ls.type_ != LineStyle::None )
	    {
		mGetLineStyleString( ls, penstr );
	    }
	    else if ( shape == ODGMT::Line )
		continue;
	}
	else
	{
	    Color pencol;
	    par->get( sKey::Color, pencol );
	    BufferString colstr;
	    mGetColorString( pencol, colstr );
	    penstr = "1p,"; penstr += colstr;
	}

	bool dofill;
	par->getYN( ODGMT::sKeyFill, dofill );
	BufferString legendstring = "S 0.6c ";
	legendstring += symbstr; legendstring += " "; 
	legendstring += size > 1 ? 1 : size;
	legendstring += "c ";
	if ( dofill )
	{
	    BufferString fillcolstr;
	    Color fillcol;
	    par->get( ODGMT::sKeyFillColor, fillcol );
	    mGetColorString( fillcol, fillcolstr );
	    legendstring += fillcolstr;
	}
	else
	    legendstring += "-";

	legendstring += " ";
	if ( penstr.isEmpty() )
	    legendstring += "-";
	else
	    legendstring += penstr;

	legendstring += " "; legendstring += 1.3;
	legendstring += " "; legendstring += namestr;
	*sd.ostrm << legendstring << std::endl;
	*sd.ostrm << "G0.2c" << std::endl;
    }

    sd.close();
    strm << "Done" << std::endl;
    return true;
}



int GMTCommand::factoryid_ = -1;

void GMTCommand::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Advanced", GMTCommand::createInstance );
}

GMTPar* GMTCommand::createInstance( const IOPar& iop )
{
    return new GMTCommand( iop );
}


const char* GMTCommand::userRef() const
{
    BufferString* str = new BufferString( "GMT Command: " );
    const char* res = find( ODGMT::sKeyCustomComm );
    *str += res;
    *( str->buf() + 25 ) = '\0';
    return str->buf();
}


bool GMTCommand::execute( std::ostream& strm, const char* fnm )
{
    strm << "Executing custom command" << std::endl;
    const char* res = find( ODGMT::sKeyCustomComm );
    if ( !res || !*res )
	mErrStrmRet("No command to execute")

    strm << res << std::endl;
    BufferString comm = res;
    char* commptr = comm.buf();
    if ( strstr(commptr,".ps") )
    {
	char* ptr = strstr( commptr, ">>" );
	if ( ptr )
	{
	    BufferString temp = ptr;
	    *ptr = '\0';
	    comm += "-O -K ";
	    comm += temp;
	}
    }

    if ( system(comm.buf()) )
	mErrStrmRet("... Failed")

    strm << "... Done" << std::endl;
    return true;
}

