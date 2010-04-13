/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          March 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: SoDGBDragPointDragger.cc,v 1.6 2010-04-13 07:28:03 cvsbert Exp $";

#include "SoDGBDragPointDragger.h"

#include <Inventor/SbRotation.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/projectors/SbLineProjector.h>
#include <Inventor/projectors/SbPlaneProjector.h>

// constraints on the plane for movement along 2 axes
#define CONSTRAINT_OFF  0
#define CONSTRAINT_WAIT 1
#define CONSTRAINT_X    2
#define CONSTRAINT_Y    3

SO_KIT_SOURCE(SoDGBDragPointDragger);

void SoDGBDragPointDragger::initClass()
{
    SO_KIT_INIT_CLASS(SoDGBDragPointDragger, SoDragger, "Dragger" );
}


SoDGBDragPointDragger::SoDGBDragPointDragger()
{
    SO_KIT_CONSTRUCTOR(SoDGBDragPointDragger);

    // noRotSep
    SO_KIT_ADD_CATALOG_ENTRY(noRotSep, SoSeparator, FALSE, topSeparator, 
	    rotXSep, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(xTranslatorSwitch, SoSwitch, FALSE, noRotSep, 
	    xyTranslatorSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(xTranslator, SoSeparator, TRUE, xTranslatorSwitch, 
	    "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(xyTranslatorSwitch, SoSwitch, FALSE, noRotSep, 
	    "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(xyTranslator, SoSeparator, TRUE, 
	    xyTranslatorSwitch, "", TRUE);

    // rotXSep
    SO_KIT_ADD_CATALOG_ENTRY(rotXSep, SoSeparator, FALSE, topSeparator, 
	    rotYSep, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(rotX, SoRotation, TRUE, rotXSep, 
	    xzTranslatorSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(xzTranslatorSwitch, SoSwitch, FALSE, rotXSep, 
	    "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(xzTranslator, SoSeparator, TRUE, 
	    xzTranslatorSwitch, "", TRUE);
    
    // rotYSep
    SO_KIT_ADD_CATALOG_ENTRY(rotYSep, SoSeparator, FALSE, topSeparator, 
	    rotZSep, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(rotY, SoRotation, TRUE, rotYSep, 
	    zTranslatorSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(zTranslatorSwitch, SoSwitch, FALSE, rotYSep, 
	    yzTranslatorSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(zTranslator, SoSeparator, TRUE, 
	    zTranslatorSwitch, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yzTranslatorSwitch, SoSwitch, FALSE, rotYSep, 
	    "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(yzTranslator, SoSeparator, TRUE, 
	    yzTranslatorSwitch, "", TRUE);
    
    // rotZSep
    SO_KIT_ADD_CATALOG_ENTRY(rotZSep, SoSeparator, FALSE, topSeparator, 
	    xFeedbackSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(rotZ, SoRotation, TRUE, rotZSep, 
	    yTranslatorSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(yTranslatorSwitch, SoSwitch, FALSE, rotZSep, 
	    "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(yTranslator, SoSeparator, TRUE, yTranslatorSwitch, 
	    "", TRUE);
    
    // X feedback
    SO_KIT_ADD_CATALOG_ENTRY(xFeedbackSwitch, SoSwitch, FALSE, topSeparator, 
	    yFeedbackSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(xFeedbackSep, SoSeparator, FALSE, xFeedbackSwitch, 
	    "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(xFeedbackTranslation, SoTranslation, FALSE, 
	    xFeedbackSep, xFeedback, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(xFeedback, SoSeparator, TRUE, xFeedbackSep, 
	    "", TRUE);
    
    // Y feedback
    SO_KIT_ADD_CATALOG_ENTRY(yFeedbackSwitch, SoSwitch, FALSE, topSeparator, 
	    zFeedbackSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(yFeedbackSep, SoSeparator, FALSE, yFeedbackSwitch, 
	    "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(yFeedbackTranslation, SoTranslation, FALSE, 
	    yFeedbackSep, yFeedback, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(yFeedback, SoSeparator, TRUE, yFeedbackSep, 
	    "", TRUE);
    
    // Z feedback
    SO_KIT_ADD_CATALOG_ENTRY(zFeedbackSwitch, SoSwitch, FALSE, topSeparator, 
	    planeFeedbackSep, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(zFeedbackSep, SoSeparator, FALSE, zFeedbackSwitch, 
	    "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(zFeedbackTranslation, SoTranslation, FALSE, 
	    zFeedbackSep, zFeedback, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(zFeedback, SoSeparator, TRUE, zFeedbackSep, 
	    "", TRUE);
    
    // Plane feedback
    SO_KIT_ADD_CATALOG_ENTRY(planeFeedbackSep, SoSeparator, FALSE, 
	    topSeparator, geomSeparator, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(planeFeedbackTranslation, SoTranslation, FALSE, 
	    planeFeedbackSep, planeFeedbackSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(planeFeedbackSwitch, SoSwitch, FALSE, 
	    planeFeedbackSep, "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(yzFeedback, SoSeparator, TRUE, 
	    planeFeedbackSwitch, xzFeedback, TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(xzFeedback, SoSeparator, TRUE, 
	    planeFeedbackSwitch, xyFeedback, TRUE);    
    SO_KIT_ADD_CATALOG_ENTRY(xyFeedback, SoSeparator, TRUE, 
	    planeFeedbackSwitch, "", TRUE);
    
    if (SO_KIT_IS_FIRST_INSTANCE())
    {
	SoInteractionKit::readDefaultParts("dragPointDragger.iv",
		draggergeometry_, static_cast<int>(strlen(draggergeometry_)));
    }

    SO_KIT_ADD_FIELD(translation, (0.0f, 0.0f, 0.0f));
    SO_KIT_INIT_INSTANCE();

    // initialize default parts pertaining to feedback
    this->setPartAsDefault("xFeedback", "dragPointXFeedback");
    this->setPartAsDefault("yFeedback", "dragPointYFeedback");
    this->setPartAsDefault("zFeedback", "dragPointZFeedback");
    this->setPartAsDefault("yzFeedback", "dragPointYZFeedback");
    this->setPartAsDefault("xzFeedback", "dragPointXZFeedback");
    this->setPartAsDefault("xyFeedback", "dragPointXYFeedback");

    // initialise the translators to inactive geometry states
    this->setPartAsDefault(linetranslatornames_[0],
		"dragPointXTranslatorTranslator");
    this->setPartAsDefault(linetranslatornames_[1],
		"dragPointYTranslatorTranslator");
    this->setPartAsDefault(linetranslatornames_[2],
		"dragPointZTranslatorTranslator");
    this->setPartAsDefault(planetranslatornames_[0],
		"dragPointYZTranslatorTranslator");
    this->setPartAsDefault(planetranslatornames_[1], 
		"dragPointXZTranslatorTranslator");
    this->setPartAsDefault(planetranslatornames_[2], 
		"dragPointXYTranslatorTranslator");
    
	// set rotations to align draggers to their respective axis/planes
    SoRotation* xrot = new SoRotation;
    xrot->rotation.setValue(SbRotation(
		SbVec3f(1.0f, 0.0f, 0.0f), (static_cast<float>(M_PI))*0.5f));
    this->setAnyPartAsDefault("rotX", xrot);
    SoRotation* yrot = new SoRotation;
    yrot->rotation.setValue(SbRotation(
		SbVec3f(0.0f, 1.0f, 0.0f), (static_cast<float>(M_PI))*0.5f));
    this->setAnyPartAsDefault("rotY", yrot);
    SoRotation* zrot = new SoRotation;
    zrot->rotation.setValue(SbRotation(
		SbVec3f(0.0f, 0.0f, 1.0f), (static_cast<float>(M_PI))*0.5f));
    this->setAnyPartAsDefault("rotZ", zrot);

    // initialize feedback switch nodes
    SoSwitch* sw;
    sw = SO_GET_ANY_PART(this, "planeFeedbackSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
    for ( int i = 0; i < 3; i++ )
    {
	sw = SO_GET_ANY_PART(this, linefbswitchnames_[i], SoSwitch);
	SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
    }
    
    // set up projector
    this->lineproj_ = new SbLineProjector();
    this->planeproj_ = new SbPlaneProjector();

    this->curraxis_ = 1;
    this->updateSwitchNodes();
    movecyl_ = false;

    this->addStartCallback(SoDGBDragPointDragger::startCB, this);
    this->addMotionCallback(SoDGBDragPointDragger::motionCB, this);
    this->addFinishCallback(SoDGBDragPointDragger::finishCB, this);
	//this->addOtherEventCallback(&SoDGBDragPointDragger::metaKeyChangeCB, this);
    this->addValueChangedCallback(SoDGBDragPointDragger::valueChangedCB);
    this->fieldSensor = new SoFieldSensor(
		SoDGBDragPointDragger::fieldSensorCB, this);
    this->fieldSensor->setPriority(0);

    this->constraintstate_ = CONSTRAINT_OFF;
    this->setUpConnections(TRUE, TRUE);
}


SoDGBDragPointDragger::~SoDGBDragPointDragger()
{
    delete this->fieldSensor;
    delete lineproj_;
    delete planeproj_;
}


SbBool SoDGBDragPointDragger::setUpConnections( SbBool onoff, 
	SbBool doitalways )
{
    if (!doitalways && this->connectionsSetUp == onoff) return onoff;

    SbBool oldval = this->connectionsSetUp;

    if (onoff) 
    {
	SoDragger::setUpConnections(onoff, doitalways);

	SoDGBDragPointDragger::fieldSensorCB(this, NULL);

	if (this->fieldSensor->getAttachedField() != &this->translation) 
	    this->fieldSensor->attach(&this->translation);
    }
    else 
    {
        if (this->fieldSensor->getAttachedField() != NULL)
	    this->fieldSensor->detach();
	SoDragger::setUpConnections(onoff, doitalways);
    }
   
    this->connectionsSetUp = onoff;
    return oldval;
}


void SoDGBDragPointDragger::setDefaultOnNonWritingFields()
{
    this->xTranslator.setDefault( TRUE );
    this->yTranslator.setDefault( TRUE );
    this->zTranslator.setDefault( TRUE );

    this->xyTranslator.setDefault( TRUE );
    this->xzTranslator.setDefault( TRUE );
    this->yzTranslator.setDefault( TRUE );

    this->planeFeedbackTranslation.setDefault (TRUE );
    this->xFeedbackTranslation.setDefault( TRUE );
    this->yFeedbackTranslation.setDefault( TRUE );
    this->zFeedbackTranslation.setDefault( TRUE );

    SoDragger::setDefaultOnNonWritingFields();
}


void SoDGBDragPointDragger::fieldSensorCB( void* d, SoSensor* )
{
    SoDGBDragPointDragger* thisp = static_cast<SoDGBDragPointDragger*>(d);
    SbMatrix matrix = thisp->getMotionMatrix();
    thisp->workFieldsIntoTransform(matrix);
    thisp->setMotionMatrix(matrix);
}


void SoDGBDragPointDragger::valueChangedCB( void*, SoDragger* d )
{
    SoDGBDragPointDragger* thisp = static_cast<SoDGBDragPointDragger*>(d);

    SbMatrix matrix = thisp->getMotionMatrix();
    SbVec3f t;
    t[0] = matrix[3][0];
    t[1] = matrix[3][1];
    t[2] = matrix[3][2];

    thisp->fieldSensor->detach();
    if (thisp->translation.getValue() != t)
	thisp->translation = t;

    thisp->fieldSensor->attach(&thisp->translation);
}


void SoDGBDragPointDragger::metaKeyChangeCB( void *, SoDragger *d )
{
    // check if plane dragger is active
    SoDGBDragPointDragger* thisp = static_cast<SoDGBDragPointDragger*>(d);
    if ( !thisp->isActive.getValue() && movecyl_ ) return;

    const SoEvent *event = thisp->getEvent();
    if ( thisp->constraintstate_ == CONSTRAINT_OFF &&
         event->wasShiftDown() )
        thisp->drag();
    else if ( thisp->constraintstate_ != CONSTRAINT_OFF &&
              !event->wasShiftDown() )
	thisp->drag();
}


// Circulate the dragger's three different sets of geometry, to circulate the 
// orientation of the translation axis and translation plane through the three
// principal axes.
void SoDGBDragPointDragger::showNextDraggerSet()
{
    this->curraxis_ = (this->curraxis_ + 1) % 3;
    this->updateSwitchNodes();
}


void SoDGBDragPointDragger::dragStart()
{
    // determine if the cylinder was picked or the cube
    SoCylinder* cyl = SO_GET_ANY_PART(this, linetranslatornames_[curraxis_], 
		SoCylinder);
    SoCube* cube = SO_GET_ANY_PART(this, planetranslatornames_[curraxis_], 
		SoCube);	
    
    if ( !cyl || !cube )
	return;
	
    // find the orientation of the dragger to the Z-axis
    SbViewVolume vw = getViewVolume();
    SbVec3f worldprojdir = vw.getProjectionDirection();
    const SbMatrix& mat = getWorldToLocalMatrix();
    SbVec3f localprojdir;
    mat.multDirMatrix( worldprojdir, localprojdir );
    localprojdir.normalize();

    const float angletox = fabs( localprojdir[0] );
    const float angletoy = fabs( localprojdir[1] );
    const float angletoz = fabs( localprojdir[2] );
    const float upperlimit = 0.8;
    const float lowerlimit = 0.2;
    
    // When the cylinder is lying flat (almost along the Z axis), restrict 
    // picking the cylinder. User probably wants to move just the rectangle 
    // but has picked the cylinder by mistake.
    //
    // When the cylinder is upright, restrict picking the rectangle. User 
    // probably wants to move just the cylinder but has picked the rectangle 
    // by mistake.

    // to do: check the conditions
    if ( ( angletox <= lowerlimit ) &&
	 ( angletoy >= upperlimit ) &&
	 ( angletoz <= lowerlimit ) )
	     movecyl_ = false;
    else if ( ( angletox >= upperlimit ) &&
	      ( angletoy >= upperlimit ) &&
	      ( angletoz <= lowerlimit ) )
	    movecyl_ = true;
    else
    {
        // Let the user drag as desired. Find which object the user has picked.
	const SoPath* pickpath = getPickPath();
	if ( pickpath->containsNode( cyl ) )
	    movecyl_ = true;
	else if ( pickpath->containsNode( cube ) )
	    movecyl_ = false;
	else	return;
    }
	
    SoSwitch* sw;
    if ( movecyl_ )
    {
        sw = SO_GET_ANY_PART(this, linefbswitchnames_[curraxis_], SoSwitch);
	SoInteractionKit::setSwitchValue( sw, 0 );

	// set the active part - to do
	if ( curraxis_ == 0 )
	    this->setPart( "dragPointXTranslatorTranslatorActive", cyl );
	else if ( curraxis_ == 1 )
	    this->setPart( "dragPointYTranslatorTranslatorActive", cyl );
	else if (curraxis_ == 2 )
	    this->setPart( "dragPointZTranslatorTranslatorActive", cyl );
			
	SbVec3f hitpt = this->getLocalStartingPoint();
	SbVec3f endpt( 0.0f, 0.0f, 0.0f );
	endpt[curraxis_] = 1.0f;
	lineproj_->setLine( SbLine( hitpt, hitpt + endpt ) );
    }
    else
    {
	sw = SO_GET_ANY_PART(this, "planeFeedbackSwitch", SoSwitch);
	SoInteractionKit::setSwitchValue( sw, this->curraxis_ );

	// set the active part - to do
	const char* activepartname = "";
	if ( curraxis_ == 0 )
	    activepartname = "dragPointYZTranslatorTranslatorActive";
	else if ( curraxis_ == 1 )
	    activepartname = "dragPointXZTranslatorTranslatorActive";
	else if (curraxis_ == 2 )
	    activepartname = "dragPointXYTranslatorTranslatorActive";
			
	this->setPart( activepartname, cube );

	SbVec3f hitpt = this->getLocalStartingPoint();
	SbVec3f endpt( 0.0f, 0.0f, 0.0f );
	endpt[curraxis_] = 1.0f;
	planeproj_->setPlane( SbPlane( endpt, hitpt ) );
	if ( this->getEvent()->wasShiftDown() )
	{
	    this->getLocalToWorldMatrix().multVecMatrix( 
			    hitpt, this->worldrestartpt_ );
	    this->constraintstate_ = CONSTRAINT_WAIT;
	}
		
	this->extramotion_ = SbVec3f(0, 0, 0);
    }
}


// to do? Extra feedback for moving plane along individual axes
void SoDGBDragPointDragger::drag()
{
    if ( movecyl_ )
    {
        lineproj_->setViewVolume( this->getViewVolume() );
	lineproj_->setWorkingSpace( this->getLocalToWorldMatrix() );
  
	const float epsilon = this->getProjectorEpsilon();
	SbVec3f projpt;
	
	if ( lineproj_->tryProject( this->getNormalizedLocaterPosition(), 
			epsilon, projpt ) )
	{
	    SbVec3f startpt = this->getLocalStartingPoint();
	    SbVec3f motion = projpt - startpt;
	    SbMatrix mm = this->appendTranslation( 
			    this->getStartMotionMatrix(), motion );
	    this->setMotionMatrix( mm );
	}
    }
    else
    {
	this->planeproj_->setViewVolume( this->getViewVolume() );
	this->planeproj_->setWorkingSpace( this->getLocalToWorldMatrix() );

	SbVec3f projpt;
	if (this->planeproj_->tryProject( this->getNormalizedLocaterPosition(),
				this->getProjectorEpsilon(), projpt ) )
	{
	    const SoEvent *event = this->getEvent();
	    if ( event->wasShiftDown() && 
		 this->constraintstate_ == CONSTRAINT_OFF )
	    {
		this->constraintstate_ = CONSTRAINT_WAIT;
		this->setStartLocaterPosition( event->getPosition() );
		this->getLocalToWorldMatrix().multVecMatrix( projpt, 
				this->worldrestartpt_ );
            }
	    else if ( !event->wasShiftDown() && 
		      this->constraintstate_ != CONSTRAINT_OFF )
	    {
		SbVec3f worldprojpt;
		this->getLocalToWorldMatrix().multVecMatrix( projpt, 
				worldprojpt );
		this->setStartingPoint( worldprojpt );
		this->extramotion_ += this->lastmotion_;
      
		/*SoSwitch *sw = SO_GET_ANY_PART(
		 this, "axisFeedbackSwitch", SoSwitch);
		 SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);*/
		this->constraintstate_ = CONSTRAINT_OFF;
	    }
    
	    SbVec3f startpt = this->getLocalStartingPoint();
	    SbVec3f motion;
	    SbVec3f localrestartpt;
    
	    if ( this->constraintstate_ != CONSTRAINT_OFF )
	    {
		this->getWorldToLocalMatrix().multVecMatrix(
				this->worldrestartpt_, localrestartpt );
		motion = localrestartpt - startpt;
	    }
	    else 
		motion = projpt - startpt;
    
	    switch( this->constraintstate_ )
	    {
		case CONSTRAINT_OFF:
		    break;

	        case CONSTRAINT_WAIT:
		    if ( this->isAdequateConstraintMotion() )
		    {
			SbVec3f newmotion = projpt - localrestartpt;
			if ( fabs( newmotion[0] ) >= fabs( newmotion[1] ) )
			{
			    this->constraintstate_ = CONSTRAINT_X;
			    motion[0] += newmotion[0];
			    /*SoSwitch *sw = SO_GET_ANY_PART(
			     this, "axisFeedbackSwitch", SoSwitch);
			     SoInteractionKit::setSwitchValue(sw, 0);*/
		        }
			else 
			{
			    this->constraintstate_ = CONSTRAINT_Y;
			    motion[1] += newmotion[1];
			    /*SoSwitch *sw = SO_GET_ANY_PART(
			     this, "axisFeedbackSwitch", SoSwitch);
			     SoInteractionKit::setSwitchValue(sw, 1);*/
        		}
      		    }
      		    else
			return;
      		break;
	    
	    case CONSTRAINT_X:
	        motion[0] += projpt[0] - localrestartpt[0];
		break;

	    case CONSTRAINT_Y:
		motion[1] += projpt[1] - localrestartpt[1];
		break;
	    }

	    this->lastmotion_ = motion;
	    this->setMotionMatrix( this->appendTranslation( 
		this->getStartMotionMatrix(), this->extramotion_+motion ) );
        }
    }
}


void SoDGBDragPointDragger::dragFinish()
{
    SoSwitch* sw;
    
    if ( movecyl_ )
    {
	sw = SO_GET_ANY_PART(this, linefbswitchnames_[curraxis_], SoSwitch);
	SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
	    
	// set the inactive part
	const char* inactivepartname = "";
	if ( curraxis_ == 0 )
	    inactivepartname = "dragPointXTranslatorTranslator";
	else if ( curraxis_ == 1 )
	    inactivepartname = "dragPointYTranslatorTranslator";
	else if (curraxis_ == 2 )
	    inactivepartname = "dragPointZTranslatorTranslator";
			
	SoNode* node = SO_GET_ANY_PART(this, linetranslatornames_[curraxis_], 
			SoCylinder );
	this->setPart( inactivepartname, node );
    }
    else
    {
	sw = SO_GET_ANY_PART(this, "planeFeedbackSwitch", SoSwitch);
	SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);

	// set the inactive part
	const char* inactivepartname = "";
	if ( curraxis_ == 0 )
	    inactivepartname = "dragPointYZTranslatorTranslator";
	else if ( curraxis_ == 1 )
	    inactivepartname = "dragPointXZTranslatorTranslator";
	else if (curraxis_ == 2 )
	    inactivepartname = "dragPointXYTranslatorTranslator";
			
	SoNode* node = SO_GET_ANY_PART(this, planetranslatornames_[curraxis_], 
			SoCube );
	this->setPart( inactivepartname, node );

	this->constraintstate_ = CONSTRAINT_OFF;
    }
}


void SoDGBDragPointDragger::startCB( void* d, SoDragger* )
{
    SoDGBDragPointDragger* thisp = static_cast<SoDGBDragPointDragger* >(d);
    thisp->dragStart();
}


void SoDGBDragPointDragger::motionCB( void* d, SoDragger* )
{
     SoDGBDragPointDragger* thisp = static_cast<SoDGBDragPointDragger* >(d);
     thisp->drag();
}


void SoDGBDragPointDragger::finishCB( void* d, SoDragger* )
{
    SoDGBDragPointDragger* thisp = static_cast<SoDGBDragPointDragger* >(d);
    thisp->dragFinish();
}


void SoDGBDragPointDragger::updateSwitchNodes()
{
    // switch the dragger geometry
    SoSwitch *sw;
    
    sw = SO_GET_ANY_PART(this, "xTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, this->curraxis_ == 0 ? 
	    0 : SO_SWITCH_NONE);
    sw = SO_GET_ANY_PART(this, "yTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, this->curraxis_ == 1 ? 
	    0 : SO_SWITCH_NONE);
    sw = SO_GET_ANY_PART(this, "zTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, this->curraxis_ == 2 ? 
	    0 : SO_SWITCH_NONE);

    sw = SO_GET_ANY_PART(this, "yzTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, this->curraxis_ == 0 ? 
	    0 : SO_SWITCH_NONE);
    sw = SO_GET_ANY_PART(this, "xzTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, this->curraxis_ == 1 ? 
	    0 : SO_SWITCH_NONE);
    sw = SO_GET_ANY_PART(this, "xyTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, this->curraxis_ == 2 ? 
	    0 : SO_SWITCH_NONE);
}


const char* SoDGBDragPointDragger::linefbswitchnames_[] = 
{ "xFeedbackSwitch", "yFeedbackSwitch", "zFeedbackSwitch" };

const char* SoDGBDragPointDragger::linetranslatornames_[] = 
{ "xTranslator", "yTranslator", "zTranslator" };

const char* SoDGBDragPointDragger::planetranslatornames_[] = 
{ "yzTranslator", "xzTranslator", "xyTranslator" };

const char* SoDGBDragPointDragger::draggergeometry_ =
  "#Inventor V2.1 ascii\n"
  "\n"
  "\n"
  "\n"
  "DEF DRAGPOINT_INACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0.5  emissiveColor 0.5 0.5 0.5 }\n"
  "DEF DRAGPOINT_ACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0  emissiveColor 0.5 0.5 0 }\n"
  "DEF DRAGPOINT_FEEDBACK_MATERIAL Material { diffuseColor 0.5 0 0.5  emissiveColor  0.5 0 0.5  transparency 0.2 }\n"
  "\n"
  "\n"
  "\n"
  "DEF DRAGPOINT_STICK Group {\n"
  "   Rotation { rotation 0 0 1  1.57 }\n"
  "   Cylinder { height 1.5 radius 0.2 }\n"
  "}\n"
  "\n"
  "DEF DRAGPOINT_INACTIVE_STICK Separator {\n"
  "   USE DRAGPOINT_INACTIVE_MATERIAL\n"
  "   USE DRAGPOINT_STICK\n"
  "}\n"
  "DEF DRAGPOINT_ACTIVE_STICK Separator {\n"
  "   USE DRAGPOINT_ACTIVE_MATERIAL\n"
  "   USE DRAGPOINT_STICK\n"
  "}\n"
  "\n"
  "DEF dragPointXTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_STICK } \n"
  "DEF dragPointXTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_STICK }\n"
  "DEF dragPointYTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_STICK }\n"
  "DEF dragPointYTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_STICK }\n"
  "DEF dragPointZTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_STICK }\n"
  "DEF dragPointZTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_STICK }\n"
  "\n"
  "\n"
  "DEF DRAGPOINT_PLANE Group { Cube { width 1  height 1  depth .1 } }\n"
  "\n"
  "DEF DRAGPOINT_INACTIVE_PLANE Separator {\n"
  "   USE DRAGPOINT_INACTIVE_MATERIAL\n"
  "   USE DRAGPOINT_PLANE\n"
  "}\n"
  "DEF DRAGPOINT_ACTIVE_PLANE Separator {\n"
  "   USE DRAGPOINT_ACTIVE_MATERIAL\n"
  "   USE DRAGPOINT_PLANE\n"
  "}\n"
  "\n"
  "DEF dragPointXYTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_PLANE }\n"
  "DEF dragPointXYTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_PLANE }\n"
  "DEF dragPointXZTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_PLANE }\n"
  "DEF dragPointXZTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_PLANE }\n"
  "DEF dragPointYZTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_PLANE }\n"
  "DEF dragPointYZTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_PLANE }\n"
  "\n"
  "\n"
  "DEF DRAGPOINT_FEEDBACK_LINE Group {\n"
  "   Coordinate3 { point [ 0 -10 0, 0 10 0 ] }\n"
  "   LineSet { }\n"
  "\n"
  "   Transform { translation 0 10 0 }\n"
  "   DEF DRAGPOINT_FEEDBACK_ARROWHEAD Cone { height 0.5 bottomRadius 0.5 }\n"
  "   Transform { translation 0 -20 0 }\n"
  "   Rotation { rotation 0 0 1  3.14 }\n"
  "   USE DRAGPOINT_FEEDBACK_ARROWHEAD\n"
  "}\n"
  "\n"
  "DEF dragPointXFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   Rotation { rotation 0 0 1 1.57 }\n"
  "   USE DRAGPOINT_FEEDBACK_LINE\n"
  "}\n"
  "DEF dragPointYFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   USE DRAGPOINT_FEEDBACK_LINE\n"
  "}\n"
  "DEF dragPointZFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   Rotation { rotation 1 0 0 1.57 }\n"
  "   USE DRAGPOINT_FEEDBACK_LINE\n"
  "}\n"
  "\n"
  "\n"
  "DEF DRAGPOINT_FEEDBACK_PLANE Group {\n"
  "   ShapeHints { shapeType UNKNOWN_SHAPE_TYPE }\n"
  "   Coordinate3 { point [ -10 0 -10, -10 0 10, 10 0 10, 10 0 -10, -10 0 -10 ] }\n"
  "   FaceSet { }\n"
  "   Scale { scaleFactor 1.05 1 1.05 }\n"
  "   LineSet { }\n"
  "}\n"
  "\n"
  "DEF dragPointXYFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   Rotation { rotation 1 0 0  1.57 }\n"
  "   USE DRAGPOINT_FEEDBACK_PLANE\n"
  "}\n"
  "DEF dragPointXZFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   USE DRAGPOINT_FEEDBACK_PLANE\n"
  "}\n"
  "DEF dragPointYZFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   Rotation { rotation 0 0 1  1.57 }\n"
  "   USE DRAGPOINT_FEEDBACK_PLANE\n"
  "}\n";

