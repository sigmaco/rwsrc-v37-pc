/*
 *
 * Functionality for 2D rendering
 *
 * Copyright (c) 2001 Criterion Software Ltd.
 */

/****************************************************************************
 *                                                                          *
 *  Module  :   object.c                                                    *
 *                                                                          *
 *  Purpose :   Hierarchical representation                                 *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 Includes
 */
#include <string.h>

#include "rwcore.h"
#include "rpdbgerr.h"
#include "rpworld.h"

#include "rt2d.h"
#include "font.h"
#include "path.h"
#include "tri.h"
#include "gstate.h"
#include "scene.h"
#include "shape.h"
#include "pickreg.h"
#include "2dstr.h"
#include "object.h"

#define FILE_CURRENT_VERSION 0x01
#define FILE_LAST_SUPPORTED_VERSION 0x01

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

/**
 * \ingroup rt2dobject
 * \page rt2dobjectoverview Rt2dObject Overview
 *
 * An object is either a shape, scene, pick regions or object string.
 * Objects work by manipulate transformation matrices.
 *
 * There are 3 transformation matrices used:
 * \li Current Transformation Matrix (CTM)
 * \li Local Transformation Matrix (LTM)
 * \li Modeling Transformation Matrix (MTM)
 *
 * \par Manipulating Objects in a Scene
 * -# \ref Rt2dSceneUnlock unlocks the scene.
 * -# \ref Rt2dSceneGetChildByIndex obtains a pointer to an object
 *    or \ref Rt2dSceneForAllChildren obtains pointers to all objects.
 * -# Manipulate objects and object colors using Rt2dObject functions.
 * -# \ref Rt2dObjectApplyCTM copies the current transformation
 *    matrix (CTM) to the object modeling transformation matrix
 *    (MTM). This is necessary to apply camera changes i.e. changing
 *    the viewpoint.
 * -# \ref Rt2dSceneUpdateLTM updates the LTM because the scene MTM
 *    has changed and may need to be recalculated for rendering.
 *    If the LTM does not need updating, for example for collision
 *    detection, you can wait until rendering because the rendering
 *    functions update the LTM.
 * -# \ref Rt2dSceneSetDepthDirty tells the scene the next time it
 *    renders the depth may have changed. This function is required if
 *    \ref Rt2dObjectSetDepth has manipulated an object.
 *
 * \see Rt2dSceneCreate
 * \see Rt2dObjectStringCreate
 * \see Rt2dPickRegionCreate
 * \see Rt2dShapeCreate
 * \see
 */

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   Functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

void
_rt2dObjectClose(void)
{
    RWFUNCTION(RWSTRING("_rt2dObjectClose"));

    RwFreeListDestroy(Rt2dGlobals.objectFreeList);

    Rt2dGlobals.objectFreeList = (RwFreeList *)NULL;

    RWRETURNVOID();
}

static RwInt32 _rt2dObjectFreeListBlockSize = 32,
               _rt2dObjectFreeListPreallocBlocks = 1;
static RwFreeList _rt2dObjectFreeList;

/**
 * \ingroup rt2d
 * \ref Rt2dObjectSetFreeListCreateParams allows the developer to specify
 * how many \ref Rt2dObject s to preallocate space for.
 * Call before \ref RwEngineInit.
 *
 * \param blockSize  number of entries per freelist block.
 * \param numBlocksToPrealloc  number of blocks to allocate on
 * \ref RwFreeListCreateAndPreallocateSpace.
 *
 * \see RwFreeList
 *
 */
void
Rt2dObjectSetFreeListCreateParams( RwInt32 blockSize, RwInt32 numBlocksToPrealloc )
{
    /* cannot use debugging macros since the debugger is not initialized before RwEngineInit */
    /*
    RWAPIFUNCTION( RWSTRING( "Rt2dObjectSetFreeListCreateParams" ) );
    */

    _rt2dObjectFreeListBlockSize = blockSize;
    _rt2dObjectFreeListPreallocBlocks = numBlocksToPrealloc;

    /*
    RWRETURNVOID();
    */
}

/****************************************************************************/

RwBool
_rt2dObjectOpen(void)
{
    RWFUNCTION(RWSTRING("_rt2dObjectOpen"));

    Rt2dGlobals.objectFreeList =
        RwFreeListCreateAndPreallocateSpace(sizeof(Rt2dObject), _rt2dObjectFreeListBlockSize, 16,
        _rt2dObjectFreeListPreallocBlocks, &_rt2dObjectFreeList,
        rwMEMHINTDUR_GLOBAL | rwID_2DOBJECT);

    RWRETURN(TRUE);
}

/*************************************************************************/

