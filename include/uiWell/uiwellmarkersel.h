#ifndef uiwellmarkersel_h
#define uiwellmarkersel_h
/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bert
Date:          Aug 2012
RCS:           $Id: uiwellmarkersel.h,v 1.1 2012-08-27 11:45:36 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uigroup.h"
#include "bufstringset.h"
class uiComboBox;
namespace Well { class Marker; class MarkerSet; } 


/*!\brief Select one or two markers (i.e. a range) */

mDefClass(uiWell) uiWellMarkerSel : public uiGroup
{
public:

    mDefClass(uiWell) Setup
    {
    public:
			Setup(bool one,const char* sel_txt=0);

	mDefSetupMemb(bool,single);	//!< false = two levels (a zone)
	mDefSetupMemb(bool,allowsame);	//!< only useful when single == false
	mDefSetupMemb(bool,adddatabounds); //!< add 'virtual' start/stop levels
	mDefSetupMemb(BufferString,seltxt);
    };

			uiWellMarkerSel(uiParent*,const Setup&);

    void		setMarkers(const Well::MarkerSet&);
    void		setMarkers(const BufferStringSet&);

    void		setInput(const Well::Marker&,bool top=true);
    void		setInput(const char*,bool top=true);

    const char*		getText(bool top=true) const;
    int			getType(bool top=true) const;
				//!< -1=before-first, 0=marker, 1=after-last
    				//!< only useful if setup.adddatabounds

    static const char*	sKeyDataStart()		{ return "<Start of data>"; }
    static const char*	sKeyDataStop()		{ return "<End of data>"; }

protected:

    BufferStringSet	nms_;
    const Setup		setup_;
    uiComboBox*		topfld_;
    uiComboBox*		botfld_;

    void		setMarkers(uiComboBox&,const BufferStringSet&);
    void		mrkSel(CallBacker*);

};


#endif

