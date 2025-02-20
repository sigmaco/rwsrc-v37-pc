/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   patchsmooth.c                                              -*
 *-                                                                         -*
 *-  Purpose :   General patch handling.                                    -*
 *-                                                                         -*
 *===========================================================================*/

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <string.h>

#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rppatch.h"

#include "patchsmooth.h"
#include "patch.h"

/*===========================================================================*
 *--- Private Types ---------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Global Variables ----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Private Defines -------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/
typedef struct PatchIdxTable PatchIdxTable;
struct PatchIdxTable
{
    RwUInt32 count;
    RwUInt32 size;
    RxVertexIndex *idx;
};

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/
#define PATCHSMOOTHIDXTABLESIZE (8)

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/

/*****************************************************************************
 PatchSmoothIdxTableCreate

 Inputs : size - Size of index table.
 Outputs: PatchIdxTable - Constructed table.
 */
static PatchIdxTable *
PatchSmoothIdxTableCreate(RwUInt32 size)
{
    PatchIdxTable *idxTable;
    RwUInt32 sizeTotal;

    RWFUNCTION(RWSTRING("PatchSmoothIdxTableCreate"));

    /* How much memory. */
    sizeTotal = size * sizeof(PatchIdxTable);

    /* Grab some. */
    idxTable = (PatchIdxTable *)RwMalloc(sizeTotal,
                 rwID_PATCHPLUGIN | rwMEMHINTDUR_EVENT);
    RWASSERT(NULL != idxTable);

    /* Init the idx table. */
    memset(idxTable, 0, sizeTotal);

    RWRETURN(idxTable);
}

/*****************************************************************************
 PatchSmoothIdxTableDestroy

 Inputs : idxTable - Table to destroy
 Outputs: RwBool - TRUE if successful.
 */
static RwBool
PatchSmoothIdxTableDestroy( PatchIdxTable *idxTable,
                            RwUInt32 size )
{
    RwUInt32 i;

    RWFUNCTION(RWSTRING("PatchSmoothIdxTableDestroy"));
    RWASSERT(NULL != idxTable);

    for( i = 0; i < size; i++ )
    {
        if(idxTable[i].idx != NULL)
        {
            RwFree(idxTable[i].idx);
        }

        idxTable[i].idx = (RxVertexIndex *)NULL;
        idxTable[i].count = 0;
        idxTable[i].size = 0;
    }

    RwFree(idxTable);

    RWRETURN(TRUE);
}

/*****************************************************************************
 PatchSmoothIdxTableAddIdx

 Inputs : idxTable - Index table to add an index to.
           i       - Element in table.
           j       - Index.
 Outputs: PatchIdxTable - The table if successful.
 */
static PatchIdxTable *
PatchSmoothIdxTableAddIdx( PatchIdxTable *idxTable,
                           RwUInt32 i,
                           RwUInt32 j)
{
    RwUInt32 newSize;
    RxVertexIndex *newIdx;
    RxVertexIndex *oldIdx;

    RWFUNCTION(RWSTRING("PatchSmoothIdxTableAddIdx"));
    RWASSERT(NULL != idxTable);

    if(idxTable[i].count == idxTable[i].size)
    {
        oldIdx = idxTable[i].idx;

        newSize = idxTable[i].size + PATCHSMOOTHIDXTABLESIZE;
        newIdx = (RxVertexIndex *)RwMalloc(newSize * sizeof(RxVertexIndex),
                                   rwID_PATCHPLUGIN | rwMEMHINTDUR_EVENT);
        RWASSERT(NULL != newIdx);

        if (oldIdx != NULL)
        {
            memcpy(newIdx, oldIdx, idxTable[i].count * sizeof(RxVertexIndex));

            RwFree(oldIdx);
        }

        idxTable[i].size = newSize;
        idxTable[i].idx = newIdx;
    }

    idxTable[i].idx[idxTable[i].count] = (RxVertexIndex)j;
    idxTable[i].count++;

    RWRETURN(idxTable);
}

/*****************************************************************************
 PatchSmoothIdxTablePopulate

 Inputs : vertPosTol - Control point position tolerance.
          positions  - The positions to smooth.
          numControlPoints - Number of control points.
          idxTable   - Index table to populate.
 Outputs: PatchIdxTable - PatchTable on success.
 */
static PatchIdxTable *
PatchSmoothIdxTablePopulate( RwReal vertPosTol,
                             RwV3d *positions,
                             RwUInt32 numControlPoints,
                             PatchIdxTable *idxTable )
{
    RwReal dx, dy, dz;
    RwUInt32 i,j;

    RWFUNCTION(RWSTRING("PatchSmoothIdxTablePopulate"));
    RWASSERT(NULL != positions);
    RWASSERT(NULL != idxTable);

    for( i = 0; i < numControlPoints; i++ )
    {
        for( j = (i + 1); j < numControlPoints; j++ )
        {
            dx = positions[i].x - positions[j].x;
            dy = positions[i].y - positions[j].y;
            dz = positions[i].z - positions[j].z;

            if( ( (dx >= -vertPosTol) && (dx <= vertPosTol) ) &&
                ( (dy >= -vertPosTol) && (dy <= vertPosTol) ) &&
                ( (dz >= -vertPosTol) && (dz <= vertPosTol) ) )
            {
                idxTable = PatchSmoothIdxTableAddIdx(idxTable, i, j);
                RWASSERT(NULL != idxTable);
                idxTable = PatchSmoothIdxTableAddIdx(idxTable, j, i);
                RWASSERT(NULL != idxTable);
            }
        }
    }

    RWRETURN(idxTable);
}