RwBool
_rt2dObjectDestruct(Rt2dObject *object)
{
    RwBool              result;
    RWFUNCTION(RWSTRING("_rt2dObjectDestruct"));
    RWASSERT(object);

    /* depending upon the type, call the appropriate destructor */
    switch (object->type)
    {
        case rt2DOBJECTTYPEOBJECT:
        {
            /* Object is untyped */
            RWMESSAGE((RWSTRING("Object lacks type")));
            result = FALSE;
            break;
        }
        case rt2DOBJECTTYPESCENE:
        {
            /* Container of objects */
            result = _rt2dSceneDestruct( object );
            break;
        }
        case rt2DOBJECTTYPESHAPE:
        {
            /* An indivisible shape */
            result = _rt2dShapeDestruct( object );
            break;
        }
        case rt2DOBJECTTYPEPICKREGION:
        {
            /* A pick region */
            result = _rt2dPickRegionDestruct( object );
            break;
        }
        case rt2DOBJECTTYPEOBJECTSTRING:
        {
            /* A text string */
            result = _rt2dObjectStringDestruct( object );
            break;
        }
        default:
        {
            /* Unrecognized */
            RWMESSAGE((RWSTRING("Unrecognized 2D object type")));
            result = FALSE;
        }
    }

    RWRETURN(result);
}

RwBool
_rt2dObjectDestroy(Rt2dObject *object)
{
    RwBool              result;
    RWFUNCTION(RWSTRING("_rt2dObjectDestroy"));

    /* NULL path is valid */
    result = (NULL != object);
    if (result)
    {
        /* Destruct the contents */
        result = _rt2dObjectDestruct( object );

        /* Free memory */
        RwFreeListFree(Rt2dGlobals.objectFreeList,object);
    }

    RWRETURN(result);
}

/****************************************************************************/

void
_rt2dObjectDestroyAfterRealloc(Rt2dObject *object)
{
    RWFUNCTION(RWSTRING("_rt2dObjectDestroyAfterRealloc"));
    RWASSERT(object);

    /* Free memory */
    RwFreeListFree(Rt2dGlobals.objectFreeList,object);

    RWRETURNVOID();
}

/****************************************************************************/

void
_rt2dObjectRealloc(Rt2dObject *dst, Rt2dObject *src)
{
    RWFUNCTION(RWSTRING("_rt2dObjectRealloc"));
    RWASSERT(dst);
    RWASSERT(src);

    memcpy(dst,src,sizeof(Rt2dObject));

    RWRETURNVOID();
}

/****************************************************************************/

/* In-place copy-to-dst with no destruct */
Rt2dObject *
_rt2dObjectCopy(Rt2dObject *dst, Rt2dObject *src)
{
    RWFUNCTION(RWSTRING("_rt2dObjectCopy"));
    RWASSERT(src);
    RWASSERT(dst);
    RWASSERT(dst!=src);
    /* The source object should be unlocked , so just assert here */
    RWASSERT(!(src->flag & Rt2dObjectIsLocked));

    /* depending upon the type, copy in the appropriate manner */
    switch (src->type)
    {
        case rt2DOBJECTTYPEOBJECT:
        {
            /* Object is untyped */
            RWMESSAGE((RWSTRING("Object lacks type")));
            RWRETURN(NULL);
        }
        case rt2DOBJECTTYPESCENE:
        {
            /* Container of objects */
            if (!_rt2dSceneCopy(dst, src))
            {
                RWRETURN(NULL);
            }
            break;
        }
        case rt2DOBJECTTYPESHAPE:
        {
            /* An indivisible shape */
            if (!_rt2dShapeCopy(dst, src))
            {
                RWRETURN(NULL);
            }
            break;
        }
        case rt2DOBJECTTYPEPICKREGION:
        case rt2DOBJECTTYPEOBJECTSTRING:
        {
            RwStream *stream;
            RwMemory buffer;

            buffer.length = 0;
            buffer.start = (RwUInt8 *)NULL;

            stream = RwStreamOpen(rwSTREAMMEMORY , rwSTREAMWRITE, &buffer);
            RWASSERT(stream);

            if( !_rt2dObjectStreamWrite(src, stream, NULL) )
            {
                RWRETURN((Rt2dObject*)NULL);
            }

            RwStreamClose(stream, &buffer);

            stream = RwStreamOpen(rwSTREAMMEMORY , rwSTREAMREAD, &buffer);
            RWASSERT(stream);

#if 0
            /* Discard chunk header */
            {
                RwChunkHeaderInfo header;

                if (!RwStreamReadChunkHeaderInfo(stream, &header))
                {
                    RWRETURN((Rt2dObject *)NULL);
                }
            }
#endif
            /* Read Base information */
            _rt2dObjectBaseStreamReadTo(dst, stream);

            if( !_rt2dObjectStreamReadTo(dst, stream, NULL) )
            {
                RWRETURN((Rt2dObject*)NULL);
            }

            RwStreamClose(stream, &buffer);

            RwFree(buffer.start);
            break;
        }
        default:
        {
            /* Unrecognized */
            RWMESSAGE((RWSTRING("Unrecognized 2D object type")));
            RWRETURN(NULL);
        }
    }

    RWRETURN(dst);
}


