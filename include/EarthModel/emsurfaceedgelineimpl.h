#ifndef emsurfaceedgelineimpl_h
#define emsurfaceedgelineimpl_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emsurfaceedgelineimpl.h,v 1.2 2004-09-03 13:18:46 kristofer Exp $
________________________________________________________________________


-*/

#include "emsurfaceedgeline.h"
template <class T> class MathFunction;

namespace EM
{


class TerminationEdgeLineSegment : public EdgeLineSegment
{
public:
		    mEdgeLineSegmentClone(TerminationEdgeLineSegment, TermLine);
		    TerminationEdgeLineSegment( EM::Surface& surf,
						const EM::SectionID& sect )
			: EdgeLineSegment( surf, sect ) {}

    virtual bool    shouldTrack(int) const { return false; }
};


class SurfaceConnectLine : public EdgeLineSegment
{
public:
    			mEdgeLineSegmentClone(SurfaceConnectLine,ConnLine);
			SurfaceConnectLine( EM::Surface& surf,
					    const EM::SectionID& sect )
			    : EdgeLineSegment( surf, sect ) {}
    virtual bool	shouldTrack(int) const { return false; }
    bool		isNodeOK(const RowCol&);

    void		setConnectingSection(const EM::SectionID& ns )
			{ connectingsection = ns; }
    EM::SectionID	connectingSection() const { return connectingsection; }

protected:
    EM::SectionID	connectingsection;
    bool	internalIdenticalSettings(const EM::EdgeLineSegment&) const;

};


class SurfaceCutLine : public EdgeLineSegment
{
public:
    			mEdgeLineSegmentClone(SurfaceCutLine,CutLine);
			SurfaceCutLine( EM::Surface&, const EM::SectionID& );

    bool		canTrack() const { return cuttingsurface; }
    const EM::Surface*	cuttingSurface() const { return cuttingsurface; }
    virtual bool	shouldTrack(int) const;
    virtual bool	shouldExpand() const { return true; }

    static float	getMeshDist();

    bool		trackWithCache( int, bool, const EdgeLineSegment*,
					const EdgeLineSegment* );

    virtual void	setTime2Depth( const MathFunction<float>* f) { t2d=f; }
    void		setCuttingSurface( const EM::Surface* cs, bool pos )
			{ cuttingsurface = cs; cutonpositiveside = pos; }

    bool		isNodeOK(const RowCol&);

    static
    SurfaceCutLine*	createCutFromEdges( EM::Surface& surface,
					    const EM::SectionID& section,
					    int relidx,
					    const MathFunction<float>* t2d );
    			/*!< Creates a cutline from a surface relation.
			     \variable relidx	Surface relation that
			     			will be tracked.
			*/

    static
    SurfaceCutLine*	createCutFromSeed( EM::Surface& surface,
					   const EM::SectionID& section,
					   int relidx, const RowCol& seed,
					   bool boothdirs,
					   const MathFunction<float>* t2d );
    			/*!< Creates a cutline from a surface relation.
			     \variable relidx	Surface relation that
			     			will be tracked.
			*/
    static void		computeDistancesAlongLine( const EM::EdgeLine&,
					       const EM::Surface&,
					       const  MathFunction<float>* t2d,
					       TypeSet<RowCol>&,
					       TypeSet<float>&,
					       bool negate,
					       bool usecaching );

protected:
    bool	internalIdenticalSettings(const EM::EdgeLineSegment&) const;
    void		removeCache();
    void		commitChanges();
    bool		isAtCuttingEdge(int idx) const;

    float		computeScore( const RowCol&,
	    			      bool& changescorepos,
				      Coord3& scorepos );
    
    TypeSet<RowCol>	cacherc;
    TypeSet<Coord3>	poscache;
    TypeSet<float>	distcache;
    BoolTypeSet		ischanged;
    const float		meshdist;

    const MathFunction<float>*	t2d;
    const EM::Surface*	cuttingsurface;
    bool		cutonpositiveside;
};


};

#endif
