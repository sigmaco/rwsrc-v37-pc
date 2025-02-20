#ifndef TEAM_H
#define TEAM_H

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpteam.h"

#include "teamtypes.h"
#include "teamoptimize.h"
#include "teamplayer.h"
#include "teamskin.h"
#include "teamdictionary.h"
#include "teamstatic.h"
#include "teammatfx.h"
#include "teammaterial.h"
#include "teampipes.h"

/*===========================================================================*
 *--- Platform specific extention -------------------------------------------*
 *===========================================================================*/

#if (defined(SKY2_DRVMODEL_H))
#include "sky2/teamsky.h"
#include "sky2/teamskyrender.h"
#elif (defined(XBOX_DRVMODEL_H))
#include "xbox/teamxbox.h"
#include "xbox/teamrenderxbox.h"
#elif ((defined(NULL_DRVMODEL_H))    || \
       (defined(NULLGCN_DRVMODEL_H)) || \
       (defined(NULLSKY_DRVMODEL_H)) || \
       (defined(NULLXBOX_DRVMODEL_H)))
#include "null/teamnull.h"
#elif defined (GCN_DRVMODEL_H)
#include "gcn/teamgcn.h"
#include "gcn/teamgcnrender.h"
#elif defined (D3D8_DRVMODEL_H)
#include "d3d8/teamd3d8.h"
#include "d3d8/teamrenderd3d8.h"
#elif defined (D3D9_DRVMODEL_H)
#include "d3d9/teamd3d9.h"
#include "d3d9/teamrenderd3d9.h"
#elif defined (OPENGL_DRVMODEL_H)
#include "opengl/teamopengl.h"
#include "opengl/teamrenderopengl.h"
/*
#elif defined (SOFTRAS_DRVMODEL_H)
#include "softras/.h"
*/
#else
#error "unrecognised driver"
#endif


/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/
#define _rpTeamRenderAtomic(_atomic, _result)                           \
MACRO_START                                                             \
{                                                                       \
    RWASSERT(NULL != _atomic);                                          \
     /* We do not support user render callbacks */                      \
    RWASSERT(NULL == _atomic->renderCallBack);                          \
    RWASSERT(NULL != _atomic->pipeline);                                \
                                                                        \
    _rpTeamPipeOpenGlobalPipe(_atomic->pipeline);                       \
                                                                        \
    if(NULL != _rpTeamPipeRenderAtomic(_atomic))                        \
    {                                                                   \
        _result = _atomic;                                              \
    }                                                                   \
    else                                                                \
    {                                                                   \
        _result = (RpAtomic *)NULL;                                     \
    }                                                                   \
}                                                                       \
MACRO_STOP

/*===========================================================================*
 *--- Public Types ----------------------------------------------------------*
 *===========================================================================*/
enum RpTeamLightIndex
{
    rpTEAMAMBIENTLIGHT      = 0,
    rpTEAMDIRECTIONAL1LIGHT = 1,
    rpTEAMDIRECTIONAL2LIGHT = 2,
    rpTEAMDIRECTIONAL3LIGHT = 3,
    rpTEAMDIRECTIONAL4LIGHT = 4,
    rpTEAMPOINTLIGHT        = 5,
    rpTEAMMAXNUMLIGHTS      = 6,

    rpTEAMLIGHTINDEXFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};

/* Do not change these enumeration values without considering the ps2
 * vu code arrays as they currently match up. */
enum RpTeamLightType
{
    rpTEAMAMBIENTDIRECTIONLIGHTING  = 0,
    rpTEAMAMBIENT4DIRECTIONLIGHTING = 1,
    rpTEAMAMBIENTPOINTLIGHTING      = 2,

    rpTEAMLIGHTTYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RpTeamLightType RpTeamLightType;

typedef struct RpTeamFrustumTest RpTeamFrustumTest;
struct RpTeamFrustumTest                /* 8 bytes total. */
{
    RpTeamCameraTestSphere callBack;    /* 4 bytes. */
    RpTeamCameraTestSphereData data;    /* 4 bytes. */
};

struct RpTeam
{
    RwLLLink            list;
    RwUInt32            lockFlags;
    RpTeamPlayerData    data;
    RpTeamOptElements   opData;
    RpTeamLOD           lod;
    RpTeamDictionary   *dictionary;
    RwUInt32           *selected;    /* Selection count for dict elements. */
    RpTeamPlayer       *players;
    RpTeamShadows      *shadows;
#if(defined(TEAMMATRIXCACHECACHE))
    SkinCyclicCache     cyclicCache;
#endif
    RpTeamFrustumTest   frustumTest;
    /*-- Persistent team lights. --*/
    RpLight            *lights[rpTEAMMAXNUMLIGHTS];
    RpTeamLightType     lightType;
};

/*===========================================================================*
 *--- Public Global Variables -----------------------------------------------*
 *===========================================================================*/
extern RwModuleInfo TeamModule;

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/
extern void
_rpTeamAddTeamToList(RpTeam *team);

extern void
_rpTeamRemoveTeamFromList(RpTeam *team);

extern RwFrustumTestResult
_rpTeamCameraCustomTestSphere( const RpTeam *team,
                               RwUInt32 playerIndex,
                               const RwCamera *camera,
                               const RwSphere *sphere,
                               RpTeamCameraTestSphereData data );

extern RxPipeline **
_rpTeamGetRenderPipelines(void);

extern void
_rpTeamInstancePipelinesDestroy(void);

extern RxPipeline **
_rpTeamInstancePipelinesCreate(void);

/*===========================================================================*
 *--- Rendering Functions ---------------------------------------------------*
 *===========================================================================*/
extern RpAtomic *
_rpTeamShadowRenderAtomic( RpAtomic *atomic,
                           RpTeamPlayer *player );

extern RpAtomic *
_rpTeamPlayerRenderAtomic( RpAtomic *atomic,
                           const RpTeam *team,
                           RpTeamPlayer *player );

extern RpAtomic *
_rpTeamAtomicSetFrame( RpAtomic *atomic, RwFrame *frame );

/*===========================================================================*
 *--- Overloaded RW functions -----------------------------------------------*
 *===========================================================================*/
#if !(defined(TeamFrameGetLTM))
#define TeamFrameGetLTM(frame)                                      \
    RwFrameGetLTM(frame)
#endif /* !(defined(TeamFrameGetLTM)) */

#if !(defined(TeamV3dTransformVectors))
#define TeamV3dTransformVectors(pointsOut, pointsIn, matrix)        \
    RwV3dTransformVector(pointsOut, pointsIn, matrix)
#endif /* !(defined(TeamV3dTransformVectors)) */

#if !(defined(TeamV3dTransformPoints))
#define TeamV3dTransformPoints(pointsOut, pointsIn, matrix)         \
    RwV3dTransformPoint(pointsOut, pointsIn, matrix)
#endif /* !(defined(TeamV3dTransformPoints)) */

#if !(defined(TeamMatrixInvert))
#define TeamMatrixInvert(matrixOut, matrixIn)                       \
    RwMatrixInvert(matrixOut, matrixIn)
#endif /* !(defined(TeamMatrixInvert)) */

#if !(defined(TeamMatrixMultiply))
#define TeamMatrixMultiply(matrix, matrix1, matrix2)                \
    RwMatrixMultiply(matrix, matrix1, matrix2)
#endif /* !(defined(TeamMatrixMultiply)) */

#if !(defined(TeamMatrixCopy))
#define TeamMatrixCopy(matrix1, matrix2)                            \
    RwMatrixCopy(matrix1, matrix2)
#endif /* !(defined(TeamMatrixCopy)) */

#if !(defined(TeamV3dNormalize))
#define TeamV3dNormalize(vectorIn, vectorOut)                       \
    RwV3dNormalize(vectorIn, vectorOut)
#endif /* !(defined(TeamV3dNormalize)) */

/*===========================================================================*
 *--- Debugging functions ---------------------------------------------------*
 *===========================================================================*/

#if (defined(TEAMDRAWBOUNDINGSPHERES))

extern void
_rpTeamDrawAtomicBounds(RpAtomic *atomic);

extern void
_rpTeamDrawPlayerBounds(RpTeamPlayer *player);

#else /* (defined(TEAMDRAWBOUNDINGSPHERES)) */

#define _rpTeamDrawAtomicBounds(atomic)
#define _rpTeamDrawPlayerBounds(player)

#endif /* (defined(TEAMDRAWBOUNDINGSPHERES)) */

#define TEAMCHECKLODATOMICS

#if (defined(TEAMCHECKLODATOMICS))
#define TEAMCHECKLODATOMIC(atomic)  \
    if(NULL == atomic) continue
#else /* (defined(TEAMCHECKLODATOMICS)) */
#define TEAMCHECKLODATOMIC(atomic)  \
    RWASSERT(NULL != atomic)
#endif /* (defined(TEAMCHECKLODATOMICS)) */

/*---------------------------------------------------------------------------*/

#if !(defined(RWDEBUG))
#define TEAMINLINE __inline
#else /* !(defined(RWDEBUG)) */
#define TEAMINLINE
#endif /* !(defined(RWDEBUG)) */

/*---------------------------------------------------------------------------*/

#endif /* TEAM_H */