/**
 * \ingroup rt2dobject
 * \ref Rt2dObjectCopy copies an object to another one.
 * If the object is a scene, the scene should be unlocked before copying.
 *
 * \param dst Pointer to the destination memory to copy the object toobject.
 * \param src Pointer to the source object.
 *
 * \return Returns the destination object if successful; NULL otherwise.
 *
 * \see Rt2dObjectStringCreate
 * \see Rt2dPickRegionCreate
 * \see Rt2dSceneCreate
 * \see Rt2dShapeCreate
 * \see Rt2dSceneUnlock
 */
Rt2dObject *
Rt2dObjectCopy(Rt2dObject *dst, Rt2dObject *src)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectCopy"));
    RWASSERT(src);

    /* Early out if copying over self (can happen in some circumstances) */
    if (src==dst)
    {
        RWRETURN(dst);
    }

    if (dst)
    {
        _rt2dObjectDestruct(dst);
    }
    else
    {
        dst=_rt2dObjectCreate();
    }
    dst = _rt2dObjectCopy(dst, src);

    RWRETURN(dst);
}


/****************************************************************************/

Rt2dObject*
_rt2dObjectCreate()
{
    Rt2dObject           *object;

    RWFUNCTION(RWSTRING("_rt2dObjectCreate"));

    object = (Rt2dObject *) RwFreeListAlloc(Rt2dGlobals.objectFreeList,
                                rwMEMHINTDUR_EVENT | rwID_2DOBJECT);
    RWASSERT(object);

    RWRETURN(object);
}

Rt2dObject*
_rt2dObjectInit(Rt2dObject *object, RwInt32 objectType)
{
    RWFUNCTION(RWSTRING("_rt2dObjectInit"));
    RWASSERT(object);

    /* Initialize the object */
    object->type = objectType;
    object->depth = 666;

    RwMatrixSetIdentity(&object->MTM);
    RwMatrixSetIdentity(&object->LTM);

    object->colorMult.red = 1.0f;
    object->colorMult.green = 1.0f;
    object->colorMult.blue = 1.0f;
    object->colorMult.alpha = 1.0f;

    object->colorOffs.red = 0.0f;
    object->colorOffs.green = 0.0f;
    object->colorOffs.blue = 0.0f;
    object->colorOffs.alpha = 0.0f;
    object->flag = Rt2dObjectDirtyLTM | Rt2dObjectVisible;

    memset(&object->data,0,sizeof(object->data));
    RWRETURN(object);
}

/****************************************************************************/


RwUInt32
_rt2dObjectBaseStreamGetSize(Rt2dObject *object)
{
    RwUInt32 size;
    RWFUNCTION(RWSTRING("_rt2dObjectBaseStreamGetSize"));
    RWASSERT(object);

    size = sizeof(_rt2dStreamObject);

    size += RwMatrixStreamGetSize(&object->MTM);

    RWRETURN(size);
}

Rt2dObject*
_rt2dObjectBaseStreamWrite(Rt2dObject *object, RwStream *stream)
{
    _rt2dStreamObject sObject;
    RWFUNCTION(RWSTRING("_rt2dObjectBaseStreamWrite"));
    RWASSERT(object);
    RWASSERT(stream);

    sObject.version = FILE_CURRENT_VERSION;
    sObject.type = object->type;
    sObject.depth = object->depth;
    sObject.colorMult = object->colorMult;
    sObject.colorOffs = object->colorOffs;

    (void)RwMemRealToFloat32(&sObject.colorMult.red, sizeof(sObject.colorMult.red));
    (void)RwMemRealToFloat32(&sObject.colorMult.green,
                            sizeof(sObject.colorMult.green));
    (void)RwMemRealToFloat32(&sObject.colorMult.blue,
                            sizeof(sObject.colorMult.blue));
    (void)RwMemRealToFloat32(&sObject.colorMult.alpha,
                            sizeof(sObject.colorMult.alpha));
    (void)RwMemRealToFloat32(&sObject.colorOffs.red,
                            sizeof(sObject.colorOffs.red));
    (void)RwMemRealToFloat32(&sObject.colorOffs.green,
                            sizeof(sObject.colorOffs.green));
    (void)RwMemRealToFloat32(&sObject.colorOffs.blue,
                            sizeof(sObject.colorOffs.blue));
    (void)RwMemRealToFloat32(&sObject.colorOffs.alpha,
                            sizeof(sObject.colorOffs.alpha));

    (void)RwMemLittleEndian32(&sObject, sizeof(sObject));

    if (!RwStreamWrite(stream, &sObject, sizeof(_rt2dStreamObject)))
    {
        RWRETURN((Rt2dObject *)NULL);
    }

    if (!RwMatrixStreamWrite(&object->MTM,stream))
    {
        RWRETURN((Rt2dObject *)NULL);
    }

    RWRETURN(object);
}

