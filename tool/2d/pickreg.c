/*
 *
 * Functionality for 2D rendering
 *
 * Copyright (c) 2001 Criterion Software Ltd.
 */

/****************************************************************************
 *                                                                          *
 *  Module  :   pickreg.c                                                   *
 *                                                                          *
 *  Purpose :   Pick regions                                                *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 Includes
 */
#include "rwcore.h"
#include "rpdbgerr.h"
#include "rpworld.h"

#include "rt2d.h"
#include "object.h"
#include "font.h"
#include "path.h"
#include "tri.h"
#include "gstate.h"
#include "pickreg.h"

/*-----------------20/08/2001 10:20 AJH -------------
 * Is that really needed ???
 * --------------------------------------------------*/
#if defined (__MWERKS__)
#if (defined(RWVERBOSE))
#pragma message (__FILE__ "/" _SKY_EXPAND(__LINE__) ": __MWERKS__ == " _SKY_EXPAND(__MWERKS__))
#endif /* (defined (__MWERKS__)) */
#if (__option (global_optimizer))
#pragma always_inline off
#endif /* (__option (global_optimizer)) */
#endif /*  defined (__MWERKS__) */

/****************************************************************************
 Local Types
 */

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Defines
 */

/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   Rt2dPickRegion

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/**
 * \ingroup rt2dpickregion
 * \page rt2dpickregionoverview Rt2dPickRegion Overview
 *
 * A pick region is an invisible area on a screen. Pick regions can be used
 * as buttons and clickable areas on a screen. Pick regions can also be used
 * in conjunction with shapes and added to a scene.
 *
 * \par Creating a Pick Region
 * -# \ref Rt2dPickRegionCreate creates the pick region.
 * -# \ref Rt2dPickRegionGetPath returns the path.
 *
 * \par Adding PickRegions to Scenes
 * -# \ref Rt2dSceneLock locks the scene.
 * -# \ref Rt2dSceneAddChild adds the pick region to the scene.
 * -# \ref Rt2dSceneUnlock unlocks the scene.
 *
 * \see Rt2dSceneCreate
 */

/**
 * \ingroup rt2dpickregion
 * \ref Rt2dPickRegionCreate creates a pick region object.
 * To create a new pick region:
 *
 * -# \ref Rt2dPickRegionCreate creates the pick region.
 * -# \ref Rt2dPickRegionGetPath creates the path.
 *
 * \return Returns the pick regions if successful, NULL otherwise.
 *
 * \see Rt2dPickRegionDestroy
 * \see Rt2dPickRegionGetPath
 */
Rt2dObject  *
Rt2dPickRegionCreate(void)
{
    Rt2dObject           *object;

    RWAPIFUNCTION(RWSTRING("Rt2dPickRegionCreate"));

    object = _rt2dObjectCreate();
    RWASSERT(object);

    _rt2dObjectInit(object, rt2DOBJECTTYPEPICKREGION);

    _rt2dPickRegionInit(object);

    RWRETURN(object);
}

Rt2dObject *
_rt2dPickRegionInit(Rt2dObject *pickRegion)
{
    RWFUNCTION(RWSTRING("_rt2dPickRegionInit"));
    RWASSERT(pickRegion);

    pickRegion->data.pickRegion.path = Rt2dPathCreate();
    RWASSERT(pickRegion->data.pickRegion.path);

    /* Pick regions are always locked */
    pickRegion->flag &= ~Rt2dObjectIsLocked;

    RWRETURN(pickRegion);
}

RwBool
_rt2dPickRegionDestruct(Rt2dObject *object)
{
    RwBool result;
    RWFUNCTION(RWSTRING("_rt2dPickRegionDestruct"));

    RWASSERT(object);
    RWASSERT(rt2DOBJECTTYPEPICKREGION == object->type);

    result = Rt2dPathDestroy(object->data.pickRegion.path);

    RWRETURN(result);
}

