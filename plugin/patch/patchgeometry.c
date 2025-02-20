/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   patchgeometry.c                                            -*
 *-                                                                         -*
 *-  Purpose :   General patch handling.                                    -*
 *-                                                                         -*
 *===========================================================================*/

/*===========================================================================*
 *--- Include files ---------------------------------------------------------*
 *===========================================================================*/
#include <rwcore.h>
#include <rpworld.h>

#include "rpplugin.h"
#include "rpdbgerr.h"

#include "rppatch.h"

#include "patchgeometry.h"
#include "patchskin.h"
#include "patchmesh.h"
#include "patchstream.h"
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
#define PATCHGEOMETRYSHAREx

/*===========================================================================*
 *--- Local Types -----------------------------------------------------------*
 *===========================================================================*/

/*===========================================================================*
 *--- Local Global Variables ------------------------------------------------*
 *===========================================================================*/
/*===========================================================================*
 *--- Local Defines ---------------------------------------------------------*
 *===========================================================================*/
#define PATCHFLAGTRUE(flag, bit)                                        \
    ((flag & bit) != 0)

/*===========================================================================*
 *--- Local functions -------------------------------------------------------*
 *===========================================================================*/

/*****************************************************************************
 PatchTagTriPatches

 Runs throught the tri patches flagging them as tri patches.
 This is done but, setting the u texcoord of the rpPATCHTRIPATCHINDEX
 control point to rpPATCHTRIPATCHFLAG.

 Inputs:
 Outputs:
 */
static PatchMesh *
PatchTagTriPatches( PatchMesh *patchMesh,
                    RwTexCoords *texCoords )
{
    RwUInt32 iPatch;

    RWFUNCTION(RWSTRING("PatchTagTriPatches"));
    RWASSERT(NULL != patchMesh);
    RWASSERT(NULL != patchMesh->triPatches);
    RWASSERT(NULL != patchMesh->quadPatches);
    RWASSERT(_rpPatchMeshCheckFlag(patchMesh, rpPATCHMESHTEXTURED));
    RWASSERT(NULL != (patchMesh->userMesh.texCoords[0]));

    for( iPatch = 0;
         iPatch < _rpPatchMeshGetNumTriPatches(patchMesh);
         iPatch++ )
    {
        TriPatch *triPatch;
        RwUInt32 index;

        triPatch = &(patchMesh->triPatches[iPatch]);
        RWASSERT(NULL != triPatch);

        index = triPatch->cpIndices[rpPATCHTRIPATCHINDEX];
        RWASSERT(index < _rpPatchMeshGetNumControlPoints(patchMesh));

        texCoords[index].u = rpPATCHTRIPATCHFLAG;
        texCoords[index].v = rpPATCHTRIPATCHFLAG;
    }

    for( iPatch = 0;
         iPatch < _rpPatchMeshGetNumQuadPatches(patchMesh);
         iPatch++ )
    {
        QuadPatch *quadPatch;
        RwUInt32 index;

        quadPatch = &(patchMesh->quadPatches[iPatch]);
        RWASSERT(NULL != quadPatch);

        index = quadPatch->cpIndices[rpPATCHTRIPATCHINDEX];
        RWASSERT(index < _rpPatchMeshGetNumControlPoints(patchMesh));

        texCoords[index].u = rpPATCHQUADPATCHFLAG;
        texCoords[index].v = rpPATCHQUADPATCHFLAG;
    }

    RWRETURN(patchMesh);
}

/*****************************************************************************
 PatchGeometryGenerateFlags

 Converts the patch meshes definition flags to geometry flags.

 Inputs: definitioFlag - The patch mesh definition flags to convert.
 Outputs: RwInt32 - The geometry flags version.
 */
static RwInt32
PatchGeometryGenerateFlags( RwUInt32 definitionFlag )
{
    RwInt32 flags;
    RwUInt32 numTexSets;

    RWFUNCTION(RWSTRING("PatchGeometryGenerateFlags"));

    /* Need to convert patch flags to goemetry flags. */
    flags = rpGEOMETRYTRISTRIP;

    flags |= rpGEOMETRYPOSITIONS *
             PATCHFLAGTRUE(definitionFlag, rpPATCHMESHPOSITIONS);
    flags |= rpGEOMETRYTEXTURED *
             PATCHFLAGTRUE(definitionFlag, rpPATCHMESHTEXTURED);
    flags |= rpGEOMETRYPRELIT *
             PATCHFLAGTRUE(definitionFlag, rpPATCHMESHPRELIGHTS);
    flags |= rpGEOMETRYNORMALS *
             PATCHFLAGTRUE(definitionFlag, rpPATCHMESHNORMALS);
    flags |= rpGEOMETRYLIGHT *
             PATCHFLAGTRUE(definitionFlag, rpPATCHMESHLIGHT);
    flags |= rpGEOMETRYMODULATEMATERIALCOLOR *
             PATCHFLAGTRUE(definitionFlag, rpPATCHMESHMODULATEMATERIALCOLOR);

    numTexSets = _rpPatchMeshFlagGetNumTexCoords(definitionFlag);
    RWASSERT(rwMAXTEXTURECOORDS > numTexSets);

    flags |= rpGEOMETRYTEXCOORDSETS(numTexSets);

    RWRETURN(flags);
}