/****************************************************************************/
Rt2dObject*
_rt2dObjectBaseStreamReadTo(Rt2dObject *object, RwStream *stream)
{
    _rt2dStreamObject sObject;
    RWFUNCTION(RWSTRING("_rt2dObjectBaseStreamReadTo"));
    RWASSERT(stream);
    RWASSERT(object);

    /* Read the Object header */
    if (RwStreamRead(stream, &sObject, sizeof(sObject)) != sizeof(sObject))
    {
        RWRETURN((Rt2dObject *)NULL);
    }

    /* Convert it */
    (void)RwMemNative32(&sObject, sizeof(sObject));

    (void)RwMemFloat32ToReal(&sObject.colorMult.red,   sizeof(sObject.colorMult.red));
    (void)RwMemFloat32ToReal(&sObject.colorMult.green, sizeof(sObject.colorMult.green));
    (void)RwMemFloat32ToReal(&sObject.colorMult.blue,  sizeof(sObject.colorMult.blue));
    (void)RwMemFloat32ToReal(&sObject.colorMult.alpha, sizeof(sObject.colorMult.alpha));

    (void)RwMemFloat32ToReal(&sObject.colorOffs.red,   sizeof(sObject.colorOffs.red));
    (void)RwMemFloat32ToReal(&sObject.colorOffs.green, sizeof(sObject.colorOffs.green));
    (void)RwMemFloat32ToReal(&sObject.colorOffs.blue,  sizeof(sObject.colorOffs.blue));
    (void)RwMemFloat32ToReal(&sObject.colorOffs.alpha, sizeof(sObject.colorOffs.alpha));

    RWASSERT(sObject.version <= FILE_LAST_SUPPORTED_VERSION
                && sObject.version >= FILE_CURRENT_VERSION);

    if (!RwStreamFindChunk(stream, rwID_MATRIX,
                           (RwUInt32 *)NULL,
                           (RwUInt32 *)NULL))
    {
        RWRETURN((Rt2dObject *)NULL);
    }

    RwMatrixStreamRead(stream, &object->MTM);

    object->type = sObject.type;
    object->depth = sObject.depth;
    object->colorMult = sObject.colorMult;
    object->colorOffs = sObject.colorOffs;
    object->flag = Rt2dObjectDirtyLTM | Rt2dObjectVisible;

    RWRETURN(object);
}

RwUInt32
_rt2dObjectStreamGetSize(Rt2dObject *object, _rt2dSceneResourcePool *pool)
{
    RWFUNCTION(RWSTRING("_rt2dObjectStreamGetSize"));
    RWASSERT(object);

    /* Type dependant streaming */
    switch (object->type)
    {
        case rt2DOBJECTTYPEOBJECT:
        {
            /* Object is untyped */
            RWMESSAGE((RWSTRING("Object lacks type")));
            RWRETURN( 0 );
            break;
        }
        case rt2DOBJECTTYPESCENE:
        {
            /* Container of objects */
            RWRETURN(_rt2dSceneStreamGetSize(object, pool));
            break;
        }
        case rt2DOBJECTTYPESHAPE:
        {
            /* An indivisible shape */
            RWRETURN(_rt2dShapeStreamGetSize(object, pool));
            break;
        }
        case rt2DOBJECTTYPEPICKREGION:
        {
            /* A pick region */
            RWRETURN( _rt2dPickRegionStreamGetSize( object ) );
            break;
        }
        case rt2DOBJECTTYPEOBJECTSTRING:
        {
            /* A text string */
            RWRETURN( _rt2dObjectStringStreamGetSize( object ) );
            break;
        }
        default:
        {
            /* Unrecognized */
            RWMESSAGE((RWSTRING("Unrecognized 2D object type")));
            RWRETURN( 0 );
        }
    }
}

Rt2dObject*
_rt2dObjectStreamWrite(Rt2dObject *object, RwStream *stream, _rt2dSceneResourcePool *pool)
{
    RWFUNCTION(RWSTRING("_rt2dObjectStreamWrite"));
    RWASSERT(object);
    RWASSERT(stream);

    /* Type dependant streaming */
    switch (object->type)
    {
        case rt2DOBJECTTYPEOBJECT:
        {
            /* Object is untyped */
            RWMESSAGE((RWSTRING("Object lacks type")));
            RWRETURN( (Rt2dObject*) NULL );
            break;
        }
        case rt2DOBJECTTYPESCENE:
        {
            /* Container of objects */
            RWRETURN(_rt2dSceneStreamWrite(object, stream, pool));
            break;
        }
        case rt2DOBJECTTYPESHAPE:
        {
            /* An indivisible shape */
            RWRETURN(_rt2dShapeStreamWrite(object, stream, pool));
            break;
        }
        case rt2DOBJECTTYPEPICKREGION:
        {
            /* A pick region */
            RWRETURN(_rt2dPickRegionStreamWrite( object, stream ) );
            break;
        }
        case rt2DOBJECTTYPEOBJECTSTRING:
        {
            /* A text string */
            RWRETURN(_rt2dObjectStringStreamWrite( object, stream ) );
            break;
        }
        default:
        {
            /* Unrecognized */
            RWMESSAGE((RWSTRING("Unrecognized 2D object type")));
            RWRETURN( (Rt2dObject*) NULL );
        }
    }
}



