#ifndef uistratsynthexport_h
#define uistratsynthexport_h
/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki / Bert
 Date:          July 2013
 RCS:           $Id$
 _______________________________________________________________________

      -*/


#include "uiwellattribmod.h"
#include "uidialog.h"
class uiGenInput;
class uiSeisSel;
class StratSynth;
class uiSeis2DLineSel;


mExpClass(uiWellAttrib) uiStratSynthExport : public uiDialog
{
public:
    			uiStratSynthExport(uiParent*,const StratSynth&);
			~uiStratSynthExport();


protected:

    uiGenInput*		crnewfld_;
    uiSeisSel*		linesetsel_;
    uiSeis2DLineSel*	linenmsel_;

    const StratSynth&	ss_;

    BufferString	getWinTitle(const StratSynth&) const;
    void		crNewChg(CallBacker*);
    bool		acceptOK(CallBacker*);

};

#endif