/*****************************************************************************
 PatchGeometryGenerateLockFlags

 Converts patch mesh lock flags into geometry lock flags.

 Inputs: patchLockFlags - The patch mesh lock flags to convert.
 Outputs: RwUInt32      - The constructed geometry lock flags.
 */
static RwInt32
PatchGeometryGenerateLockFlags( RwUInt32 patchLockFlags )
{
    RwInt32 lockFlags;

    RWFUNCTION(RWSTRING("PatchGeometryGenerateLockFlags"));

    /* Need to convert patch lock flags to goemetry lock flags. */
    lockFlags = 0;

    lockFlags |= rpGEOMETRYLOCKPOLYGONS *
                 PATCHFLAGTRUE(patchLockFlags, rpPATCHMESHLOCKPATCHES);
    lockFlags |= rpGEOMETRYLOCKVERTICES *
                 PATCHFLAGTRUE(patchLockFlags, rpPATCHMESHLOCKPOSITIONS);
    lockFlags |= rpGEOMETRYLOCKNORMALS *
                 PATCHFLAGTRUE(patchLockFlags, rpPATCHMESHLOCKNORMALS);
    lockFlags |= rpGEOMETRYLOCKPRELIGHT *
                 PATCHFLAGTRUE(patchLockFlags, rpPATCHMESHLOCKPRELIGHTS);
    lockFlags |= rpGEOMETRYLOCKTEXCOORDS1 *
                 PATCHFLAGTRUE(patchLockFlags, rpPATCHMESHLOCKTEXCOORDS1);
    lockFlags |= rpGEOMETRYLOCKTEXCOORDS2 *
                 PATCHFLAGTRUE(patchLockFlags, rpPATCHMESHLOCKTEXCOORDS2);
    lockFlags |= rpGEOMETRYLOCKTEXCOORDS3 *
                 PATCHFLAGTRUE(patchLockFlags, rpPATCHMESHLOCKTEXCOORDS3);
    lockFlags |= rpGEOMETRYLOCKTEXCOORDS4 *
                 PATCHFLAGTRUE(patchLockFlags, rpPATCHMESHLOCKTEXCOORDS4);
    lockFlags |= rpGEOMETRYLOCKTEXCOORDS5 *
                 PATCHFLAGTRUE(patchLockFlags, rpPATCHMESHLOCKTEXCOORDS5);
    lockFlags |= rpGEOMETRYLOCKTEXCOORDS6 *
                 PATCHFLAGTRUE(patchLockFlags, rpPATCHMESHLOCKTEXCOORDS6);
    lockFlags |= rpGEOMETRYLOCKTEXCOORDS7 *
                 PATCHFLAGTRUE(patchLockFlags, rpPATCHMESHLOCKTEXCOORDS7);
    lockFlags |= rpGEOMETRYLOCKTEXCOORDS8 *
                 PATCHFLAGTRUE(patchLockFlags, rpPATCHMESHLOCKTEXCOORDS8);

    RWRETURN(lockFlags);
}

/*****************************************************************************
 PatchGeometryCreateMeshHeader

 Creates the patch meshes geometry RpMeshHeader.

 Inputs: patch - The PatchMesh to construct a mesh header for.
 Outputs: RpMeshHeader * - The constructed mesh header.
 */
