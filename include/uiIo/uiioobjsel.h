#ifndef uiioobjsel_h
#define uiioobjsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiioobjsel.h,v 1.9 2001-05-30 16:13:13 bert Exp $
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
			uiIOObjSelDlg(uiParent*,const CtxtIOObj&);
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
			uiIOObjSel(uiParent*,CtxtIOObj&,const char* txt=0,
				      bool withclear=false);

    void		updateInput();
    CtxtIOObj&		ctxtIOObj()		{ return ctio; }

protected:

    CtxtIOObj&		ctio;
    bool		forread;

    void		doObjSel(CallBacker*);
    virtual const char*	userNameFromKey(const char*) const;
    virtual void	objSel();

};


#endif