/**
 * \ingroup rt2dpickregion
 * \ref Rt2dPickRegionDestroy
 * Destroys a pick region object.
 *
 * \param object is the pick region to destroy.
 *
 * \return Returns TRUE if successful, FALSE otherwise.
 *
 * \see Rt2dPickRegionCreate
 */
RwBool
Rt2dPickRegionDestroy(Rt2dObject *object)
{
    RwBool result;

    RWAPIFUNCTION(RWSTRING("Rt2dPickRegionDestroy"));
    RWASSERT(object);
    RWASSERT( rt2DOBJECTTYPEPICKREGION == object->type);

    /* NULL path is valid */
    result = (NULL != object);
    if (result)
    {
        /* Destruct contents */
        result=_rt2dPickRegionDestruct(object);

        /* Destroy base object */
        RwFreeListFree(Rt2dGlobals.objectFreeList, object);
    }

    RWRETURN(result);
}

/**
 * \ingroup rt2dpickregion
 * \ref Rt2dPickRegionGetPath
 * Returns the local definition of the pick region path.
 *
 * \param pickRegion is the pickRegion to obtain the path from
 *
 * \return Returns the pick region path if successful, NULL otherwise.
 *
 * \see Rt2dPickRegionIsPointIn
 * \see Rt2dPickRegionCreate
 */
Rt2dPath *
Rt2dPickRegionGetPath(Rt2dObject *pickRegion)
{
    RWAPIFUNCTION(RWSTRING("Rt2dPickRegionGetPath"));
    RWASSERT(pickRegion);
    RWASSERT(rt2DOBJECTTYPEPICKREGION == pickRegion->type );

    RWRETURN(pickRegion->data.pickRegion.path);
}

/**
 * \ingroup rt2dpickregion
 * \ref Rt2dPickRegionIsPointIn
 * Checks if a point is in the pick region. The last 'render' operation
 * provides the background for the positioning of the pick region.
 *
 * \param pickRegion is the pickRegion to check the point against
 * \param point is the point to check
 *
 * \return returns TRUE if the point is in the given pick region,
 * FALSE otherwise
 *
 * \see Rt2dPickRegionGetPath
 */
RwBool
Rt2dPickRegionIsPointIn(Rt2dObject *pickRegion, RwV2d *point)
{
    RwV2d localPoint;
    RwV3d pointIn3d;
    RwV3d localPointIn3d;

    RWAPIFUNCTION(RWSTRING("Rt2dPickRegionIsPointIn"));
    RWASSERT(pickRegion);
    RWASSERT(rt2DOBJECTTYPEPICKREGION == pickRegion->type );
    RWASSERT(point);
    RWASSERT( Rt2dObjectDirtyLTM != (Rt2dObjectDirtyLTM & pickRegion->flag));

    if( (pickRegion->flag & Rt2dObjectVisible) == Rt2dObjectVisible )
    {
        /* First transform the point into 3d */
        pointIn3d.x = point->x;
        pointIn3d.y = point->y;
        pointIn3d.z = 0.0f;

        /* Transform the point into local coordinates */
        RwV3dTransformPoint( &localPointIn3d, &pointIn3d,
                            &pickRegion->data.pickRegion.transformation );

        /* Back to 2d */
        localPoint.x = localPointIn3d.x;
        localPoint.y = localPointIn3d.y;

        /* Do the check */
        RWRETURN(RwV2dIsInPath(&localPoint, pickRegion->data.pickRegion.path));
    }

    RWRETURN(FALSE);
}


Rt2dObject *
_rt2dPickRegionStreamWrite(Rt2dObject *pickRegion, RwStream *stream)
{
    RWFUNCTION(RWSTRING("_rt2dPickRegionStreamWrite"));
    RWASSERT(pickRegion);
    RWASSERT(rt2DOBJECTTYPEPICKREGION == pickRegion->type );
    RWASSERT(stream);

    /* Write base class information */
    if (!_rt2dObjectBaseStreamWrite(pickRegion, stream))
    {
        RWRETURN((Rt2dObject*)NULL);
    }

    /* Write path */
    if (!Rt2dPathStreamWrite(pickRegion->data.pickRegion.path, stream))
    {
        RWRETURN((Rt2dObject *)NULL);
    }

    RWRETURN(pickRegion);
}