static RpMeshHeader *
PatchGeometryCreateMeshHeader(PatchMesh *patch)
{
    RpMeshHeader *meshHeader;
    RpMesh *mesh;
    RxVertexIndex *meshIndices;

    RwUInt32 numIndices;
    RwUInt32 numMeshes;

    RwUInt32 size;
    RwUInt32 iMesh;

    RWFUNCTION(RWSTRING("PatchGeometryCreateMeshHeader"));
    RWASSERT(NULL != patch);

    /*
     * This function generates the RpMeshHeader for an RpGeometry.
     * We need to fillin the following structures:
     *
     * struct RpMesh
     * {
     *     RxVertexIndex *indices;    < vertex indices defining the mesh
     *     RwUInt32       numIndices; < number of vertices in mesh
     *     RpMaterial    *material;   < pointer to material used
     *                                  to render the mesh.
     * };
     *
     * struct RpMeshHeader
     * {
     *     RwUInt32 flags;              < see \see RpMeshHeaderFlags
     *     RwUInt16 numMeshes;          < number of meshes in object
     *     RwUInt16 serialNum;          < Determine if mesh has changed
     *                                    since last instance
     *     RwUInt32 totalIndicesInMesh; < Total triangle index
     *                                    count in all meshes
     *     RwUInt32 firstMeshOffset;    < offset in bytes from end this struct
     *                                    RpMeshHeader to the first mesh
     * };
     *
     */

    /* Get the number of numIndeces. */
    numIndices = (_rpPatchMeshGetNumQuadPatches(patch) +
                  _rpPatchMeshGetNumTriPatches(patch)) *
                 rpQUADPATCHNUMCONTROLINDICES;

    /* Get the number of meshes. */
    numMeshes = (RwUInt32)(patch->matList.numMaterials);

    /* Calculate size of mesh header. */
    size = (sizeof(RpMeshHeader)) +
           (sizeof(RpMesh) * numMeshes) +
           (sizeof(RxVertexIndex) * numIndices);

    /* Create the mesh header. */
    meshHeader = _rpMeshHeaderCreate(size);
    RWASSERT(NULL != meshHeader);
    memset(meshHeader, 0, sizeof(RpMeshHeader));

    /* Set the mesh headers' primitive type. */
    meshHeader = RpMeshHeaderSetPrimType(meshHeader, rwPRIMTYPEPOINTLIST);
    RWASSERT(NULL != meshHeader);

    /* Get the first mesh. */
    mesh = (RpMesh *)(meshHeader + 1);
    meshIndices = (RxVertexIndex *)(mesh + numMeshes);

    /* Setup the mesh header. */
    meshHeader->numMeshes = (RwUInt16)numMeshes;
    meshHeader->serialNum = _rpMeshGetNextSerialNumber();
    meshHeader->firstMeshOffset = 0;
    meshHeader->totalIndicesInMesh = numIndices;

    /* Lets start setting up the meshes. */
    for( iMesh = 0; iMesh < numMeshes; iMesh++ )
    {
        RwUInt32 meshNumIndices;
        RpMaterial *material;

        RwUInt32 iQuad;
        RwUInt32 iTri;

        /* Reset the meshNumIndices and get the material. */
        meshNumIndices = 0;
        material = patch->matList.materials[iMesh];

        mesh->indices = meshIndices;
        mesh->material = patch->matList.materials[iMesh];

        for( iQuad = 0; iQuad < _rpPatchMeshGetNumQuadPatches(patch); iQuad++ )
        {
            QuadPatch *quad;

            /* Get the quad. */
            quad = &(patch->quadPatches[iQuad]);

            /* Is this quad part of the mesh? */
            if(quad->matIndex == iMesh)
            {
                _rpQuadPatchAssignVertexIndices(quad, meshIndices);
                meshIndices += rpQUADPATCHNUMVERTEXINDICES;
                meshNumIndices += rpQUADPATCHNUMVERTEXINDICES;
            }
        }

        for( iTri = 0; iTri < _rpPatchMeshGetNumTriPatches(patch); iTri++ )
        {
            TriPatch *tri;

            /* Get the tri. */
            tri = &(patch->triPatches[iTri]);

            /* Is the tri part of the mesh? */
            if(tri->matIndex == iMesh)
            {
                _rpTriPatchAssignVertexIndices(tri, meshIndices);
                meshIndices += rpTRIPATCHNUMVERTEXINDICES;
                meshNumIndices += rpTRIPATCHNUMVERTEXINDICES;
            }
        }

        /* Record the number of indices. */
        mesh->numIndices = meshNumIndices;

        /* Move onto the next mesh. */
        mesh++;
    }

    RWASSERT((RxVertexIndex *)mesh == ((RpMesh *)(meshHeader + 1))->indices);

    RWRETURN(meshHeader);
}

/*===========================================================================*
 *--- Private functions -----------------------------------------------------*
 *===========================================================================*/

