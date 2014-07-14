#ifndef wellt2dtransform_h
#define wellt2dtransform_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          July 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "wellmod.h"
#include "zaxistransform.h"
#include "callback.h"


namespace Well { class Data; }

/*!
\brief Time to depth transform for wells.
*/

mExpClass(Well) WellT2DTransform : public ZAxisTransform
				 , public CallBacker
{ mODTextTranslationClass(WellT2DTransform);
public:

    mDefaultFactoryInstantiation( ZAxisTransform, WellT2DTransform,
				  "WellT2D", sFactoryKeyword() );

				WellT2DTransform();
				WellT2DTransform(const MultiID&);

    bool			isOK() const;
    void			transformTrc(const TrcKey&,
	    				  const SamplingData<float>&,
					  int sz,float* res) const;
    void			transformTrcBack(const TrcKey&,
	    				      const SamplingData<float>&,
					      int sz,float* res) const;
    bool			canTransformSurv(Pos::SurvID) const
				{ return true; }

    Interval<float>		getZInterval(bool time) const;
    bool			needsVolumeOfInterest() const { return false; }

    bool			setWellID(const MultiID&);

    bool			usePar(const IOPar&);

protected:

				~WellT2DTransform();

    Well::Data*			data_;
    TypeSet<float>		times_;
    TypeSet<float>		depths_;

    bool			calcDepths();
    void			doTransform(const SamplingData<float>&,
					    int sz,float*,bool) const;
    void			wellToBeDeleted(CallBacker*);
};

#endif