/****************************************************************************/
Rt2dObject*
_rt2dObjectStreamReadTo(Rt2dObject *object, RwStream *stream, _rt2dSceneResourcePool *pool)
{
    RWFUNCTION(RWSTRING("_rt2dObjectStreamReadTo"));
    RWASSERT(stream);
    RWASSERT(object);

    /* Type dependant streaming */
    switch (object->type)
    {
        case rt2DOBJECTTYPESCENE:
        {
            /* Container of objects */
            _rt2dSceneStreamReadTo(object, stream, pool);
            break;
        }
        case rt2DOBJECTTYPESHAPE:
        {
            /* An indivisible shape */
            _rt2dShapeStreamReadTo(object, stream, pool);
            break;
        }
        case rt2DOBJECTTYPEPICKREGION:
        {
            /* A pick region */
            _rt2dPickRegionStreamReadTo( object, stream );
            break;
        }
        case rt2DOBJECTTYPEOBJECTSTRING:
        {
            /* A text string */
            _rt2dObjectStringStreamReadTo( object, stream );
            break;
        }
        default:
        {
            /* Unrecognized */
            RWMESSAGE((RWSTRING("Unrecognized 2D object type")));
            RWRETURN( (Rt2dObject*) NULL );
        }
    }

    RWRETURN(object);
}

/****************************************************************************/

extern Rt2dObject *
_rt2dObjectRender(Rt2dObject  *object, const RwRGBAReal *colorMult, const RwRGBAReal *colorOffs)
{
    RWFUNCTION(RWSTRING("_rt2dObjectRender"));
    RWASSERT(object);

    /* depending upon the type, call the appropriate sub-rendering functions */
    switch (object->type)
    {
        case rt2DOBJECTTYPEOBJECT:
        {
            /* Object is untyped */
            RWMESSAGE((RWSTRING("Object lacks type")));
            break;
        }
        case rt2DOBJECTTYPESCENE:
        {
            /* Container of objects */
            _rt2dSceneRenderChild( object, colorMult, colorOffs );
            break;
        }
        case rt2DOBJECTTYPESHAPE:
        {
            /* An indivisible shape */
            _rt2dShapeRenderChild( object, colorMult, colorOffs );
            break;
        }
        case rt2DOBJECTTYPEPICKREGION:
        {
            /* A pick region */
            _rt2dPickRegionRender( object );
            break;
        }
        case rt2DOBJECTTYPEOBJECTSTRING:
        {
            /* A text string */
            Rt2dObjectStringRender( object );
            break;
        }
        default:
        {
            /* Unrecognized */
            RWMESSAGE((RWSTRING("Unrecognized 2D object type")));
            object = (Rt2dObject*) NULL;
        }
    }

    RWRETURN(object);
}


/*
 * OBJECT MANIPULATION
 * OPTIM MACRO
 */

/**
 * \ingroup rt2dobject
 * \ref Rt2dObjectSetVisible sets whether or not the object is rendered.
 *
 * \param object    Pointer to the object to use.
 * \param isVisible TRUE if object should be rendered.
 *
 * \return Returns the 2dObject if successful or NULL if there is error.
 *
 * \see Rt2dObjectIsVisible
 */
Rt2dObject *
Rt2dObjectSetVisible(Rt2dObject *object, RwBool isVisible)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectSetVisible"));
    RWASSERT(object);

    if(TRUE == isVisible)
    {
        object->flag |= Rt2dObjectVisible;
    }
    else
    {
        object->flag &= ~Rt2dObjectVisible;
    }

    RWRETURN(object);
}

/**
 * \ingroup rt2dobject
 * \ref Rt2dObjectIsVisible checks if the object will be rendered.
 *
 * \param object Pointer to the object to check.
 *
 * \return TRUE if visible; FALSE otherwise.
 *
 * \see Rt2dObjectSetVisible
 */
RwBool
Rt2dObjectIsVisible(Rt2dObject *object)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectIsVisible"));
    RWASSERT(object);

    RWRETURN(object->flag & Rt2dObjectVisible);
}

/**
 * \ingroup rt2dobject
 * \ref Rt2dObjectGetMTM gets the MTM for positioning the object.
 *
 * \param object Pointer to the object to use.
 *
 * \return  Returns the MTM relative to the object's parent.
 *
 * \see Rt2dObjectSetMTM
 * \see Rt2dObjectMTMChanged
 * \see Rt2dObjectGetLTM
 */
RwMatrix *
Rt2dObjectGetMTM(Rt2dObject *object)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectGetMTM"));
    RWASSERT(object);

    RWRETURN(&object->MTM);
}

/**
 * \ingroup rt2dobject
 * \ref Rt2dObjectApplyCTM applies the current CTM to the object MTM.
 *
 * \param object Pointer to the object to use.
 *
 * \return NULL.
 *
 * \see Rt2dObjectGetLTM
 * \see Rt2dObjectGetMTM
 * \see Rt2dObjectSetMTM
 * \see Rt2dObjectMTMChanged
 * \see Rt2dCTMPush
 * \see Rt2dCTMPop
 * \see Rt2dCTMSetIdentity
 * \see Rt2dCTMScale
 * \see Rt2dCTMTranslate
 * \see Rt2dCTMRotate
 *
 */