/*****************************************************************************
 _rpPatchGeometryCreateFromPatchMesh

 Creates a suitable RpGeometry to store the RpPatchMesh.

 Inputs: patchMesh - PatchMesh to construct an RpGeometry for.
 Outputs: RpGeometry * - The fresh empty RpGeometry.
 */
RpGeometry *
_rpPatchGeometryCreateFromPatchMesh( PatchMesh *patchMesh )
{
    RpGeometry *geometry;
    PatchGeometryData *geometryData;

    /* Geometry create variables. */
    RwUInt32 numVerts;
    RwUInt32 numTriangles;
    RwUInt32 numControlPoints;
    RwUInt32 flags;

    RWFUNCTION(RWSTRING("_rpPatchGeometryCreateFromPatchMesh"));
    RWASSERT(NULL != patchMesh);

    /* Setup the geometry create. */
    numControlPoints = _rpPatchMeshGetNumControlPoints(patchMesh);
    numVerts = (RwInt32)numControlPoints;
    numTriangles = 0;

#if (defined(PATCHGEOMETRYSHARE))
    flags = rpGEOMETRYTRISTRIP;
#else /* (defined(PATCHGEOMETRYSHARE)) */
    flags = PatchGeometryGenerateFlags(_rpPatchMeshGetFlags(patchMesh));
#endif /* (defined(PATCHGEOMETRYSHARE)) */

    /* Lets create an empty geometry. */
    geometry = RpGeometryCreate(numVerts, numTriangles, flags);
    RWASSERT(NULL != geometry);

    /* Get the patch geometry extension. */
    geometryData = PATCHGEOMETRYGETDATA(geometry);
    RWASSERT(NULL != geometryData);

    /* Store the mesh in the geometry. */
    geometryData->mesh = patchMesh;

    /* Inc ref count. */
    patchMesh = _rpPatchMeshAddRef(patchMesh);
    RWASSERT(NULL != patchMesh);

    /*
     * We no longer create the mesh or fill in the geometry's data here
     * instead it happens during the unlock.
     */

    /* Check the geometry has been setup correctly. */
    RWASSERT(0 == geometry->numTriangles);
    RWASSERT(NULL == geometry->triangles);
    RWASSERT(_rpPatchMeshGetNumControlPoints(patchMesh) ==
             (RwUInt32)(geometry->numVertices));
    RWASSERT(1 == geometry->numMorphTargets);
    RWASSERT(NULL != geometry->morphTarget);

    RWRETURN(geometry);
}

/*****************************************************************************
 _rpPatchGeometryTransferFromPatchMesh

 Converts the patch meshes control point data into the geomtry.
 Call when the patch meshes geometry is first create and whenever the
 patch mesh is locked and edited.
 We make the assumption that the patch mesh already contains a build geometry.

 Initially the patch mesh is created with all it's elements locked, hence we
 can test the patch mesh lock flags to see what needs transfered into the
 geometry.

 Inputs:  patchMesh   - Pointer to the patchMesh which will have it's control
                        points transfered into it's geometry.
 Outputs: PatchMesh * - Returns the patch mesh if everything transfered
                        successfully.
 */