/**
 * \ingroup rt2dpickregion
 * \ref Rt2dPickRegionStreamWrite is used to write the specified pick
 * region to the given binary stream. Note that the stream will have been
 * opened prior to this function call.
 *
 * \param pickRegion is the pickRegion to stream
 * \param stream is the stream to write to
 *
 * \return returns the pick region on success, NULL otherwise
 *
 * \see Rt2dPickRegionStreamGetSize
 * \see Rt2dPickRegionStreamRead
 * \see RwStreamOpen
 * \see RwStreamClose
 */
Rt2dObject *
Rt2dPickRegionStreamWrite(Rt2dObject *pickRegion, RwStream *stream)
{
    RwUInt32 size;

    RWAPIFUNCTION(RWSTRING("Rt2dPickRegionStreamWrite"));
    RWASSERT(pickRegion);
    RWASSERT(rt2DOBJECTTYPEPICKREGION == pickRegion->type );
    RWASSERT(stream);

    /* Write header */
    size = Rt2dPickRegionStreamGetSize(pickRegion);
    if (!RwStreamWriteChunkHeader(stream, rwID_2DPICKREGION, size))
    {
        RWRETURN((Rt2dObject *)NULL);
    }

    /* Write body */
    _rt2dPickRegionStreamWrite(pickRegion, stream);

    RWRETURN(pickRegion);
}

RwUInt32
_rt2dPickRegionStreamGetSize(Rt2dObject *pickRegion)
{
    RwUInt32 size;

    RWFUNCTION(RWSTRING("_rt2dPickRegionStreamGetSize"));
    RWASSERT(pickRegion);
    RWASSERT(rt2DOBJECTTYPEPICKREGION == pickRegion->type );

    /* Scene is the size of the base class data */
    size = _rt2dObjectBaseStreamGetSize(pickRegion);

    /* ...plus the size of the path */
    size += Rt2dPathStreamGetSize(pickRegion->data.pickRegion.path);

    RWRETURN(size);
}

/**
 * \ingroup rt2dpickregion
 * \ref Rt2dPickRegionStreamGetSize is used to determine the size in bytes of
 * the binary representation of the given pick region. This value is used in
 * the binary chunk header to indicate the size of the chunk. The size
 * does include the size of the chunk header.
 *
 * \param pickRegion the pick region to obtain the size of
 *
 * \return Returns a \ref RwUInt32 value equal to the chunk size (in
 * bytes) of the shape, NULL otherwise.
 *
 * \see Rt2dPickRegionStreamWrite
 * \see Rt2dPickRegionStreamRead
 */
RwUInt32
Rt2dPickRegionStreamGetSize(Rt2dObject *pickRegion)
{
    RwUInt32 size;

    RWAPIFUNCTION(RWSTRING("Rt2dPickRegionStreamGetSize"));
    RWASSERT(pickRegion);
    RWASSERT(rt2DOBJECTTYPEPICKREGION == pickRegion->type );

    /* Scene is the size of the chunk header... */
    size =  rwCHUNKHEADERSIZE;

    /* ...plus the size of the body */
    size += _rt2dPickRegionStreamGetSize(pickRegion);

    RWRETURN(size);
}


Rt2dObject *
_rt2dPickRegionStreamReadTo(Rt2dObject *pickRegion, RwStream *stream)
{
    RwUInt32 size, version;

    RWFUNCTION(RWSTRING("_rt2dPickRegionStreamReadTo"));
    RWASSERT(pickRegion);
    RWASSERT(rt2DOBJECTTYPEPICKREGION == pickRegion->type );
    RWASSERT(stream);

    _rt2dPickRegionInit(pickRegion);

    /* Get path */
    if (!RwStreamFindChunk(stream, rwID_2DPATH, &size, &version))
    {
        RWRETURN((Rt2dObject *)NULL);
    }

    if (!_rt2dPathStreamReadTo(pickRegion->data.pickRegion.path, stream))
    {
        RWRETURN((Rt2dObject *)NULL);
    }

    RWRETURN(pickRegion);
}