void
Rt2dObjectApplyCTM(Rt2dObject *object)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectApplyCTM"));
    RWASSERT(object);

    Rt2dObjectSetMTM(object, _rt2dCTMGetDirect());

    RWRETURNVOID();
}

/**
 * \ingroup rt2dobject
 * \ref Rt2dObjectSetMTM sets the MTM for positioning the object (relative
 * to the parent).
 *
 * \param object Pointer to the object to use.
 * \param mtm    Positioning matrix relative to the parent.
 *
 * \return Returns the 2dObject if successful or NULL if there is error.
 *
 * \see Rt2dObjectGetMTM
 * \see Rt2dObjectMTMChanged
 * \see Rt2dObjectGetLTM
 */
Rt2dObject *
Rt2dObjectSetMTM(Rt2dObject *object, RwMatrix *mtm)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectSetMTM"));
    RWASSERT(object);
    RWASSERT(mtm);

    RwMatrixCopy(&object->MTM, mtm);
    object->flag |= Rt2dObjectDirtyLTM;

    RWRETURN(object);
}

/**
 * \ingroup rt2dobject
 * \ref Rt2dObjectGetLTM gets the LTM for positioning the object.
 *
 * \param object Pointer to the object to use.
 *
 * \return  Returns the LTM.
 *
 * \see Rt2dObjectSetMTM
 * \see Rt2dObjectGetMTM
 * \see Rt2dObjectMTMChanged
 */
RwMatrix *
Rt2dObjectGetLTM(Rt2dObject *object)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectGetLTM"));
    RWASSERT(object);

    /*AJH:
    * if LTM is dirty we might need to force an update, nope ?
    */

    RWRETURN(&object->LTM);
}

/**
 * \ingroup rt2dobject
 * \ref Rt2dObjectMTMChanged warns the object that the MTM matrix has been
 * changed (using \ref Rt2dObjectGetMTM) and that the LTM might need to be
 * recalculated.
 *
 * \param object Pointer to the object to use.
 *
 * \return NULL
 *
 * \see Rt2dObjectSetMTM
 * \see Rt2dObjectGetMTM
 * \see Rt2dObjectGetLTM
 */
void
Rt2dObjectMTMChanged(Rt2dObject *object)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectMTMChanged"));
    RWASSERT(object);

    object->flag |= Rt2dObjectDirtyLTM;

    RWRETURNVOID();
}

/**
 * \ingroup rt2dobject
 * \ref Rt2dObjectMTMSetIdentity is used to set the object MTM equal to the
 * identity matrix. This procedure removes all previously applied
 * transformations.
 *
 * \param object Pointer to the object to use.
 *
 * \return Returns the object pointer on success.
 *
 * \see Rt2dObjectMTMChanged
 * \see Rt2dObjectGetLTM
 * \see Rt2dObjectGetMTM
 * \see Rt2dObjectSetMTM
 * \see Rt2dObjectApplyCTM
 * \see Rt2dObjectMTMScale
 * \see Rt2dObjectMTMTranslate
 * \see Rt2dObjectMTMRotate
 *
 */
Rt2dObject *
Rt2dObjectMTMSetIdentity(Rt2dObject *object)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectMTMSetIdentity"));
    RWASSERT(object);

    RwMatrixSetIdentity(&object->MTM);
    Rt2dObjectMTMChanged(object);

    RWRETURN(object);
}

/**
 * \ingroup rt2dobject
 * \ref Rt2dObjectMTMScale
 * is used to apply a scale transformation to the object's MTM
 * using the specified x- and y-scale factors.
 * Note that the scale is preconcatenated with the MTM.
 *
 * \param object Pointer to the object to use.
 * \param x      A \ref RwReal value equal to the scale factor in the x-direction.
 * \param y      A \ref RwReal value equal to the scale factor in the y-direction.
 *
 * \return Returns the object pointer on success.
 *
 * \see Rt2dObjectMTMChanged
 * \see Rt2dObjectGetLTM
 * \see Rt2dObjectGetMTM
 * \see Rt2dObjectSetMTM
 * \see Rt2dObjectApplyCTM
 * \see Rt2dObjectMTMSetIdentity
 * \see Rt2dObjectMTMTranslate
 * \see Rt2dObjectMTMRotate
 *
 */
Rt2dObject *
Rt2dObjectMTMScale(Rt2dObject *object, RwReal x, RwReal y)
{
    RwV3d               scale;

    RWAPIFUNCTION(RWSTRING("Rt2dObjectMTMScale"));
    RWASSERT(object);

    scale.x = x;
    scale.y = y;
    scale.z = 1.0f;
    RwMatrixScale(&object->MTM, &scale, rwCOMBINEPRECONCAT);

    Rt2dObjectMTMChanged(object);

    RWRETURN(object);
}