PatchMesh *
_rpPatchGeometryTransferFromPatchMesh( PatchMesh *patchMesh )
{
    RpGeometry *geometry;
    RpMorphTarget *morphTarget;

    RwUInt32 numControlPoints;
    RwUInt32 geometryLockMode;

#if(!defined(PATCHGEOMETRYSHARE))
    RwUInt32 cp;
#endif /* (!defined(PATCHGEOMETRYSHARE)) */

    RWFUNCTION(RWSTRING("_rpPatchGeometryTransferFromPatchMesh"));
    RWASSERT(NULL != patchMesh);
    RWASSERT(_rpPatchMeshCheckFlag(patchMesh, rpPATCHMESHPOSITIONS));

    /* For now we also assert that the number of texture coord sets */
    /* is sufficent at least one if we have any tri patches.        */
    RWASSERT( (0 < _rpPatchMeshGetNumTexCoordSets(patchMesh)) ||
              (0 == _rpPatchMeshGetNumTriPatches(patchMesh)) );

    numControlPoints = _rpPatchMeshGetNumControlPoints(patchMesh);

    geometry = _rpPatchMeshGetGeometry(patchMesh);
    RWASSERT(NULL != geometry);
    RWASSERT(NULL != patchMesh->geometry);

    morphTarget = RpGeometryGetMorphTarget(geometry, 0);
    RWASSERT(NULL != morphTarget);

    /* We lock the geometry. */
    geometryLockMode = PatchGeometryGenerateLockFlags(patchMesh->lockMode);
    geometry = RpGeometryLock(geometry, geometryLockMode);
    RWASSERT(NULL != geometry);

    /* Do we need to update the patch meshes mesh header. */
    if(_rpPatchMeshCheckLocked(patchMesh, rpPATCHMESHLOCKPATCHES))
    {
        RpMeshHeader *meshHeader;
        RwUInt32 iMaterial;

        /*
         * At the moment we need to destroy the present mesh header and
         * construct a new one, because the number of meshes / materials
         * might have changed.
         */

        /* Is there a mesh header there already? */
        if(NULL != geometry->mesh)
        {
            _rpMeshHeaderDestroy(geometry->mesh);
            geometry->mesh = (RpMeshHeader *)NULL;
        }

        /* Deinitialize the material list.  */
        _rpMaterialListDeinitialize(&(geometry->matList));

        /* Need to build the geometries mesh. */
        meshHeader = PatchGeometryCreateMeshHeader(patchMesh);
        RWASSERT(NULL != meshHeader);

        /* Add the mesh header to the geometry. */
        RWASSERT(NULL == geometry->mesh);
        geometry->mesh = meshHeader;

        /* Need to setup the geometries material list. */
        for( iMaterial = 0;
             iMaterial < (RwUInt32)(patchMesh->matList.numMaterials);
             iMaterial++ )
        {
            RpMaterial *material;

            material = patchMesh->matList.materials[iMaterial];
            RWASSERT(NULL != material);

            _rpMaterialListAppendMaterial(&(geometry->matList), material);
        }
    }

#if (defined(PATCHGEOMETRYSHARE))

    {
        RwUInt32 geometryFlags;
        RwUInt32 iTextureSet;

        /* Get the goemetry flags. */
        geometryFlags = PatchGeometryGenerateFlags(
                            _rpPatchMeshGetFlags(patchMesh) );

        /* Set them. */
        RpGeometrySetFlags(geometry, geometryFlags);

        /* Share up the patch meshes data. */
        morphTarget->verts   = _rpPatchMeshGetPositions(patchMesh);
        morphTarget->normals = _rpPatchMeshGetNormals(patchMesh);
        geometry->preLitLum  = _rpPatchMeshGetPreLightColors(patchMesh);

        for( iTextureSet = 0; iTextureSet < rwMAXTEXTURECOORDS; iTextureSet++ )
        {
            geometry->texCoords[iTextureSet] =
                _rpPatchMeshGetTexCoords(patchMesh, iTextureSet);
        }
    }

#else /* (defined(PATCHGEOMETRYSHARE)) */

    /* Do we need to update the geometry's positions? */
    if( _rpPatchMeshCheckLocked(patchMesh, rpPATCHMESHLOCKPOSITIONS) &&
        _rpPatchMeshCheckFlag(patchMesh, rpPATCHMESHPOSITIONS) )
    {
        RwSphere boundingSphere;

        RwV3d *patchPositions;
        RwV3d *geomPositions;

        /* Get the geometry's positions. */
        geomPositions = RpMorphTargetGetVertices(morphTarget);
        RWASSERT(NULL != geomPositions);

        /* Get the patch meshes positions. */
        patchPositions = _rpPatchMeshGetPositions(patchMesh);
        RWASSERT(NULL != patchPositions);

        /* Copy the control points into the vertices. */
        for( cp = 0; cp< numControlPoints; cp++ )
        {
            geomPositions[cp] = patchPositions[cp];
        }

        RpMorphTargetCalcBoundingSphere(morphTarget, &boundingSphere);
        RpMorphTargetSetBoundingSphere(morphTarget, &boundingSphere);
    }

    /* Do we need to update the geometry's normals? */
    if( _rpPatchMeshCheckLocked(patchMesh, rpPATCHMESHLOCKNORMALS) &&
        _rpPatchMeshCheckFlag(patchMesh, rpPATCHMESHNORMALS) )
    {
        RwV3d *patchNormals;
        RwV3d *geomNormals;

        /* Get the geometry's normals. */
        geomNormals = RpMorphTargetGetVertexNormals(morphTarget);
        RWASSERT(NULL != geomNormals);

        /* Get the patch mesh normals. */
        patchNormals = _rpPatchMeshGetNormals(patchMesh);
        RWASSERT(NULL != patchMesh);

        /* Do we need to smooth these normals? */
        if(_rpPatchMeshCheckFlag(patchMesh, rpPATCHMESHSMOOTHNORMALS))
        {
            RwReal vertPosTol;
            RwReal vertNrmTol;

            vertPosTol = PATCHSMOOTHVERTPOSTOL;
            vertNrmTol = PATCHSMOOTHVERTNRMTOL;

            patchMesh = _rpPatchSmoothNormal( patchMesh,
                                              vertPosTol,
                                              vertNrmTol,
                                              patchNormals,
                                              geomNormals );
            RWASSERT(NULL != patchMesh);
        }
        else
        {
           /* Copy the control points into the vertices. */
            for( cp = 0; cp< numControlPoints; cp++ )
            {
                geomNormals[cp] = patchNormals[cp];
            }
        }
    }

    /* Do we need to update the geometry's pre-light colours. */
    if( _rpPatchMeshCheckLocked(patchMesh, rpPATCHMESHLOCKPRELIGHTS) &&
        _rpPatchMeshCheckFlag(patchMesh, rpPATCHMESHPRELIGHTS) )
    {
        RwRGBA *patchPreLightColours;
        RwRGBA *geomPreLightColours;

        /* Get the geometry's pre-light colours. */
        geomPreLightColours = RpGeometryGetPreLightColors(geometry);
        RWASSERT(NULL != geomPreLightColours);

        /* Get the patch meshes pre-light colours. */
        patchPreLightColours = _rpPatchMeshGetPreLightColors(patchMesh);
        RWASSERT(NULL != patchPreLightColours);

        /* Copy the control points into the vertices. */
        for( cp = 0; cp< numControlPoints; cp++ )
        {
            geomPreLightColours[cp] = patchPreLightColours[cp];
        }
    }

    /* Does the patch mesh have any texCoord sets? */
    if( 0 < _rpPatchMeshGetNumTexCoordSets(patchMesh) )
    {
        RwUInt32 iTextureSet;

        for( iTextureSet = 0; iTextureSet < rwMAXTEXTURECOORDS; iTextureSet++ )
        {
            RwTexCoords *patchTexCoords;

            RwUInt32 lockTexCoords;

            lockTexCoords = rpPATCHMESHLOCKTEXCOORDS1 << iTextureSet;

            /* Get the patch meshes texCoord set. */
            patchTexCoords = _rpPatchMeshGetTexCoords( patchMesh,
                                                       iTextureSet );

            if( (_rpPatchMeshCheckLocked(patchMesh, lockTexCoords)) &&
                (NULL != patchTexCoords) )
            {
                RwTexCoords *geomTexCoords;

                /* Get the geometry's texCoord set. */
                geomTexCoords =
                    RpGeometryGetVertexTexCoords( geometry,
                        (RwTextureCoordinateIndex)(iTextureSet + 1) );
                RWASSERT(NULL != geomTexCoords);

                /* Copy across the texture coords. */
                for( cp = 0; cp < numControlPoints; cp++ )
                {
                    geomTexCoords[cp] = patchTexCoords[cp];
                }
            }
        }
    }

#endif /* (defined(PATCHGEOMETRYSHARE)) */

    /* We might need to flag the tri patches as tri patches.  */
    if( (_rpPatchMeshCheckLocked(patchMesh, rpPATCHMESHLOCKPATCHES)) ||
        (_rpPatchMeshCheckLocked(patchMesh, rpPATCHMESHLOCKTEXCOORDS1)) )
    {
        RwTexCoords *geomTexCoords;

        /* Get the geometry's texCoord set. */
        geomTexCoords = RpGeometryGetVertexTexCoords( geometry,
                            (RwTextureCoordinateIndex)1 );

        /* Flag the tri patches as tri patches.  */
        /* And the quad patches as quad patches. */
        if (NULL != geomTexCoords)
        {
            patchMesh = PatchTagTriPatches(patchMesh, geomTexCoords);
            RWASSERT(NULL != patchMesh);
        }
    }

    /* Need to unlock the geometry, here. */
    geometry = RpGeometryUnlock(geometry);
    RWASSERT(NULL != geometry);

    RWRETURN(patchMesh);
}