/*****************************************************************************
 PatchSmoothIdxTableSmoothNormal

 Inputs : vertNrmTol    - Vertex normal tolerance.
          sourceNormals - Source normals.
          targetNormals - Target normals.
          numControlPoints - Number of control points (normals).
          idxTable      - Index table to smooth the normals.
 Outputs: PatchIdxTable - Index table on success.
 */
static PatchIdxTable *
PatchSmoothIdxTableSmoothNormal( RwReal vertNrmTol,
                                 RwV3d *sourceNormals,
                                 RwV3d *targetNormals,
                                 RwUInt32 numControlPoints,
                                 PatchIdxTable *idxTable )
{
    RwV3d avgnrm;
    RwReal dot;
    RwReal length;

    RxVertexIndex *idx;

    RwUInt32 i,j;

    RWFUNCTION(RWSTRING("PatchSmoothIdxTableSmoothNormal"));
    RWASSERT(NULL != sourceNormals);
    RWASSERT(NULL != targetNormals);
    RWASSERT(NULL != idxTable);

    for(i = 0; i < numControlPoints; i++)
    {
        if(idxTable[i].count > 0)
        {
            avgnrm.x = sourceNormals[i].x;
            avgnrm.y = sourceNormals[i].y;
            avgnrm.z = sourceNormals[i].z;

            idx = idxTable[i].idx;

            for( j = 0; j < idxTable[i].count; j++ )
            {
                dot = RwV3dDotProduct( &sourceNormals[i],
                                       &sourceNormals[*idx] );

                if( dot >= vertNrmTol)
                {
                    avgnrm.x += sourceNormals[*idx].x;
                    avgnrm.y += sourceNormals[*idx].y;
                    avgnrm.z += sourceNormals[*idx].z;
                }

                idx++;
            }

            length = RwV3dLength(&avgnrm);

            if(length > (RwReal)0.0)
            {
                RwV3dScale( &targetNormals[i],
                            &avgnrm,
                            ((RwReal)1.0 / length) );
            }
        }
        else if(targetNormals != sourceNormals)
        {
            targetNormals[i].x = sourceNormals[i].x;
            targetNormals[i].y = sourceNormals[i].y;
            targetNormals[i].z = sourceNormals[i].z;
        }
    }

    RWRETURN(idxTable);
}

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

/*****************************************************************************
 _rpPatchSmoothNormal

 Smooth the patch meshes normals.

 Inputs :   patchMesh     - PatchMesh to smooth.
            vertPosTol    - Control point position tolerance.
            vertNrmTol    - Control point normal tolerance.
            sourceNormals - Location of normals to smooth.
            targetNormals - Destination of smoothed normals.
 Outputs:   PatchMesh - Returns the PatchMesh on success.
 */
PatchMesh *
_rpPatchSmoothNormal( PatchMesh *patchMesh,
                      RwReal vertPosTol,
                      RwReal vertNrmTol,
                      RwV3d *sourceNormals,
                      RwV3d *targetNormals )
{
    PatchIdxTable *idxTable;

    RwV3d *positions;

    RwUInt32 cp;

    RWFUNCTION(RWSTRING("_rpPatchSmoothNormal"));
    RWASSERT(NULL != patchMesh);

    /* Check we've got a source. */
    if(NULL == sourceNormals)
    {
        RWASSERT(_rpPatchMeshCheckFlag(patchMesh, rpPATCHMESHNORMALS));
        sourceNormals = patchMesh->userMesh.normals;
    }

    /* Check we've got a target. */
    if(NULL == targetNormals)
    {
        RWASSERT(_rpPatchMeshCheckFlag(patchMesh, rpPATCHMESHNORMALS));
        targetNormals = patchMesh->userMesh.normals;
    }

    /* First build an index array of co-incident points. */
    cp = _rpPatchMeshGetNumControlPoints(patchMesh);

    idxTable = PatchSmoothIdxTableCreate(cp);
    RWASSERT(NULL != idxTable);

    RWASSERT(_rpPatchMeshCheckFlag(patchMesh, rpPATCHMESHPOSITIONS));
    positions = patchMesh->userMesh.positions;

    /* Collect co-incident points. */
    idxTable = PatchSmoothIdxTablePopulate( vertPosTol,
                                            positions,
                                            cp,
                                            idxTable );
    RWASSERT(NULL != idxTable);

    /* Smooth the normals. */
    idxTable = PatchSmoothIdxTableSmoothNormal( vertNrmTol,
                                                sourceNormals,
                                                targetNormals,
                                                cp,
                                                idxTable );
    RWASSERT(NULL != idxTable);

    PatchSmoothIdxTableDestroy(idxTable, cp);

    RWRETURN(patchMesh);
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/
