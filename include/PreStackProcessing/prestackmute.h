#ifndef prestackmute_h
#define prestackmute_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Nov 2006
 RCS:		$Id: prestackmute.h,v 1.10 2010-02-17 21:12:15 cvskris Exp $
________________________________________________________________________


-*/

#include "prestackprocessor.h"
#include "multiid.h"

class Muter;

namespace PreStack
{
class MuteDef;

mClass Mute : public Processor
{
public:

    static void		initClass();
    static Processor*	createFunc();

 			Mute();
    			~Mute();

    bool		prepareWork();

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    const char*		errMsg() const		{ return errmsg_; }

    static const char*	sName()			{ return "Mute"; }
    static const char*	sTaperLength()		{ return "Taper Length";}
    static const char*	sTailMute()		{ return "Tail Mute";}
    static const char*	sMuteDef()		{ return "Mute Definition";}

    const MultiID&	muteDefID() const	{ return id_; }
    const MuteDef&	muteDef() const		{ return def_; }
    MuteDef&		muteDef()		{ return def_; }
    bool		isTailMute() const	{ return tail_; }
    float		taperLength() const	{ return taperlen_; }
    bool		setMuteDefID(const MultiID&);
    void		setEmptyMute();
    void		setTailMute(bool yn=true);
    void		setTaperLength(float l);

protected:

    MuteDef&			def_;
    Muter*			muter_;
    MultiID			id_;
    BufferString		errmsg_;

    bool			doWork(od_int64,od_int64,int);

    bool			tail_;
    float			taperlen_;
};


}; //namespace

#endif