/*===========================================================================*
 *--- Plugin Engine Functions -----------------------------------------------*
 *===========================================================================*/

/*****************************************************************************
 _rpPatchGeometryConstructor

 Geometry patch extension constructor.

 Inputs: object - RpGeometry.
         offset - Offset of Patch data in the RpGeometry.
         size   - Size of patch data in the RpGeometry.
 Outputs: RpGeometry - the geometry.
 */
void *
_rpPatchGeometryConstructor( void *object,
                             RwInt32 offset __RWUNUSED__,
                             RwInt32 size __RWUNUSED__ )
{
    RpGeometry *geometry;
    PatchGeometryData *geometryData;

    RWFUNCTION(RWSTRING("_rpPatchGeometryConstructor"));
    RWASSERT(NULL != object);

    geometry = (RpGeometry *)object;
    RWASSERT(NULL != geometry);

    geometryData = PATCHGEOMETRYGETDATA(geometry);
    RWASSERT(NULL != geometryData);

    geometryData->mesh = (PatchMesh *)NULL;
    geometryData->skin = (RpSkin *)NULL;

    RWRETURN(object);
}

/*****************************************************************************
 _rpPatchGeometryDestructor

 Geoemtry patch extension destructor.

 Inputs: object - RpGeometry.
         offset - Offset of Patch data in the RpGeometry.
         size   - Size of patch data in the RpGeometry.
 Outputs: RpGeometry - the geometry.
 */