/**
 * \ingroup rt2dpickregion
 * \ref Rt2dPickRegionStreamRead is used to read a pick region from
 * the specified binary stream. Note that prior to this function call,
 * a binary pick region chunk must be found in the stream using the
 * \ref RwStreamFindChunk API function.
 *
 * The sequence to locate and read a pick region from a binary stream is
 * as follows:
 * \code
   RwStream *stream;
   Rt2dObject *newPickRegion;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_2DPICKREGION, NULL, NULL) )
       {
           newPickRegion = Rt2dPickRegionStreamRead(stream);
       }

       RwStreamClose(stream, NULL);
   }
   \endcode
 *
 * \param stream is the stream to read from
 *
 * \return returns the pick region on success, NULL otherwise
 *
 * \see Rt2dPickRegionStreamWrite
 * \see Rt2dPickRegionStreamGetSize
 * \see RwStreamClose
 * \see RwStreamFindChunk
 */
Rt2dObject *
Rt2dPickRegionStreamRead(RwStream *stream)
{
    Rt2dObject *pickRegion = _rt2dObjectCreate();

    RWAPIFUNCTION(RWSTRING("Rt2dPickRegionStreamRead"));
    RWASSERT(stream);
    RWASSERT(pickRegion);

    if (!_rt2dObjectBaseStreamReadTo(pickRegion, stream))
    {
        RWRETURN((Rt2dObject*)NULL);
    }

    if (!_rt2dPickRegionStreamReadTo(pickRegion, stream))
    {
        RWRETURN((Rt2dObject*)NULL);
    }

    RWRETURN(pickRegion);
}

Rt2dObject *
_rt2dPickRegionRender(Rt2dObject *object)
{
    RWFUNCTION(RWSTRING("_rt2dPickRegionRender"));
    RWASSERT(object);
    RWASSERT(object->type == rt2DOBJECTTYPEPICKREGION);

    /* Object level stuff */
    if( (object->flag & Rt2dObjectVisible) == Rt2dObjectVisible )
    {
        if((object->flag & Rt2dObjectDirtyLTM) == Rt2dObjectDirtyLTM)
        {
            _rt2dCTMPush(&object->MTM);
            RwMatrixCopy(&object->LTM, _rt2dCTMGetDirect());

            /* Get the inverse transformation of
             * that used to position the object
             */
            RwMatrixInvert(&object->data.pickRegion.transformation,
                    &object->LTM);

            object->flag &= ~Rt2dObjectDirtyLTM;


        }
        else
        {
            _rt2dCTMSet(&object->LTM);
        }

#if 0
{
        Rt2dBrush *brush;
        brush = Rt2dBrushCreate();

        Rt2dPathStroke(object->data.pickRegion.path,brush);

        Rt2dBrushDestroy(brush);

}
#endif
        /* Store total transformation */
  /*      RwMatrixCopy(&object->data.pickRegion.transformation,
                     _rt2dCTMGetDirect());*/

        /* Calculate bounding box */
        /* DUMMY
        RwV2d bottomLeft;
        RwV2d topRight;
        bottomLeft = startBBox top right modified by CTM
        topRight = startBBox top right modified by CTM*/

        /* Restore CTM */
        Rt2dCTMPop();
    }

    RWRETURN(object);
}


/****************************************************************************/

#if defined (__MWERKS__)
#if (defined(RWVERBOSE))
#pragma message (__FILE__ "/" _SKY_EXPAND(__LINE__) ": __MWERKS__ == " _SKY_EXPAND(__MWERKS__))
#endif /* (defined (__MWERKS__)) */
#if (__option (global_optimizer))
#pragma always_inline on
#endif /* (__option (global_optimizer)) */
#endif /*  defined (__MWERKS__) */
