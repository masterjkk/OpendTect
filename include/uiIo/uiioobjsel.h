#ifndef uiioobjsel_h
#define uiioobjsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiioobjsel.h,v 1.6 2001-05-08 08:20:40 bert Exp $
________________________________________________________________________

-*/

#include <uiiosel.h>
#include <uidialog.h>
class IOObj;
class CtxtIOObj;
class IODirEntryList;
class uiListBox;
class uiGenInput;

/*! \brief Dialog for selection of IOObjs */

class uiIOObjSelDlg : public uiDialog
{
public:
			uiIOObjSelDlg(uiObject*,const CtxtIOObj&);
			~uiIOObjSelDlg();

    const IOObj*	ioObj() const		{ return ioobj; }

protected:

    const CtxtIOObj&	ctio;
    IODirEntryList*	entrylist;
    IOObj*		ioobj;

    uiListBox*		listfld;
    uiGenInput*		nmfld;

    bool		acceptOK(CallBacker*);
    void		selChg(CallBacker*);
};


/*! \brief UI element for selection of IOObjs */

class uiIOObjSel : public uiIOSelect
{
public:
			uiIOObjSel(uiObject*,CtxtIOObj&,const char* txt=0,
				      bool withclear=false);

    CtxtIOObj&		ctxtIOObj()		{ return ctio; }

protected:

    CtxtIOObj&		ctio;
    bool		forread;

    void		updateInput();
    void		doObjSel(CallBacker*);
    virtual const char*	userNameFromKey(const char*) const;

};


#endif