void *
_rpPatchGeometryDestructor( void *object,
                            RwInt32 offset __RWUNUSED__,
                            RwInt32 size __RWUNUSED__ )
{
    RpGeometry *geometry;
    PatchGeometryData *geometryData;

    RWFUNCTION(RWSTRING("_rpPatchGeometryDestructor"));
    RWASSERT(NULL != object);

    geometry = (RpGeometry *)object;
    RWASSERT(NULL != geometry);

    geometryData = PATCHGEOMETRYGETDATA(geometry);
    RWASSERT(NULL != geometryData);

    if(NULL != geometryData->mesh)
    {
        RwBool success;

        success = _rpPatchMeshDestroy(geometryData->mesh);
        RWASSERT(TRUE == success);
    }

    geometryData->mesh = (PatchMesh *)NULL;
    geometryData->skin = (RpSkin *)NULL;

    RWRETURN(object);
}

/*****************************************************************************
 _rpPatchGeometryCopy

 Geoemtry patch extension copy.

 Inputs: dstObject - Destination geometry.
         srcObject - Source geometry.
         offset - Offset of Patch data in the RpGeometry.
         size   - Size of patch data in the RpGeometry.
 Outputs: RpGeometry - The destination geometry.
 */
void *
_rpPatchGeometryCopy( void *dstObject,
                      const void *srcObject,
                      RwInt32 offset __RWUNUSED__,
                      RwInt32 size __RWUNUSED__ )
{
    const RpGeometry *srcGeometry;
    RpGeometry *dstGeometry;
    const PatchGeometryData *srcGeometryData;
    PatchGeometryData *dstGeometryData;

    RWFUNCTION(RWSTRING("_rpPatchGeometryCopy"));
    RWASSERT(NULL != dstObject);
    RWASSERT(NULL != srcObject);

    srcGeometry = (const RpGeometry *)srcObject;
    RWASSERT(NULL != srcGeometry);
    dstGeometry = (RpGeometry *)dstObject;
    RWASSERT(NULL != dstGeometry);

    srcGeometryData = PATCHGEOMETRYGETCONSTDATA(srcGeometry);
    RWASSERT(NULL != srcGeometryData);
    dstGeometryData = PATCHGEOMETRYGETDATA(dstGeometry);
    RWASSERT(NULL != dstGeometryData);

    dstGeometryData->mesh = srcGeometryData->mesh;
    dstGeometryData->skin = _rpPatchSkinGeometryGetSkin(dstGeometry);

    if(NULL != dstGeometryData->mesh)
    {
        /*
         * Adding a reference may confuse the count.
         * When a geometry is attached to an atomic it's referenced count is
         * increased, and the meshes ref count is increased.
         * But as the geomtry now has ref count of two and the meshes has a ref
         * count of two, when the atomics are destroyed the mesh will not be
         * destroyed.
         */
        dstGeometryData->mesh = _rpPatchMeshAddRef(dstGeometryData->mesh);
        RWASSERT(NULL != dstGeometryData->mesh);
    }

    RWRETURN(dstObject);
}

/*****************************************************************************
 _rpPatchGeometryGetSize

 Patch geoemtry stream get size.

 Inputs: object - RpGeometry object.
         offset - Offset of patch extension data.
         size   - Size of patch extension data.
 Outputs: RwInt32 - Size of extension data in stream.
 */
