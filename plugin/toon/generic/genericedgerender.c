#include "rwcore.h"
#include "rpworld.h"

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "toon.h"
#include "genericedgerender.h"

/* Generic 3D immediate mode wrapper for edge rendering */
RwIm3DVertex *_rpToonGenericVertexBuffer = NULL;


void
_rpToonScreenSpaceProjectionInit(ScreenSpaceProjection *projection,
                                 const RwMatrix *transform)
{
    RwMatrix invCamLTM;
    RwCamera *camera;
    RwFrame *cameraFrame;

    RWFUNCTION(RWSTRING("_rpToonScreenSpaceProjectionInit"));

    camera = RwCameraGetCurrentCamera();
    RWASSERT(0 != camera);

    cameraFrame = RwCameraGetFrame(camera);
    RWASSERT(0 != cameraFrame);

    RwMatrixInvert( &invCamLTM, RwFrameGetLTM(cameraFrame));

    RwMatrixMultiply( &projection->matrix, transform, &invCamLTM );

    RWRETURNVOID();
}

void
_rpToonGenericEdgeRenderInit(void)
{
    RWFUNCTION(RWSTRING("_rpToonGenericEdgeRenderInit"));

    _rpToonGenericVertexBuffer = (RwIm3DVertex *)
        RwMalloc(sizeof(RwIm3DVertex) * RPTOON_MAX_VERTICES,
            rwID_TOONPLUGIN | rwMEMHINTDUR_GLOBAL);
    RWASSERT((_rpToonGenericVertexBuffer!=0) && "RwMalloc failed");
    RWRETURNVOID();
}

void
_rpToonGenericEdgeRenderClean(void)
{
    RWFUNCTION(RWSTRING("_rpToonGenericEdgeRenderClean"));
    RwFree(_rpToonGenericVertexBuffer);
    RWRETURNVOID();
}

#if (0)
static RwUInt32 oldShadeMode;
static RwUInt32 oldSrcBlend;
static RwUInt32 oldDstBlend;
#endif/*  (0) */

void
_rpToonGenericEdgeRenderPushState(void)
{

    RWFUNCTION(RWSTRING("_rpToonGenericEdgeRenderPushState"));

    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void *) rwSHADEMODEGOURAUD);

#ifdef TRANSPARENTEDGES
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,(void *) TRUE);
#endif

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)rwBLENDINVSRCALPHA);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void *)NULL);

    RWRETURNVOID();

}

void
_rpToonGenericEdgeRenderPopState(void)
{

    RWFUNCTION(RWSTRING("_rpToonGenericEdgeRenderPopState"));
    RWRETURNVOID();
}

void
_rpToonGenericEdgeRenderSubmit( RwInt32 numVerts, RwPrimitiveType primType )
{
    RwCamera *camera;
    RwFrame *cameraFrame;

    RWFUNCTION(RWSTRING("_rpToonGenericEdgeRenderSubmit"));

    if (numVerts == 0)
    {
        /* early out to avoid crashing driver */
        RWRETURNVOID();
    }

    RWASSERT(numVerts < RPTOON_MAX_VERTICES);

    camera = RwCameraGetCurrentCamera();
    RWASSERT(0 != camera);

    cameraFrame = RwCameraGetFrame(camera);
    RWASSERT(0 != cameraFrame);

    /*
    We're submitting vertices in camera space, but im3d will try and transform them again, so cancel it out
    */
    if (RwIm3DTransform(
            _rpToonGenericVertexBuffer,
            numVerts,
            RwFrameGetLTM(cameraFrame),
            rwIM3D_ALLOPAQUE))
    {
        RwIm3DRenderPrimitive(primType);

        RwIm3DEnd();
    }

    RWRETURNVOID();
}

void
ScreenSpaceProjectionProjectPoint(RwV2d *v0p,   /* projected x & y */
                                  RwV3d *v0,    /* camera space x,y & z */
                                  const ScreenSpaceProjection *projection,
                                  const RwV3d *point)   /* source point in object space */
{
    RwReal oov0z;


    RWFUNCTION( RWSTRING( "ScreenSpaceProjectionProjectPoint" ) );

    RwV3dTransformPoint(v0, point, &projection->matrix);

    oov0z = 1.0f / v0->z;
    v0p->x = v0->x * oov0z;
    v0p->y = v0->y * oov0z;

    RWRETURNVOID();
}

/* Computes perspective scale factor per vertex given camera space z value & precomputed coefficients */
RwReal
_rpToonComputePerspectiveScale(const RpToonInk *ink,
                               RwReal camSpaceZ)
{
    RWFUNCTION(RWSTRING("_rpToonComputePerspectiveScale"));
    RWASSERT(ink);
    RWRETURN(ink->perspectiveScale_a + ink->perspectiveScale_b * (camSpaceZ - ink->perspectiveScale_zMin) );
}

/* project a 2D vertex back to 3D */
void
BackProject(RwV3d *vertex)
{
    RWFUNCTION( RWSTRING( "BackProject" ) );

    vertex->x *= vertex->z;
    vertex->y *= vertex->z;

    RWRETURNVOID();
}

RwIm3DVertex *
_rpToonGenericEdgeRenderAppendIm3dQuad(RwIm3DVertex *ptr,
                                       const RwV3d *v0p,
                                       const RwV3d *v1p,
                                       const RwV3d *v2p,
                                       const RwV3d *v3p,
                                       RwRGBA color)
{
    RWFUNCTION( RWSTRING( "_rpToonGenericEdgeRenderAppendIm3dQuad" ) );

    RwIm3DVertexSetPos(ptr, v0p->x, v0p->y, v0p->z );
    RwIm3DVertexSetRGBA(ptr, color.red, color.green, color.blue, color.alpha);
    ptr++;
    RwIm3DVertexSetPos(ptr, v1p->x, v1p->y, v1p->z );
    RwIm3DVertexSetRGBA(ptr, color.red, color.green, color.blue, color.alpha);
    ptr++;
    RwIm3DVertexSetPos(ptr, v2p->x, v2p->y, v2p->z );
    RwIm3DVertexSetRGBA(ptr, color.red, color.green, color.blue, color.alpha);
    ptr++;
    RwIm3DVertexSetPos(ptr, v1p->x, v1p->y, v1p->z );
    RwIm3DVertexSetRGBA(ptr, color.red, color.green, color.blue, color.alpha);
    ptr++;
    RwIm3DVertexSetPos(ptr, v3p->x, v3p->y, v3p->z );
    RwIm3DVertexSetRGBA(ptr, color.red, color.green, color.blue, color.alpha);
    ptr++;
    RwIm3DVertexSetPos(ptr, v2p->x, v2p->y, v2p->z );
    RwIm3DVertexSetRGBA(ptr, color.red, color.green, color.blue, color.alpha);
    ptr++;

    RWRETURN( ptr );
}
