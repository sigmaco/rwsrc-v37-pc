#include "rwcore.h"
#include "rpworld.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "toon.h"
#include "genericedgerender.h"
#include "brutesilhouette.h"
#include "crease.h"

RwBool
_rpToonPipelinesCreate(void)
{
    RWFUNCTION(RWSTRING("_rpToonPipelinesCreate"));

    /*
     * Initialize edge renderer
     */
    _rpToonGenericEdgeRenderInit();
    _rpToonBruteSilhouetteInit();

    RWRETURN(TRUE);
}


RwBool
_rpToonPipelinesDestroy(void)
{
    RWFUNCTION(RWSTRING("_rpToonPipelinesDestroy"));

    _rpToonBruteSilhouetteClean();
    _rpToonGenericEdgeRenderClean();

    RWRETURN(TRUE);
}


static RpAtomic *
ToonAtomicRender( RpAtomic *atomic )
{
    RpToonGeo *toonGeo;
    RpMorphTarget *morph;
    RwV3d *verts, *normals;
    RwBool faceNormalsInvalid = FALSE;
    RwMatrix *lineTransform;

    RWFUNCTION(RWSTRING("ToonAtomicRender"));

    toonGeo = RpToonGeometryGetToonGeo(RpAtomicGetGeometry(atomic));

    /* cell shade set up paint render with custom pipeline */
    (*(toonGeo->defAtomicRenderCallback))(atomic);

    /* will need to be more clever for skinning/morphing */
    morph = RpGeometryGetMorphTarget(RpAtomicGetGeometry(atomic),0);
    RWASSERT(morph != 0);
    verts = RpMorphTargetGetVertices( morph );
    normals = toonGeo->extrusionVertexNormals;
    lineTransform = RwFrameGetLTM(RpAtomicGetFrame(atomic));

    _rpToonUpdateAtomicPerspectiveScale(toonGeo, atomic );


    /* edge detection & render */
    _rpToonGenericEdgeRenderPushState();
    _rpToonBruteSilhouetteRender( toonGeo, verts, normals, faceNormalsInvalid, lineTransform );
    _rpToonCreaseRender( toonGeo, verts, lineTransform );
    _rpToonGenericEdgeRenderPopState();

    RWRETURN(atomic);
}


void
_rpToonAtomicChainAtomicRenderCallback(RpAtomic *atomic)
{
    RpToonGeo *toonGeo;

    RWFUNCTION(RWSTRING("_rpToonAtomicChainAtomicRenderCallback"));

    RWASSERT(atomic);

    /*
    Don't chain umpteem million times even if the stupid skinning streaming
    code calls us umpteem million times.
    */
    toonGeo = RpToonGeometryGetToonGeo(RpAtomicGetGeometry(atomic));

    RWASSERT(toonGeo);

    if (RpAtomicGetRenderCallBack(atomic) != ToonAtomicRender)
    {
        toonGeo->defAtomicRenderCallback = RpAtomicGetRenderCallBack(atomic);
        toonGeo->defSectorRenderCallback = NULL;

        RpAtomicSetRenderCallBack(atomic, ToonAtomicRender);
    }

    RWRETURNVOID();
}


void
_rpToonAtomicChainSkinnedAtomicRenderCallback(RpAtomic *atomic)
{
    RpToonGeo *toonGeo;

    RWFUNCTION(RWSTRING("_rpToonAtomicChainSkinnedAtomicRenderCallback"));

    toonGeo = RpToonGeometryGetToonGeo(RpAtomicGetGeometry(atomic));

    RWASSERT(toonGeo);

    toonGeo->isSkinned = TRUE;

    _rpToonAtomicChainAtomicRenderCallback(atomic);

    RWRETURNVOID();
}


RwBool
_rpToonAtomicPipelinesAttach( RpAtomic *atomic )
{
    RWFUNCTION(RWSTRING("_rpToonAtomicPipelinesAttach"));

    _rpToonAtomicChainAtomicRenderCallback(atomic);

    RWRETURN(TRUE);
}


static RpWorldSector *
ToonSectorRender( RpWorldSector *sector )
{
    RpToonGeo *toonGeo;

    RWFUNCTION(RWSTRING("ToonSectorRender"));

    toonGeo = RpToonWorldSectorGetToonGeo(sector);

    /* cell shade set up paint render with custom pipeline */
    (*(toonGeo->defSectorRenderCallback))(sector);

    /* sector might be a space filling one */
    if (toonGeo->numVerts)
    {
        RwMatrix transform;

        RwMatrixSetIdentity(&transform);

        RWASSERT(sector->vertices && "sector has no vertices!");

        _rpToonUpdateWorldPerspectiveScale(toonGeo);

        _rpToonGenericEdgeRenderPushState();
        _rpToonBruteSilhouetteRender( toonGeo, sector->vertices, toonGeo->extrusionVertexNormals, FALSE,  &transform );
        _rpToonCreaseRender( toonGeo, sector->vertices, &transform );
        _rpToonGenericEdgeRenderPopState();
    }

    RWRETURN(sector);
}


RwBool
_rpToonSectorPipelinesAttach( RpWorldSector *sector )
{
    RpWorld *world;
    RpToonGeo *toonGeo;
    static RpWorld *lastWorld = 0;
    static RpWorldSectorCallBackRender defSectorRenderCallback = 0;

    RWFUNCTION(RWSTRING("_rpToonSectorPipelinesAttach"));

    RWASSERT(sector != NULL);

    toonGeo = RpToonWorldSectorGetToonGeo(sector);

    world = RpWorldSectorGetWorld(sector);
    RWASSERT(world != NULL);

    if (!lastWorld)
    {
        lastWorld = world;
        defSectorRenderCallback = RpWorldGetSectorRenderCallBack(world);
    }

    RWASSERT((lastWorld == world) && "multiple worlds TODO");

    toonGeo->defSectorRenderCallback = defSectorRenderCallback;

    RpWorldSetSectorRenderCallBack(world, ToonSectorRender);

    toonGeo->defAtomicRenderCallback = NULL;

    RWRETURN(TRUE);
}
