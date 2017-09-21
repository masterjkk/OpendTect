#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		July 2017
 RCS:		$Id: $
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uidialog.h"
#include "filepath.h"

class uiSEGYImpType;
class uiTable;

mExpClass(uiSEGY) uiSEGYVintageInfo
{ mODTextTranslationClass(uiSEGYVintageInfo)
public:

    BufferString	vintagenm_;
    BufferStringSet	filenms_;
    File::Path		fp_;
};


mExpClass(uiSEGY) uiSEGYBulkImporter : public uiDialog
{ mODTextTranslationClass(uiSEGYBulkImporter)
public:
			uiSEGYBulkImporter(uiParent*);
			~uiSEGYBulkImporter();

    int			nrSelFiles() { return selfilenms_.size(); }

protected:
    bool		selectVintage();
    void		addCB(CallBacker*);
    void		removeCB(CallBacker*);
    void		selectFilesCB(CallBacker*);
    void		editVntCB(CallBacker*);
    void		fillRow( int rowid);
    bool		acceptOK();

    BufferStringSet	selfilenms_;
    uiTable*		table_;
    uiSEGYImpType*	imptypefld_;
    BufferString	vintagenm_;
    ObjectSet<uiSEGYVintageInfo> vntinfos_;
};