/**
 * \ingroup rt2dobject
 * \ref Rt2dObjectMTMTranslate
 * is used to apply a translation to the MTM using the specified x-
 * and y-components.
 * Note that the translation is preconcatenated with the MTM.
 *
 * \param object Pointer to the object to use.
 * \param x      A \ref RwReal value equal to the translation in the x-direction.
 * \param y      A \ref RwReal value equal to the translation in the y-direction.
 *
 * \return Returns the object pointer on success.
 *
 * \see Rt2dObjectMTMChanged
 * \see Rt2dObjectGetLTM
 * \see Rt2dObjectGetMTM
 * \see Rt2dObjectSetMTM
 * \see Rt2dObjectApplyCTM
 * \see Rt2dObjectMTMSetIdentity
 * \see Rt2dObjectMTMScale
 * \see Rt2dObjectMTMRotate
 *
 */
Rt2dObject *
Rt2dObjectMTMTranslate(Rt2dObject *object, RwReal x, RwReal y)
{
    RwV3d               translate;

    RWAPIFUNCTION(RWSTRING("Rt2dObjectMTMTranslate"));
    RWASSERT(object);

    translate.x = x;
    translate.y = y;
    translate.z = 0.0f;
    RwMatrixTranslate(&object->MTM , &translate, rwCOMBINEPRECONCAT);

    Rt2dObjectMTMChanged(object);

    RWRETURN(object);
}

/**
 * \ingroup rt2dobject
 * \ref Rt2dObjectMTMRotate
 * is used to apply a rotation to the current MTM using
 * the specified angle of rotation. A rotation in an anticlockwise
 * direction are achieved using a positive angle.
 * Note that the rotation is preconcatenated with the MTM.
 *
 * \param object Pointer to the object to use.
 * \param theta  A \ref RwReal value equal to the angle of rotation in degrees.
 *
 * \return Returns the object pointer on success.
 *
 * \see Rt2dObjectGetMTM
 * \see Rt2dObjectSetMTM
 * \see Rt2dObjectApplyCTM
 * \see Rt2dObjectMTMSetIdentity
 * \see Rt2dObjectMTMScale
 * \see Rt2dObjectMTMTranslate
 *
 */
Rt2dObject *
Rt2dObjectMTMRotate(Rt2dObject *object, RwReal theta)
{
    RwV3d               axis;
    RWAPIFUNCTION(RWSTRING("Rt2dObjectMTMRotate"));
    RWASSERT(object);

    axis.x = 0.0f;
    axis.y = 0.0f;
    axis.z = 1.0f;
    RwMatrixRotate(&object->MTM, &axis, theta,
                   rwCOMBINEPRECONCAT);
    Rt2dObjectMTMChanged(object);

    RWRETURN(object);
}

/**
 * \ingroup rt2dobject
 * \ref Rt2dObjectGetColorMultiplier gets the color multiplier of this
 * object (relative to the parent object).
 * The floating point value should be between 0 and 1.
 *
 * \param object Pointer to the object to use.
 *
 * \return Returns the color multiplier of the object.
 *
 * \see Rt2dObjectSetColorMultiplier
 */
RwRGBAReal *
Rt2dObjectGetColorMultiplier(Rt2dObject *object)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectGetColorMultiplier"));
    RWASSERT(object);

    RWRETURN(&object->colorMult);
}

/**
 * \ingroup rt2dobject
 * \ref Rt2dObjectSetColorMultiplier sets the color multiplier of the object
 * (relative to the parent object).
 * The floating point value should be between 0 and 1.
 *
 * \param object  Pointer to the object to use.
 * \param multCol Pointer to an \ref RwRGBAReal color to use.
 *
 * \return Returns the 2dObject if successful or NULL if there is error.
 *
 * \see Rt2dObjectGetColorMultiplier
 */
Rt2dObject *
Rt2dObjectSetColorMultiplier(Rt2dObject *object, RwRGBAReal *multCol)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectSetColorMultiplier"));
    RWASSERT(object);
    RWASSERT(multCol);

    memcpy(&object->colorMult,multCol,sizeof(RwRGBAReal));
    object->flag |= Rt2dObjectDirtyColor;

    RWRETURN(object);
}

/**
 * \ingroup rt2dobject
 * \ref Rt2dObjectGetColorOffset gets the color offset in use
 * (relative to the parent). The floating point value should be
 * between 0 and 1.
 *
 * \param object Pointer to the object to use.
 *
 * \return Returns the color offset of the object.
 *
 * \see Rt2dObjectSetColorOffset
 */
RwRGBAReal *
Rt2dObjectGetColorOffset(Rt2dObject *object)
{
    static RwRGBAReal color;

    RWAPIFUNCTION(RWSTRING("Rt2dObjectGetColorOffset"));
    RWASSERT(object);

    memcpy(&color,&object->colorOffs,sizeof(RwRGBAReal));

    color.red   *= 1.0f/255.0f;
    color.green *= 1.0f/255.0f;
    color.blue  *= 1.0f/255.0f;
    color.alpha *= 1.0f/255.0f;

    RWRETURN(&color);
}