RwInt32
_rpPatchGeometryGetSize( const void *object,
                         RwInt32 offset __RWUNUSED__,
                         RwInt32 size __RWUNUSED__ )
{
    const RpGeometry *geometry;
    const PatchGeometryData *geometryData;

    RwInt32 sizeTotal;

    RWFUNCTION(RWSTRING("_rpPatchGeometryGetSize"));
    RWASSERT(NULL != object);

    geometry = (const RpGeometry *)object;
    RWASSERT(NULL != geometry);
    geometryData = PATCHGEOMETRYGETCONSTDATA(geometry);
    RWASSERT(NULL != geometryData);

    /* Setup the size to zero. */
    sizeTotal = 0;

    /* If we have a patch mesh then get its size. */
    if(NULL != geometryData->mesh)
    {
        sizeTotal = _rpPatchMeshStreamGetSize(geometryData->mesh);
    }

    RWRETURN(sizeTotal);
}

/*****************************************************************************
 _rpPatchGeometryRead

 Patch geometry stream read.

 Inputs: stream - RwStream to read from.
         length - Length of patch data in stream.
         object - RpGeometry object we're streaming.
         offset - Offset of patch extension in geometry.
         size   - Size of patch externsion data.
 Outputs: RwStream * - The stream if successful.
 */
RwStream *
_rpPatchGeometryRead( RwStream *stream,
                      RwInt32 length __RWUNUSED__,
                      void *object,
                      RwInt32 offset __RWUNUSED__,
                      RwInt32 size __RWUNUSED__ )
{
    RpGeometry *geometry;
    PatchGeometryData *geometryData;

    RWFUNCTION(RWSTRING("_rpPatchGeometryRead"));
    RWASSERT(NULL != stream);
    RWASSERT(NULL != object);

    geometry = (RpGeometry *)object;
    RWASSERT(NULL != geometry);
    geometryData = PATCHGEOMETRYGETDATA(geometry);
    RWASSERT(NULL != geometryData);

    geometryData->mesh = _rpPatchMeshStreamRead(stream);
    RWASSERT(NULL != geometryData->mesh);

    geometryData->mesh->geometry = geometry;
    RpGeometryAddRef(geometryData->mesh->geometry);

    RWRETURN(stream);
}

/*****************************************************************************
 _rpPatchGeometryWrite

 Patch geometry write.

 Inputs: stream - RwStream to write to.
         length - Length of patch data in stream.
         object - RpGeometry object we're streaming.
         offset - Offset of patch extension in geometry.
         size   - Size of patch externsion data.
 Outputs: RwStream * - The stream if successful.
 */
RwStream *
_rpPatchGeometryWrite( RwStream *stream,
                       RwInt32 length __RWUNUSED__,
                       const void *object,
                       RwInt32 offset __RWUNUSED__,
                       RwInt32 size __RWUNUSED__ )
{
    const RpGeometry *geometry;
    const PatchGeometryData *geometryData;

    const PatchMesh *mesh;

    RWFUNCTION(RWSTRING("_rpPatchGeometryWrite"));
    RWASSERT(NULL != stream);
    RWASSERT(NULL != object);

    geometry = (const RpGeometry *)object;
    RWASSERT(NULL != geometry);
    geometryData = PATCHGEOMETRYGETCONSTDATA(geometry);
    RWASSERT(NULL != geometryData);
    mesh = (const PatchMesh *)geometryData->mesh;

    stream = _rpPatchMeshStreamWrite(mesh, stream);
    RWASSERT(NULL != stream);

    RWRETURN(stream);
}

/*****************************************************************************
 _rpPatchGeometryAlways

 Patch geometry always callback. Setup the patch geometry with the correct
 skin.

 Inputs: object - RpGeometry object to setup.
         offset - Offset of patch extension in geometry.
         size   - Size of patch externsion data.
 Outputs: RwBool - TRUE.
 */
RwBool
_rpPatchGeometryAlways( void *object,
                        RwInt32 offset __RWUNUSED__,
                        RwInt32 size   __RWUNUSED__ )
{
    RpGeometry *geometry;
    PatchGeometryData *geometryData;

    RpSkin *skin;

    RWFUNCTION(RWSTRING("_rpPatchGeometryAlways"));
    RWASSERT(NULL != object);

    /* Grab the geometry and geomtryData. */
    geometry = (RpGeometry *)object;
    RWASSERT(NULL != geometry);
    geometryData = PATCHGEOMETRYGETDATA(geometry);
    RWASSERT(NULL != geometryData);

    /* Setup the skin correctly. */
    skin = _rpPatchSkinGeometryGetSkin(geometry);
    geometryData->skin = skin;

    RWRETURN(TRUE);
}

/*===========================================================================*
 *--- Plugin API Functions --------------------------------------------------*
 *===========================================================================*/
