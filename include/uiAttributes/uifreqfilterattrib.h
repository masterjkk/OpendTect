#ifndef uifreqfilterattrib_h
#define uifreqfilterattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2001
 RCS:           $Id: uifreqfilterattrib.h,v 1.8 2008-04-30 03:13:16 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class uiImagAttrSel;
class uiGenInput;
class uiLabeledSpinBox;
class uiWindowFunctionSel;

/*! \brief ** Attribute description editor */

class uiFreqFilterAttrib : public uiAttrDescEd
{
public:

			uiFreqFilterAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiImagAttrSel*      inpfld;
    uiGenInput*         isfftfld;
    uiGenInput*		typefld;
    uiGenInput*		freqfld;
    uiLabeledSpinBox*	polesfld;
    uiWindowFunctionSel* winfld;

    void		finaliseCB(CallBacker*);
    void		typeSel(CallBacker*);
    void		isfftSel(CallBacker*);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};

#endif