/**
 * \ingroup rt2dobject
 * \ref Rt2dObjectSetColorOffset sets the color offset in use (relative
 * to the parent). The floating point value should be between 0 and 1.
 *
 * \param object  Pointer to the object to use.
 * \param oofsCol Pointer to the \ref RwRGBAReal color to use.
 *
 * \return Returns the 2dObject if successful or NULL if there is error.
 *
 * \see Rt2dObjectGetColorOffset
 */
Rt2dObject *
Rt2dObjectSetColorOffset(Rt2dObject *object, RwRGBAReal *oofsCol)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectSetColorOffset"));
    RWASSERT(object);
    RWASSERT(oofsCol);

    memcpy(&object->colorOffs,oofsCol,sizeof(RwRGBAReal));

    object->colorOffs.red *= 255.0f;
    object->colorOffs.green *= 255.0f;
    object->colorOffs.blue *= 255.0f;
    object->colorOffs.alpha *= 255.0f;

    object->flag |= Rt2dObjectDirtyColor;

    RWRETURN(object);
}

/**
 * \ingroup rt2dobject
 * \ref Rt2dObjectGetObjectType gets the internal object type.
 *
 * \param object Pointer to the object to use.
 *
 * \return Returns the object type.
 *
 * \see Rt2dObjectTypeEnum
 */
RwInt32
Rt2dObjectGetObjectType(Rt2dObject *object)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectGetObjectType"));
    RWASSERT(object);

    RWRETURN(object->type);
}

/**
 * \ingroup rt2dobject
 * \ref Rt2dObjectGetDepth gets the z-order of the object.
 *
 * \param object Pointer to the object to use.
 *
 * \return Returns the z-order of the object.
 *
 * \see Rt2dObjectSetDepth
 */
RwInt32
Rt2dObjectGetDepth(Rt2dObject *object)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectGetDepth"));
    RWASSERT(object);

    RWRETURN(object->depth);
}

/**
 * \ingroup rt2dobject
 * \ref Rt2dObjectSetDepth sets the z-order of the object.
 *
 * \param object Pointer to the object to use.
 * \param depth  New depth of the object.
 *
 * \return Returns the 2dObject if successful or NULL if there is error.
 *
 * \see Rt2dObjectGetDepth
 */
Rt2dObject *
Rt2dObjectSetDepth(Rt2dObject *object, RwInt32 depth)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectSetDepth"));
    RWASSERT(object);

    object->depth = depth;

    RWRETURN(object);
}

/**
 * \ingroup rt2dobject
 * \ref Rt2dObjectIsScene tests if a 2d object is a Scene.
 *
 * \param object Pointer to the object to use.
 *
 * \return Returns TRUE if the object is a Scene.
 *
 * \see Rt2dObjectIsShape
 * \see Rt2dObjectIsObjectString
*  \see Rt2dObjectIsPickRegion
 */
RwBool
Rt2dObjectIsScene(Rt2dObject *object)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectIsScene"));

    RWRETURN(rt2DOBJECTTYPESCENE == object->type);
}

/**
 * \ingroup rt2dobject
 * \ref Rt2dObjectIsShape tests if 2d object is a Shape.
 *
 * \param object Pointer to the object to use.
 *
 * \return Returns TRUE if the object is a Shape.
 *
 * \see Rt2dObjectIsScene
 * \see Rt2dObjectIsObjectString
*  \see Rt2dObjectIsPickRegion
 */
RwBool
Rt2dObjectIsShape(Rt2dObject *object)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectIsShape"));

    RWRETURN(rt2DOBJECTTYPESHAPE == object->type);
}

/**
 * \ingroup rt2dobject
 * \ref Rt2dObjectIsPickRegion tests if a 2d object is a Pick Region.
 *
 * \param object Pointer to the object to use.
 *
 * \return Returns TRUE if the object is a Pick Region.
 *
 * \see Rt2dObjectIsScene
 * \see Rt2dObjectIsShape
*  \see Rt2dObjectIsObjectString
 */
RwBool
Rt2dObjectIsPickRegion(Rt2dObject *object)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectIsPickRegion"));

    RWRETURN(rt2DOBJECTTYPEPICKREGION == object->type);
}

/**
 * \ingroup rt2dobject
 * \ref Rt2dObjectIsObjectString tests if a 2d object is an Object String.
 *
 * \param object Pointer to the object to use.
 *
 * \return Returns TRUE if the object is an ObjectString.
 *
 * \see Rt2dObjectIsScene
 * \see Rt2dObjectIsShape
 * \see Rt2dObjectIsPickRegion
 */
RwBool
Rt2dObjectIsObjectString(Rt2dObject *object)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectIsObjectString"));

    RWRETURN(rt2DOBJECTTYPEOBJECTSTRING == object->type);
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
