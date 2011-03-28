#ifndef visvw2dpickset_h
#define visvw2dpickset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Ranojay Sen
 Date:		Mar 2011
 RCS:		$Id: visvw2dpickset.h,v 1.2 2011-03-28 04:39:26 cvsranojay Exp $
________________________________________________________________________

-*/

#include "flatview.h"
#include "flatauxdataeditor.h"

#include "visvw2ddata.h"

class uiFlatViewer;
class uiFlatViewAuxDataEditor;
class CubeSampling;
namespace Pick { class Set; }


mClass VW2DPickSet : public Vw2DDataObject
{
public:
			VW2DPickSet(Pick::Set&,
			    const ObjectSet<uiFlatViewAuxDataEditor>&);
			~VW2DPickSet();
     
    void		drawAll();
    void		clearPicks();
    void		enablePainting(bool yn);
    void		selected();

protected:

    Coord3		getCoord(const FlatView::Point&) const;
    void		pickAddChgCB(CallBacker*);
    void		pickRemoveCB(CallBacker*);
    void		dataChangedCB(CallBacker*);
    MarkerStyle2D	get2DMarkers(const Pick::Set& ps) const;
    void		triggerDeSel();
    void		updateSetIdx(const CubeSampling&);


    Pick::Set&			    pickset_;
    FlatView::Annotation::AuxData*  picks_;
    uiFlatViewAuxDataEditor*	    editor_;
    uiFlatViewer&		    viewer_;
    int				    auxid_;
    bool			    isselected_;
    Notifier<VW2DPickSet>	    deselected_;
    bool			    isownremove_;
    TypeSet<int>		    picksetidxs_;
};

#endif
