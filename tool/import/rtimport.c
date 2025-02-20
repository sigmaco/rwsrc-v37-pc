/**
 * \ingroup rtworldimport
 * \page rtworldimportoverview RtWorldImport Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rprandom.h, rtimport.h
 * \li \b Libraries: rwcore, rpworld, rprandom, rtimport, rtgcond, rtwing
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach, \ref RpRandomPluginAttach
 *
 * \subsection importoverview Overview
 *
 * The functions in this toolkit are used to create a static world within an
 * \ref RpWorld object and generate the associated compressed BSP data.
 *
 * The developer would normally create an \ref RtWorldImport object, initialize
 * it with all the static data needed for the final \ref RpWorld object, fill
 * the \ref RtWorldImportParameters structure with the required settings and
 * then call \ref RtWorldImportCreateWorld to generate the BSP tree.
 *
 * Callbacks are provided to increase flexibility.
 */

/*
 * Converting no hs worlds to real binary worlds (with bsps).
 * No HS worlds are used in the generation process of worlds
 *
 * The NoHS format of the world is a simplified format where all of the
 * materials, polygons and clumps are separated out such that it is possible
 * to and save and load a world quickly. This form is not suitable for
 * rendering - and must be converted into a RpWorld before rendering can
 * occur
 */

/****************************************************************************
 Includes
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "rwcore.h"
#include "rpworld.h"

#include "rpdbgerr.h"

#include "nhsstats.h"
#include "nhsutil.h"
#include "nhsworld.h"
#include "rtgcond.h"
//#include "nhswing.h"
#include "nhssplit.h"

#include "rtimport.h"

#if (defined(WIN32) && !defined(_XBOX))
#include <windows.h>
#endif /* (defined(WIN32) && !defined(_XBOX)) */

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

RwInt32             _rtWorldImportTotalPolysInWorld = 0;
RwInt32             _rtWorldImportNumPolysInLeaves = 0;
RwInt32             _rtWorldImportNumPolysInCompressedLeaves = 0;


RtWorldImportParameters* WorldImportParams;
RtWorldImportHints *WorldImportHints[2] = {(RtWorldImportHints*)NULL,(RtWorldImportHints*)NULL};

RtWorldImportBuildCallBacks BuildCallBacks = {
    (RtWorldImportPartitionBuildCallBack) NULL,
    (void *)NULL,
    (RtWorldImportTerminationBuildCallBack) NULL,
    (void *)NULL
};


/****************************************************************************
 Local (static) Globals
 */

static RwBool       ExitRequested = FALSE;

static RtWorldImportProgressCallBack progressCallBack =
    (RtWorldImportProgressCallBack) NULL;

static RtWorldImportUserdataCallBacks UserDataCallBacks = {
    (RtWorldImportDestroyVertexUserdataCallBack) NULL,
    (RtWorldImportCloneVertexUserdataCallBack) NULL,
    (RtWorldImportInterpVertexUserdataCallBack) NULL,
    (RtWorldImportDestroyPolygonUserdataCallBack) NULL,
    (RtWorldImportSplitPolygonUserdataCallBack) NULL,
    (RtWorldImportSectorSetVertexUserdataCallBack) NULL,
    (RtWorldImportSectorSetPolygonUserdataCallBack) NULL
};

/****************************************************************************
 Functions
 */

/****************************************************************************
 Static Functions
 */

static              RwBool
ImportWorldFreeVertices(RtWorldImport * nohsworld)
{
    RWFUNCTION(RWSTRING("ImportWorldFreeVertices"));
    RWASSERT(nohsworld);

    if (nohsworld)
    {
        if (nohsworld->vertices)
        {
            RtWorldImportDestroyVertexUserdataCallBack
                destroyVertexUserdata =
                UserDataCallBacks.destroyVertexUserdata;

            if (destroyVertexUserdata)
            {
                RwInt32             i;
                RtWorldImportVertex *vertices = nohsworld->vertices;

                for (i = 0; i < nohsworld->numVertices; i++)
                {
                    if (vertices[i].pUserdata)
                    {
                        destroyVertexUserdata(&vertices[i].pUserdata);
                    }
                }
            }
            RwFree(nohsworld->vertices);
            nohsworld->numVertices = 0;
        }

        RWRETURN(TRUE);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN(FALSE);
}

static              RwBool
ImportWorldFreeTriangles(RtWorldImport * nohsworld)
{
    RWFUNCTION(RWSTRING("ImportWorldFreeTriangles"));
    RWASSERT(nohsworld);

    if (nohsworld)
    {
        if (nohsworld->polygons)
        {
            RtWorldImportDestroyPolygonUserdataCallBack
                destroyPolygonUserdata =
                UserDataCallBacks.destroyPolygonUserdata;

            if (destroyPolygonUserdata)
            {
                RwInt32             i;
                RtWorldImportTriangle *polygons = nohsworld->polygons;

                for (i = 0; i < nohsworld->numPolygons; i++)
                {
                    if (polygons[i].pUserdata)
                    {
                        destroyPolygonUserdata(&polygons[i].pUserdata);
                    }
                }
            }
            RwFree(nohsworld->polygons);
            nohsworld->numPolygons = 0;
        }

        RWRETURN(TRUE);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN(FALSE);
}

/****************************************************************************
 SPI Functions
 */

void
_rtImportWorldSendProgressMessage(RwInt32 msgtype, RwReal value)
{
    RWFUNCTION(RWSTRING("_rtImportWorldSendProgressMessage"));
    if (progressCallBack)
    {
        if (!progressCallBack(msgtype, value))
        {
            ExitRequested = TRUE;
        }
    }

    RWRETURNVOID();
}

void
_rtImportWorldGetDimensions(RtWorldImport* importWorld, RwReal* min, RwReal* max, RwReal* diagonal)
{
    RwV3d sup = {RwRealMINVAL, RwRealMINVAL, RwRealMINVAL};
    RwV3d inf = {RwRealMAXVAL, RwRealMAXVAL, RwRealMAXVAL};
    RwInt32 i;

    RWFUNCTION(RWSTRING("_rtImportWorldGetDimensions"));

    for (i = 0; i < importWorld->numVertices; i++)
    {
        if (importWorld->vertices[i].OC.y > sup.y)
        {
            sup.y = importWorld->vertices[i].OC.y;
        }
        if (importWorld->vertices[i].OC.y < inf.y)
        {
            inf.y = importWorld->vertices[i].OC.y;
        }

        if (importWorld->vertices[i].OC.x > sup.x)
        {
            sup.x = importWorld->vertices[i].OC.x;
        }
        if (importWorld->vertices[i].OC.x < inf.x)
        {
            inf.x = importWorld->vertices[i].OC.x;
        }

        if (importWorld->vertices[i].OC.z > sup.z)
        {
            sup.z = importWorld->vertices[i].OC.z;
        }
        if (importWorld->vertices[i].OC.z < inf.z)
        {
            inf.z = importWorld->vertices[i].OC.z;
        }
    }

    *min = inf.y;
    *max = sup.y;
    rwSqrt(diagonal, (((sup.x - inf.x) * (sup.x - inf.x)) +
        ((sup.y - inf.y) * (sup.y - inf.y)) +
        ((sup.z - inf.z) * (sup.z - inf.z))));

    RWRETURNVOID();
}

/****************************************************************************
 API Functions
 */

/***************************************************************************
                           Userdata CallBacks
 ***************************************************************************
 ***************************************************************************/

/* *INDENT-OFF* */

/**
 * \ingroup rtworldimport
 * \ref RtWorldImportSetUserdataCallBacks is used to define a set of
 * callback functions that are called from \ref RtWorldImportCreateWorld in
 * order to allow userdata to be passed from an import world to an RpWorld.
 *
 * No callback functions are defined until this function is called.
 *
 * The world plugin must be attached before using this function.
 * The include file rtimport.h and the library file rtimport.lib are
 * also required.
 *
 * \param destroyVertexUserdataCB Pointers to the user data callback functions.
 * \param cloneVertexUserdataCB Pointer to the callback function for cloning
 * \param interpVertexUserdataCB Pointer to the callback function for interpolating
 * \param sectorSetVertexUserdataCB Pointer to the callback function for vertices on sector creation
 * \param destroyPolygonUserdataCB Pointer to the callback function for polygon destruction
 * \param splitPolygonUserdataCB Pointer to the callback function for polygon splitting
 * \param sectorSetPolygonUserdataCB Pointer to the callback function for polygons on sector creation
 *
 * \see RtWorldImportDestroyVertexUserdataCallBack
 * \see RtWorldImportCloneVertexUserdataCallBack
 * \see RtWorldImportInterpVertexUserdataCallBack
 * \see RtWorldImportDestroyPolygonUserdataCallBack
 * \see RtWorldImportSplitPolygonUserdataCallBack
 * \see RtWorldImportCreateWorld
 * \see RpWorldPluginAttach
 */
void
RtWorldImportSetUserdataCallBacks(RtWorldImportDestroyVertexUserdataCallBack destroyVertexUserdataCB,
                                  RtWorldImportCloneVertexUserdataCallBack cloneVertexUserdataCB,
                                  RtWorldImportInterpVertexUserdataCallBack interpVertexUserdataCB,
                                  RtWorldImportSectorSetVertexUserdataCallBack sectorSetVertexUserdataCB,
                                  RtWorldImportDestroyPolygonUserdataCallBack destroyPolygonUserdataCB,
                                  RtWorldImportSplitPolygonUserdataCallBack splitPolygonUserdataCB,
                                  RtWorldImportSectorSetPolygonUserdataCallBack sectorSetPolygonUserdataCB)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportSetUserdataCallBacks"));

    UserDataCallBacks.destroyVertexUserdata =
        destroyVertexUserdataCB;
    UserDataCallBacks.cloneVertexUserdata =
        cloneVertexUserdataCB;
    UserDataCallBacks.interpVertexUserdata =
        interpVertexUserdataCB;
    UserDataCallBacks.sectorSetVertexUserdata =
        sectorSetVertexUserdataCB;
    UserDataCallBacks.destroyPolygonUserdata =
        destroyPolygonUserdataCB;
    UserDataCallBacks.splitPolygonUserdata =
        splitPolygonUserdataCB;
    UserDataCallBacks.sectorSetPolygonUserdata =
        sectorSetPolygonUserdataCB;

    RWRETURNVOID();
}

/* *INDENT-ON* */

/***************************************************************************
 ***************************************************************************
                           Progress Info
 ***************************************************************************
 ***************************************************************************/

/**
 * \ingroup rtworldimport
 * \ref RtWorldImportSetProgressCallBack is used to define a callback
 * function that is called from \ref RtWorldImportCreateWorld at strategic
 * points to indicate the progress made in constructing a BSP world from
 * an import world.
 *
 * The format of the callback function is:
   \verbatim

  void (*RtWorldImportProgressCallBack) (RwInt32 message, RwReal value);
  \endverbatim
 * where message is one of the following,
 *      \li rtWORLDIMPORTPROGRESSBSPBUILDSTART The BSP creation process
 *           is about to start.
 *           The argument value is equal to 0.0.
 *      \li rtWORLDIMPORTPROGRESSBSPBUILDUPDATE The BSP creation process
 *           has finished processing a subsection of the world.
 *           The argument value is equal to the percentage of the
 *           world processed up to this point.
 *      \li rtWORLDIMPORTPROGRESSBSPBUILDEND The BSP creation process
 *           has ended. The argument value is equal to 100.0.
 *      \li rtWORLDIMPORTPROGRESSBSPCOMPRESSSTART The BSP compression process
 *           is about to start. The argument value is equal to 0.0.
 *      \li rtWORLDIMPORTPROGRESSBSPCOMPRESSUPDATE The BSP compression
 *            has finished processing a subsection of the world.
 *            The argument value is equal to the percentage of the world
 *            processed up to this point.
 *      \li rtWORLDIMPORTPROGRESSBSPCOMPRESSEND The BSP compression process
 *            has ended. The argument value is equal to 100.0.
 *
 * No callback function is defined until this function is called.
 *
 * The world plugin must be attached before using this function.
 * The include file rtimport.h and the library file rtimport.lib are
 * also required.
 *
 * \param CB  A pointer to the progress callback function of type.
 *
 * \see RtWorldImportProgressCallBack
 * \see RtWorldImportCreateWorld
 * \see RpWorldPluginAttach
 */
void
RtWorldImportSetProgressCallBack(RtWorldImportProgressCallBack CB)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportSetProgressCallBack"));
    progressCallBack = CB;
    RWRETURNVOID();
}

/**
 * \ingroup rtworldimport
 * \ref RtWorldImportSetBuildCallBacks should be called to set the required or
 * custom \ref RtWorldImportPartitionBuildCallBack and
 * \ref RtWorldImportTerminationBuildCallBack
 *
 * \param partitionBuildCB      The \ref RtWorldImportPartitionBuildCallBack
 * \param terminationBuildCB    The \ref RtWorldImportTerminationBuildCallBack
 *
 * \see RtWorldImportSetBuildCallBacksUserData
 * \see RtWorldImportSetStandardBuildPartitionSelector
 */
void
RtWorldImportSetBuildCallBacks(RtWorldImportPartitionBuildCallBack
                                partitionBuildCB,
                               RtWorldImportTerminationBuildCallBack
                                terminationBuildCB)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportSetBuildCallBacks"));

    BuildCallBacks.partitionBuild = partitionBuildCB;
    BuildCallBacks.terminationBuild = terminationBuildCB;

    RWRETURNVOID();
}

/**
 * \ingroup rtworldimport
 * \ref RtWorldImportSetBuildCallBacksUserData should be called to set the
 * user data that is required by the \ref RtWorldImportPartitionBuildCallBack and
 * \ref RtWorldImportTerminationBuildCallBack
 *
 * \param partitionUserData     The user data required for the
 *                              \ref RtWorldImportPartitionBuildCallBack
 * \param terminateUserData     The user data required for the
 *                              \ref RtWorldImportTerminationBuildCallBack
 *
 * \see RtWorldImportSetBuildCallBacks
 * \see RtWorldImportSetStandardBuildPartitionSelector
 */
void
RtWorldImportSetBuildCallBacksUserData(void *partitionUserData,
                                       void *terminateUserData)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportSetBuildCallBacksUserData"));

    BuildCallBacks.partitionUserData = partitionUserData;
    BuildCallBacks.terminationUserData = terminateUserData;

    RWRETURNVOID();
}



/***************************************************************************
 ***************************************************************************
                         Reading and Writing
 ***************************************************************************
 ***************************************************************************/

/**
 * \ingroup rtworldimport
 * \ref RtWorldImportWrite is used to save a binary representation of the
 * specified import world to the given disk file.
 *
 * The world plugin must be attached before using this function. The include
 * file rtimport.h and the library file rtimport.lib are also required.
 *
 * \param world  A pointer to the import world.
 * \param filename  Pointer to a string containing the name of the input file.
 *
 * \return Returns a pointer to the import world if successful
 *          or NULL if there is an error.
 *
 * \see RtWorldImportWrite
 * \see RpWorldPluginAttach
 */
RtWorldImport      *
RtWorldImportWrite(RtWorldImport * world, RwChar * filename)
{
    RwStream           *worldStream;

    RWAPIFUNCTION(RWSTRING("RtWorldImportWrite"));

    worldStream = RwStreamOpen(rwSTREAMFILENAME,
                               rwSTREAMWRITE, filename);

    if (!worldStream)
    {
        world = (RtWorldImport *) NULL;
    }
    else
    {
        RwStreamWriteInt(worldStream, &world->numPolygons,
                         sizeof(RwInt32));
        RwStreamWriteInt(worldStream, &world->numVertices,
                         sizeof(RwInt32));
        RwStreamWrite(worldStream, world->polygons,
                      sizeof(RtWorldImportTriangle) *
                      world->numPolygons);
        RwStreamWrite(worldStream, world->vertices,
                      sizeof(RtWorldImportVertex) * world->numVertices);
        rpMaterialListStreamWrite(&world->matList, worldStream);

        RwStreamClose(worldStream, NULL);
    }

    RWRETURN(world);
}
/**
 * \ingroup kd
 * \ref RtWorldImportGuideKDWrite Saves the given \ref RtWorldImportGuideKDTree
 *
 * \param guideKD   A pointer to the \ref RtWorldImportGuideKDTree.
 * \param filename  A pointer to the filename where it is to be stored.
 *
 * \return          The \ref RtWorldImportGuideKDTree.
 *
 * \see RtWorldImportGuideKDRead
 * \see RtWorldImportGuideKDDestroy
 * \see RtWorldImportGuideKDDeletePartition
 * \see RtWorldImportGuideKDAddPartition
 * \see RtWorldImportGuideKDCreate
 */
RtWorldImportGuideKDTree *
RtWorldImportGuideKDWrite(RtWorldImportGuideKDTree *guideKD, const RwChar *filename)
{
    RwStream *KDStream;
    _rtWorldImportGuideKDStack KDStack = {NULL, NULL, NULL};

    RWAPIFUNCTION(RWSTRING("RtWorldImportGuideKDWrite"));
    RWASSERT(guideKD);

    KDStream = RwStreamOpen(rwSTREAMFILENAME,
                               rwSTREAMWRITE, filename);

    if (!KDStream)
    {
        guideKD = (RtWorldImportGuideKDTree *) NULL;
    }
    else
    {
        KDStack.bottom = KDStack.top = NULL;
        _rtWorldImportGuideKDEncodeAsStack(guideKD, &KDStack);
        KDStack.current = KDStack.top;

        /* save all KD stack elements */
        while(KDStack.current)
        {
            RwStreamWrite(KDStream, &(KDStack.current->node->partition.type), sizeof(RwInt32));
            RwStreamWrite(KDStream, &(KDStack.current->node->partition.value), sizeof(RwReal));

            KDStack.current  = KDStack.current->next;
        };

        RwStreamClose(KDStream, NULL);
    }

    RWRETURN(guideKD);
}


static void
DivideBox(RwBBox * box, RwReal maxLeftValue, RwReal maxRightValue,
             RwBBox * lbox, RwBBox * rbox,
             RwInt32 align)
{
    RWFUNCTION(RWSTRING("DivideBox"));

    /* First make boxes the same as the root */
    lbox->inf = box->inf;
    lbox->sup = box->sup;
    rbox->inf = box->inf;
    rbox->sup = box->sup;

    /* Now adjust the relevant coordinate */
    SETCOORD(lbox->sup, align, maxLeftValue);
    SETCOORD(rbox->inf, align, maxRightValue);

    RWRETURNVOID();
}

static void
BuildGuideKDNodeFromStream(RwStream *KDStream, RtWorldImportGuideKDTree *parent, const RwBBox *leftBBox, const RwBBox *rightBBox)
{
    RtWorldImportGuideKDTree *leftKD, *rightKD;
    RwBBox childLeftBBox, childRightBBox;

    RWFUNCTION(RWSTRING("BuildGuideKDNodeFromStream"));

    leftKD = parent->left = (RtWorldImportGuideKDTree*)RwMalloc(sizeof(RtWorldImportGuideKDTree),
                                                     rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
    rightKD = parent->right = (RtWorldImportGuideKDTree*)RwMalloc(sizeof(RtWorldImportGuideKDTree),
                                                       rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);

    RwStreamRead(KDStream, &(leftKD->partition.type), sizeof(RwInt32));
    RwStreamRead(KDStream, &(leftKD->partition.value), sizeof(RwReal));

    leftKD->partition.maxRightValue = leftKD->partition.value;
    leftKD->partition.maxLeftValue = leftKD->partition.value;
    leftKD->left = NULL;
    leftKD->right = NULL;
    leftKD->order = 0;
    leftKD->bbox = *leftBBox;
    leftKD->parent = parent;

    /* validate partition value against the bounding box */
    if( (leftKD->partition.value > GETCOORD(leftKD->bbox.sup, leftKD->partition.type)) ||
        (leftKD->partition.value < GETCOORD(leftKD->bbox.inf, leftKD->partition.type)) )
    {
        /* Clamp Invalid */
        leftKD->partition.value = ( GETCOORD(leftKD->bbox.sup, leftKD->partition.type) +
                                    GETCOORD(leftKD->bbox.inf, leftKD->partition.type) ) / 2;
    }

    if(leftKD->partition.type >= 0)
    {
        /* calculate child bounding boxes */
        DivideBox(&leftKD->bbox, leftKD->partition.value, leftKD->partition.value,
         &childLeftBBox, &childRightBBox, leftKD->partition.type);

        BuildGuideKDNodeFromStream(KDStream, leftKD, &childLeftBBox, &childRightBBox);
    }

    RwStreamRead(KDStream, &(rightKD->partition.type), sizeof(RwInt32));
    RwStreamRead(KDStream, &(rightKD->partition.value), sizeof(RwReal));

    rightKD->partition.maxRightValue = rightKD->partition.value;
    rightKD->partition.maxLeftValue = rightKD->partition.value;
    rightKD->left = NULL;
    rightKD->right = NULL;
    rightKD->bbox = *rightBBox;
    rightKD->order = 1;
    rightKD->parent = parent;

    /* validate partition value against the bounding box */
    if( (rightKD->partition.value > GETCOORD(rightKD->bbox.sup, rightKD->partition.type)) ||
        (rightKD->partition.value < GETCOORD(rightKD->bbox.inf, rightKD->partition.type)) )
    {
        /* Clamp Invalid */
        rightKD->partition.value = ( GETCOORD(rightKD->bbox.sup, rightKD->partition.type) +
                                    GETCOORD(rightKD->bbox.inf, rightKD->partition.type) ) / 2;
    }

    if(rightKD->partition.type >= 0)
    {
        /* calculate child bounding boxes */
        DivideBox(&rightKD->bbox, rightKD->partition.value, rightKD->partition.value,
         &childLeftBBox, &childRightBBox, rightKD->partition.type);

        BuildGuideKDNodeFromStream(KDStream, rightKD, &childLeftBBox, &childRightBBox);
    }
    RWRETURNVOID();
}

/**
 * \ingroup kd
 * \ref RtWorldImportGuideKDRead Reads in a previously saved KD tree and stores it
 * in a \ref RtWorldImportGuideKDTree. This can subsequently be applied to a world using
 * a guided build.
 *
 * \param filename  A pointer to the filename where the KD tree is stored
 * \param bbox      The bounding box of the world
 *
 * \return          A pointer to the \ref RtWorldImportGuideKDTree
 *
 * \see RtWorldImportGuideKDRead
 * \see RtWorldImportGuideKDDestroy
 * \see RtWorldImportGuideKDDeletePartition
 * \see RtWorldImportGuideKDAddPartition
 * \see RtWorldImportGuideKDCreate
 */
RtWorldImportGuideKDTree *
RtWorldImportGuideKDRead(const RwChar *filename, const RwBBox *bbox)
{
    RtWorldImportGuideKDTree *guideKD;
    RwStream *KDStream;
    RwBBox leftBBox, rightBBox;

    RWAPIFUNCTION(RWSTRING("RtWorldImportGuideKDRead"));
    RWASSERT(bbox);
    RWASSERT(filename);
    RWASSERT(*filename != '\0');

    KDStream =
        RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, filename);

    if (!KDStream)
    {
        guideKD = (RtWorldImportGuideKDTree *) NULL;
    }
    else
    {
        guideKD = (RtWorldImportGuideKDTree*)RwMalloc(sizeof(RtWorldImportGuideKDTree),
                                             rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);

        RwStreamRead(KDStream, &(guideKD->partition.type), sizeof(RwInt32));
        RwStreamRead(KDStream, &(guideKD->partition.value), sizeof(RwReal));

        guideKD->partition.maxRightValue = guideKD->partition.value;
        guideKD->partition.maxLeftValue = guideKD->partition.value;
        guideKD->left = NULL;
        guideKD->right = NULL;
        guideKD->order = 0;
        guideKD->bbox = *bbox;
        guideKD->parent = NULL;

        /* validate partition value against the bounding box */
        if( (guideKD->partition.value > GETCOORD(guideKD->bbox.sup, guideKD->partition.type)) ||
            (guideKD->partition.value < GETCOORD(guideKD->bbox.inf, guideKD->partition.type)) )
        {
            /* Clamp Invalid */
            guideKD->partition.value = ( GETCOORD(guideKD->bbox.sup, guideKD->partition.type) +
                                        GETCOORD(guideKD->bbox.inf, guideKD->partition.type) ) / 2;
        }

        if(guideKD->partition.type >= 0)
        {
            /* calculate child bounding boxes */
            DivideBox(&guideKD->bbox, guideKD->partition.value, guideKD->partition.value,
             &leftBBox, &rightBBox, guideKD->partition.type);

            BuildGuideKDNodeFromStream(KDStream, guideKD, &leftBBox, &rightBBox);
        }

        RwStreamClose(KDStream, NULL);
    }

    RWRETURN(guideKD);
}

/**
 * \ingroup rtworldimport
 * \ref RtWorldImportRead is used to load a binary representation of the
 * an import world from the given disk file.
 *
 * The world plugin must be attached before using this function. The include
 * file rtimport.h and the library file rtimport.lib are also required.
 *
 * \param filename  Pointer to a string containing the name of the input file.
 *
 * \return Returns a pointer to the import world if successful or NULL if there
 * is an error.
 *
 * \see RtWorldImportWrite
 * \see RpWorldPluginAttach
 */
RtWorldImport      *
RtWorldImportRead(RwChar * filename)
{
    RwStream           *worldStream;
    RtWorldImport      *world;
    RwInt32             i;

    RWAPIFUNCTION(RWSTRING("RtWorldImportRead"));

    worldStream =
        RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, filename);
    if (worldStream)
    {
        world = (RtWorldImport *) RwMalloc(sizeof(RtWorldImport),
                       rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
        RwStreamReadInt(worldStream, &world->numPolygons,
                        sizeof(RwInt32));
        RwStreamReadInt(worldStream, &world->numVertices,
                        sizeof(RwInt32));
        world->polygons = (RtWorldImportTriangle *)
            RwMalloc(sizeof(RtWorldImportTriangle) *
                     world->numPolygons, rwID_NOHSWORLDPLUGIN |
                     rwMEMHINTDUR_EVENT | rwMEMHINTFLAG_RESIZABLE);
        if (!world->polygons)
        {
            RWRETURN((RtWorldImport *) NULL);
        }
        RwStreamRead(worldStream, world->polygons,
                     sizeof(RtWorldImportTriangle) *
                     world->numPolygons);
        world->vertices = (RtWorldImportVertex *)
            RwMalloc(sizeof(RtWorldImportVertex) * world->numVertices,
                     rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT |
                     rwMEMHINTFLAG_RESIZABLE);
        if (!world->vertices)
        {
            RWRETURN((RtWorldImport *) NULL);
        }
        RwStreamRead(worldStream, world->vertices,
                     sizeof(RtWorldImportVertex) * world->numVertices);
        /* Make all the userdata pointers NULL since we don't support
         * streaming of userdata */
        for (i = 0; i < world->numPolygons; i++)
        {
            world->polygons[i].pUserdata = NULL;
        }
        for (i = 0; i < world->numVertices; i++)
        {
            world->vertices[i].pUserdata = NULL;
        }
        if(!RwStreamFindChunk(worldStream, rwID_MATLIST,
                          (RwUInt32 *) NULL, (RwUInt32 *) NULL))
        {
            RwStreamClose(worldStream, NULL);
            RWRETURN((RtWorldImport *) NULL);
        }
        rpMaterialListStreamRead(worldStream, &world->matList);
        RwStreamClose(worldStream, NULL);
        RWRETURN(world);
    }
    RWRETURN((RtWorldImport *) NULL);
}

/***************************************************************************
 ***************************************************************************
                                Initializing
 ***************************************************************************
 ***************************************************************************/



/**
 * \ingroup rtworldimport
 * \ref RtWorldImportParametersInit is used to initialize the
 * specified world convert structure \ref RtWorldImportParameters with default
 * values. The world convert
 * structure is used by \ref RtWorldImportCreateWorld to produce a world
 * containing static geometry in the form of a compressed BSP tree. It
 * controls the way the new world is generated.
 *
 * The following default values are set:
 *      \li worldSectorMaxSize = RwRealMAXVAL
 *      \li maxWorldSectorPolygons = 1024
 *      \li calcNormals = TRUE
 *      \li weldThreshold = (\ref RwReal)0.001
 *      \li angularThreshold = (\ref RwReal)0.8660254
 *      \li maxOverlapPercent = (\ref RwReal)0.25
 *      \li conditionGeometry = TRUE
 *      \li userSpecifiedBBox = FALSE
 *      \li uvLimit = (\ref RwReal)16.0
 *      \li retainCreases = FALSE
 *      \li fixTJunctions = TRUE
 *      \li weldPolygons = TRUE
 *      \li flags: \ref rpWORLDTEXTURED \ref rpWORLDPRELIT \ref rpWORLDNORMALS \ref rpWORLDLIGHT
 *      \li mode = \ref rwTEXTUREADDRESSWRAP
 *      \li sortPolygons = FALSE
 *      \li cullZeroAreaPolygons = FALSE
 *      \li numTexCoordSets = 0  (ie use flags to determine this)
 *      \li terminatorCheck = TRUE
 *
 * The world plugin must be attached before using this function. The include
 * file rtimport.h and the library file rtimport.lib are also required.
 *
 * \return WorldImportParams pointer to the world convert structure.
 *
 * \see RtWorldImportParameters
 * \see RtWorldImportParametersSet
 * \see RtWorldImportParametersGet
 * \see RtWorldImportCreate
 * \see RpWorldPluginAttach
 */

void
RtWorldImportParametersInit(RtWorldImportParameters* params)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportParametersInit"));

    params->worldSectorMaxSize = RwRealMAXVAL;
    params->maxWorldSectorPolygons = 1024;
    params->maxOverlapPercent = (RwReal)0.25;
    params->calcNormals = TRUE;

    params->conditionGeometry = TRUE;
    params->userSpecifiedBBox = FALSE;
    params->fixTJunctions = TRUE;
    params->flags = (rpWORLDTEXTURED |
                               rpWORLDPRELIT   |
                               rpWORLDNORMALS  |
                               rpWORLDLIGHT);
    params->numTexCoordSets = 0;
    params->terminatorCheck = TRUE;
    /* Setting numTexCoordSets param to zero means the flags are used
     * to determine this setting. */

#if !defined(_XBOX)
#if (defined(_MSC_VER))
#if (_MSC_VER>=1000)
    {
        RwInt32             sortPolygons = 0;
        RwInt32             Congruence_Off = 0;

        RWGETWINREGDWORD(Congruence_Off, _T("Congruence_Off"));
        RWMONITOR(("%d == Congruence_Off", Congruence_Off));
    }
#endif /*  (_MSC_VER>=1000) */
#endif /* (defined(_MSC_VER)) */
#endif /* !defined(_XBOX) */

    RWRETURNVOID();
}

/**
 * \ingroup rtworldimport
 * \ref RtWorldImportParametersSet is used to set the
 * specified world convert structure \ref RtWorldImportParameters with
 * specified values.
 *
 * \param params A pointer to the parameters
 *
 * \see RtWorldImportParameters
 * \see RtWorldImportParametersInit
 * \see RtWorldImportParametersGet
 */
void
RtWorldImportParametersSet(RtWorldImportParameters* params)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportParametersSet"));

    RWASSERT(params->maxWorldSectorPolygons>0);
    RWASSERT(params->worldSectorMaxSize>0);

    WorldImportParams = params;

    RWRETURNVOID();
}

/**
 * \ingroup rtworldimport
 * \ref RtWorldImportParametersGet is used to get the
 * specified world convert structure \ref RtWorldImportParameters with
 * its given values.
 *
 * \return  A pointer to the parameters
 *
 * \see RtWorldImportParameters
 * \see RtWorldImportParametersInit
 * \see RtWorldImportParametersSet
 */
RtWorldImportParameters*
RtWorldImportParametersGet()
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportParametersGet"));

    RWRETURN(WorldImportParams);
}

/*  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * General utilities */

/**
 * \ingroup rtworldimport
 * \ref RtWorldImportAddNumVertices is used to allocate memory in the
 * specified import world for the given number of additional vertices.
 *
 * \note This function initializes the new vertex array elements to zero
 * (of type \ref RtWorldImportVertex) and the effect of this function is
 * cumulative.
 *
 * The world plugin must be attached before using this function. The include
 * file rtimport.h and the library file rtimport.lib are also required.
 *
 * \param nohsworld  A pointer to the import world.
 * \param numNewVertices  The number of vertices to add.
 *
 * \return Returns a pointer to the import world if successful or NULL if
 * there is an error.

 * \see RtWorldImportAddNumTriangles
 * \see RtWorldImportAddMaterial
 * \see RtWorldImportGetTriangles
 * \see RtWorldImportGetVertices
 * \see RtWorldImportCreate
 * \see RtWorldImportCreateWorld
 * \see RpWorldPluginAttach
 */

RtWorldImport      *
RtWorldImportAddNumVertices(RtWorldImport * nohsworld,
                            RwInt32 numNewVertices)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportAddNumVertices"));

    if (!nohsworld)
    {
        RWERROR((E_RW_NULLP));
    }
    else if (0 < numNewVertices)
    {
        const RwInt32       numVerts =
            nohsworld->numVertices + numNewVertices;
        const RwUInt32      bytes =
            sizeof(RtWorldImportVertex) * numVerts;
        RwInt32             i;
        RtWorldImportVertex *vertices = nohsworld->vertices;

        if (!vertices)
        {
            /* for initial memory allocation use RwMalloc() */
            vertices = (RtWorldImportVertex *) RwMalloc(bytes,
                    rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT |
                    rwMEMHINTFLAG_RESIZABLE);
            if (vertices)
            {
                /* initialize the memory */
                memset(vertices, 0, bytes);
            }
        }
        else
        {
            vertices =
                (RtWorldImportVertex *) RwRealloc(vertices, bytes,
                       rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT |
                       rwMEMHINTFLAG_RESIZABLE);
            if (vertices)
            {
                /* initialize the memory */
                memset(vertices + nohsworld->numVertices, 0,
                    sizeof(RtWorldImportVertex) * numNewVertices);
            }
        }

        if (!vertices)
        {
            nohsworld->numVertices = 0;
            nohsworld->vertices = (RtWorldImportVertex *) NULL;

            RWRETURN((RtWorldImport *) NULL);
        }

        /* Make the new vertex userdata pointers NULL */
        for (i = nohsworld->numVertices; i < numVerts; i++)
        {
            vertices[i].pUserdata = NULL;
        }

        nohsworld->numVertices = numVerts;
        nohsworld->vertices = vertices;

    }

    RWRETURN(nohsworld);
}

/**
 * \ingroup rtworldimport
 * \ref RtWorldImportAddNumTriangles is used to allocate memory in the
 * specified import world for the given number of additional triangles.
 *
 * \note This function initializes the new vertex array elements to zero
 * (of type \ref RtWorldImportTriangle) and the effect of this function is
 * cumulative.
 *
 * The world plugin must be attached before using this function. The include
 * file rtimport.h and the library file rtimport.lib are also required.
 *
 * \param nohsworld  A pointer to the import world.
 * \param numNewTriangles  The number of triangles to add.
 *
 * \return Returns a pointer to the import world if successful or NULL if
 * there is an error.
 *
 * \see RtWorldImportAddNumVertices
 * \see RtWorldImportAddMaterial
 * \see RtWorldImportGetTriangles
 * \see RtWorldImportGetVertices
 * \see RtWorldImportCreate
 * \see RtWorldImportCreateWorld
 * \see RpWorldPluginAttach
 */
RtWorldImport      *
RtWorldImportAddNumTriangles(RtWorldImport * nohsworld,
                             RwInt32 numNewTriangles)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportAddNumTriangles"));
    RWASSERT(nohsworld);

    if (!nohsworld)
    {

        RWERROR((E_RW_NULLP));
    }
    else if (0 < numNewTriangles)
    {
        const RwInt32       numFaces =
            nohsworld->numPolygons + numNewTriangles;
        const RwUInt32      bytes =
            sizeof(RtWorldImportTriangle) * numFaces;
        RwInt32             i;
        RtWorldImportTriangle *triangles = nohsworld->polygons;

        if (!triangles)
        {
            /* for initial memory allocation use RwMalloc() */
            triangles = (RtWorldImportTriangle *) RwMalloc(bytes,
                       rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT |
                       rwMEMHINTFLAG_RESIZABLE);
            if (triangles)
            {
                /* initialize the memory */
                memset(triangles, 0, bytes);
            }
        }
        else
        {
            triangles =
                (RtWorldImportTriangle *) RwRealloc(triangles, bytes,
                            rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT |
                            rwMEMHINTFLAG_RESIZABLE);
            if (triangles)
            {
                /* initialize the memory */
                memset(triangles + nohsworld->numPolygons, 0,
                    sizeof(RtWorldImportTriangle) * numNewTriangles);
            }
        }

        if (!triangles)
        {
            nohsworld->numPolygons = 0;
            nohsworld->polygons = (RtWorldImportTriangle *) NULL;

            RWRETURN((RtWorldImport *) NULL);
        }

        /* Make the new polygon userdata pointers NULL */
        for (i = nohsworld->numPolygons; i < numFaces; i++)
        {
            triangles[i].pUserdata = NULL;
        }

        nohsworld->numPolygons = numFaces;
        nohsworld->polygons = triangles;

    }

    RWRETURN(nohsworld);
}

/**
 * \ingroup rtworldimport
 * \ref RtWorldImportGetMaterial is used to retrieve a material from the
 * specified import world with the given index into the import world’s
 * material list.
 *
 * The world plugin must be attached before using this function. The include
 * file rtimport.h and the library file rtimport.lib are also required.
 *
 * \param nohsworld  A pointer to the import world.
 * \param matInd Material index.
 *
 * \return Returns a pointer to the material if successful or NULL if there is
 * an error.
 *
 * \see RtWorldImportGetMaterialIndex
 * \see RtWorldImportGetVertices
 * \see RtWorldImportGetTriangles
 * \see RtWorldImportGetNumVertices
 * \see RtWorldImportGetNumTriangles
 * \see RpWorldPluginAttach
 */
RpMaterial         *
RtWorldImportGetMaterial(RtWorldImport * nohsworld, RwInt32 matInd)
{
    RpMaterial         *result;

    RWAPIFUNCTION(RWSTRING("RtWorldImportGetMaterial"));
    RWASSERT(nohsworld);

    result = RtWorldImportGetMaterialMacro(nohsworld, matInd);

    RWRETURN(result);
}

/**
 * \ingroup rtworldimport
 * \ref RtWorldImportGetNumVertices is used to retrieve the number of
 * vertices defining the specified import world.
 *
 * The world plugin must be attached before using this function. The include
 * file rtimport.h and the library file rtimport.lib are also required.
 *
 * \param nohsworld  A pointer to the import world.
 *
 * \return Returns an \ref RwInt32 value equal to the number of vertices if
 * successful or -1 if there is an error.
 *
 * \see RtWorldImportAddNumTriangles
 * \see RtWorldImportAddNumVertices
 * \see RtWorldImportGetVertices
 * \see RtWorldImportGetTriangles
 * \see RtWorldImportGetMaterial
 * \see RtWorldImportGetNumTriangles
 * \see RpWorldPluginAttach
 */

RwInt32
RtWorldImportGetNumVertices(RtWorldImport * nohsworld)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportGetNumVertices"));
    RWASSERT(nohsworld);

    if (nohsworld)
    {
        RWRETURN(nohsworld->numVertices);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN(-1);
}

/**
 * \ingroup rtworldimport
 * \ref RtWorldImportGetVertices is used to retrieve the array of
 * vertices of type \ref RtWorldImportVertex from the specified import world.
 * Use this function for directly defining the properties of each vertex.
 *
 * The world plugin must be attached before using this function. The include
 * file rtimport.h and the library file rtimport.lib are also required.
 *
 * \param nohsworld  A pointer to the import world.
 *
 * \return Returns an array of \ref RtWorldImportVertex values if successful or
 * NULL if there is an error.
 *
 * \see RtWorldImportAddNumTriangles
 * \see RtWorldImportAddNumVertices
 * \see RtWorldImportGetMaterial
 * \see RtWorldImportGetTriangles
 * \see RtWorldImportGetNumVertices
 * \see RtWorldImportGetNumTriangles
 * \see RpWorldPluginAttach
 */
RtWorldImportVertex *
RtWorldImportGetVertices(RtWorldImport * nohsworld)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportGetVertices"));
    RWASSERT(nohsworld);

    if (nohsworld)
    {
        RWRETURN(nohsworld->vertices);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN((RtWorldImportVertex *) NULL);
}

/**
 * \ingroup rtworldimport
 * \ref RtWorldImportGetNumTriangles is used to retrieve the number of
 * triangles defining the specified import world.
 *
 * The world plugin must be attached before using this function. The include
 * file rtimport.h and the library file rtimport.lib are also required.
 *
 * \param nohsworld  A pointer to the import world.
 *
 * \return Returns an \ref RwInt32 value equal to the number of triangles if
 * successful or -1 if there is an error.
 *
 * \see RtWorldImportAddNumTriangles
 * \see RtWorldImportAddNumVertices
 * \see RtWorldImportGetVertices
 * \see RtWorldImportGetTriangles
 * \see RtWorldImportGetNumVertices
 * \see RtWorldImportGetMaterial
 * \see RpWorldPluginAttach
 */
RwInt32
RtWorldImportGetNumTriangles(RtWorldImport * nohsworld)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportGetNumTriangles"));
    RWASSERT(nohsworld);

    if (nohsworld)
    {
        RWRETURN(nohsworld->numPolygons);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN(-1);
}

/**
 * \ingroup rtworldimport
 * \ref RtWorldImportGetTriangles is used to retrieve the array of
 * triangles (of type \ref RtWorldImportTriangle) from the specified import
 * world. Use this function for directly defining the properties of each
 * triangle.
 *
 * The world plugin must be attached before using this function. The include
 * file rtimport.h and the library file rtimport.lib are also required.
 *
 * \param nohsworld  A pointer to the import world.
 *
 * \return Returns an array of \ref RtWorldImportTriangle values if successful
 * or NULL if there is an error.
 *
 * \see RtWorldImportAddNumTriangles
 * \see RtWorldImportAddNumVertices
 * \see RtWorldImportGetVertices
 * \see RtWorldImportGetMaterial
 * \see RtWorldImportGetNumVertices
 * \see RtWorldImportGetNumTriangles
 * \see RpWorldPluginAttach
 */

RtWorldImportTriangle *
RtWorldImportGetTriangles(RtWorldImport * nohsworld)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportGetTriangles"));
    RWASSERT(nohsworld);

    if (nohsworld)
    {
        RWRETURN(nohsworld->polygons);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN((RtWorldImportTriangle *) NULL);
}

/**
 * \ingroup rtworldimport
 * \ref RtWorldImportAddMaterial is used to register the given material
 * with the specified import world. Once registered, this material can be
 * associated with any triangle constituting the import world via the returned
 * material index. All triangles must have a material before the import world
 * is converted to a BSP world.
 *
 * The world plugin must be attached before using this function. The include
 * file rtimport.h and the library file rtimport.lib are also required.
 *
 * \param nohsworld  A pointer to the import world.
 * \param material  A pointer to the material.
 *
 * \return Returns the material’s index if successful or -1 if there is an
 * error.
 *
 * \see RtWorldImportAddNumTriangles
 * \see RtWorldImportAddNumVertices
 * \see RtWorldImportGetTriangles
 * \see RtWorldImportGetVertices
 * \see RtWorldImportCreate
 * \see RtWorldImportCreateWorld
 * \see RpWorldPluginAttach
 */
RwInt32
RtWorldImportAddMaterial(RtWorldImport * nohsworld,
                         RpMaterial * material)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportAddMaterial"));
    RWASSERT(nohsworld);
    RWASSERT(material);

    if (nohsworld && material)
    {
        RwInt32             matIndex;

        matIndex =
            rpMaterialListAppendMaterial(&nohsworld->matList, material);
        RWRETURN(matIndex);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN(-1);
}

/**
 * \ingroup rtworldimport
 * \ref RtWorldImportGetMaterialIndex is used to retrieve the index of
 * the given material from the specified import world’s material list.
 *
 * The world plugin must be attached before using this function. The include
 * file rtimport.h and the library file rtimport.lib are also required.
 *
 * \param nohsworld  A pointer to the import world.
 * \param material  A pointer to the material.
 *
 * \return Returns the material’s index if successful or -1 if there
 * is an error.
 *
 * \see RtWorldImportGetMaterial
 * \see RtWorldImportGetVertices
 * \see RtWorldImportGetTriangles
 * \see RtWorldImportGetNumVertices
 * \see RtWorldImportGetNumTriangles
 * \see RpWorldPluginAttach
 */
RwInt32
RtWorldImportGetMaterialIndex(RtWorldImport * nohsworld,
                              RpMaterial * material)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportGetMaterialIndex"));
    RWASSERT(nohsworld);
    RWASSERT(material);

    if (nohsworld && material)
    {
        RwInt32             i;

        /* If this material is in the list return the index else return -1 */
        for (i = 0; i < nohsworld->matList.numMaterials; i++)
        {
            if (material == nohsworld->matList.materials[i])
            {
                /* Found our material */
                RWRETURN(i);
            }
        }

        /* Not in the list */
        RWRETURN(-1);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN(-1);
}

/**
 * \ingroup rtworldimport
 * \ref RtWorldImportForAllMaterials is used to apply the given callback
 * function to all materials registered with the specified import world. The
 * format of the callback function is:
 * \verbatim

   RpMaterial *(*RpMaterialCallBack)(RpMaterial *material, void *data);
   \endverbatim
 * where (void *data) is a user-supplied data pointer to pass to the callback
 * function.
 *
 * The world plugin must be attached before using this function. The include
 * file rtimport.h and the library file rtimport.lib are also required.
 *
 * \note If any invocation of the callback function returns a failure
 * status the iteration is terminated. However,
 * \ref RtWorldImportForAllMaterials will still return successfully.
 *
 * \param nohsworld  A pointer to the import world.
 * \param fpCallBack  A pointer to the callback function
 *                    to apply to each material.
 * \param pData  A pointer to user-supplied data to pass to the callback
 *                    function.
 *
 * \return Returns a pointer to the import world if successful or NULL if
 * there is an error.
 *
 * \see RtWorldImportGetMaterial
 * \see RtWorldImportGetMaterialIndex
 * \see RtWorldImportGetVertices
 * \see RtWorldImportGetTriangles
 * \see RtWorldImportGetNumVertices
 * \see RtWorldImportGetNumTriangles
 * \see RpWorldPluginAttach
 */
RtWorldImport      *
RtWorldImportForAllMaterials(RtWorldImport * nohsworld,
                             RpMaterialCallBack fpCallBack, void *pData)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportForAllMaterials"));
    RWASSERT(nohsworld);
    RWASSERT(fpCallBack);

    if (nohsworld && fpCallBack)
    {
        RwInt32             numMaterials =
            nohsworld->matList.numMaterials;
        RwInt32             i;

        for (i = 0; i < numMaterials; i++)
        {
            RpMaterial         *material;

            material = RtWorldImportGetMaterial(nohsworld, i);

            RWASSERT(material);

            if (!fpCallBack(material, pData))
            {
                /* Early out */
                RWRETURN(nohsworld);
            }
        }

        /* All ok */
        RWRETURN(nohsworld);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN((RtWorldImport *) NULL);
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   Dividing up the world

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

#define _DOT_EPSILON (((RwReal)1)/((RwReal)(1<<10)))


static RwBool
ConvertWorldImportToGeometryList( RtWorldImport *worldImport,
                                        RtGCondGeometryList* geometryList)
{
    RtGCondVertex* v = NULL;
    RtWorldImportVertex* b = NULL;
    RtGCondPolygon* w = NULL;
    RtWorldImportTriangle* c = NULL;
    RwInt32 i;
    RtWorldImportParameters* params = RtWorldImportParametersGet();

    RWFUNCTION(RWSTRING("ConvertWorldImportToGeometryList"));

    geometryList->matList = worldImport->matList;
    geometryList->numUVs = params->numTexCoordSets;

    /* populate matIndex field */
    for (i = 0; i < worldImport->numPolygons; i++)
    {
        c = &worldImport->polygons[i];

        worldImport->vertices[c->vertIndex[0]].matIndex = c->matIndex;
        worldImport->vertices[c->vertIndex[1]].matIndex = c->matIndex;
        worldImport->vertices[c->vertIndex[2]].matIndex = c->matIndex;
    }

    /* Convert vertex list... */
    if (!RtGCondAllocateVertices(geometryList, worldImport->numVertices))
    {
        RWRETURN(FALSE);
    }
    
    v = geometryList->vertices;
    b = worldImport->vertices;

    for (i = 0; i < worldImport->numVertices; i++)
    {
        RwInt32 tx;

        v->index = i;
        v->matIndex = b->matIndex;
        v->normal = b->normal;
        v->position = b->OC;
        v->preLitCol = b->preLitCol;
        v->pUserData = b->pUserdata;
        for (tx = 0; tx < params->numTexCoordSets; tx++)
        {
            v->texCoords[tx] = b->texCoords[tx];
        }

        v++;
        b++;
    }
    geometryList->numVertices = worldImport->numVertices;

    /* Convert polygon list... */
    if (!RtGCondAllocatePolygons(geometryList, worldImport->numPolygons))
    {
        RtGCondFreeVertices(geometryList);
        RWRETURN(FALSE);
    }

    w = geometryList->polygons;
    c = worldImport->polygons;

    for (i = 0; i < worldImport->numPolygons; i++)
    {
        w->id = i;

        RtGCondAllocateIndices(w, 3);
        w->indices[0] = c->vertIndex[0];
        w->indices[1] = c->vertIndex[1];
        w->indices[2] = c->vertIndex[2];
        w->numIndices = 3;

        w->matIndex = c->matIndex;
        w->pUserData = c->pUserdata;

        w++;
        c++;
    }

    geometryList->numPolygons = worldImport->numPolygons;

    RWRETURN(TRUE);
}

static void
ConvertGeometryListToWorldImport( RtGCondGeometryList* geometryList,
                                  RtWorldImport *worldImport,
                                  RpMaterialList *matList)
{
    RtGCondVertex* b = NULL;
    RtWorldImportVertex* v = NULL;
    RtGCondPolygon* c = NULL;
    RtWorldImportTriangle* w = NULL;
    RwInt32 i, r;
    RwInt32 origNumPolys;
    RwInt32 tris;

    RWFUNCTION(RWSTRING("ConvertGeometryListToWorldImport"));

    for (i = 0; i < matList->numMaterials; i++)
    {
        RtWorldImportAddMaterial(worldImport, matList->materials[i]);
    }
    /* Convert vertex list... */
    RtWorldImportAddNumVertices(worldImport, geometryList->numVertices);


    b = geometryList->vertices;
    v = worldImport->vertices;

    for (i = 0; i < geometryList->numVertices; i++)
    {
        RwInt32 tx;

        v->matIndex = b->matIndex;
        v->normal = b->normal;
        v->OC = b->position;
        v->preLitCol = b->preLitCol;
        v->pUserdata = b->pUserData;
        for (tx = 0; tx < rwMAXTEXTURECOORDS; tx++)
        {
            v->texCoords[tx] = b->texCoords[tx];
        }

        v++;
        b++;
    }
    worldImport->numVertices = geometryList->numVertices;




    /* Convert polygon list... */

    /* 1st calculate how many triangles after retriangularisation */
    tris = 0;
    c = geometryList->polygons;
    for (i = 0; i < geometryList->numPolygons; i++)
    {
        if (c->numIndices == 3)
        {
            tris++;
        }
        else
        {
            /* Retriangulate >3s */
            tris += c->numIndices-2;
        }
        c++;
    }




    RtWorldImportAddNumTriangles(worldImport, tris);

    c = geometryList->polygons;
    w = worldImport->polygons;

    origNumPolys = geometryList->numPolygons;
    for (i = 0; i < origNumPolys; i++)
    {
        RWASSERT(c->numIndices >= 3);
        if (c->numIndices == 3)
        {
            w->vertIndex[0] = c->indices[0];
            w->vertIndex[1] = c->indices[1];
            w->vertIndex[2] = c->indices[2];
            w->matIndex = c->matIndex;
            w->pUserdata = c->pUserData;
            w++;
            c++;
        }
        else
        {
            /* Retriangulate >3s */
            RwInt32 l = c->numIndices - 1;
            RwInt32 v1 = 0;
            RwInt32 v2 = 1;
            RwInt32 v3 = 2;

            for (r = 0; r < c->numIndices - 2; r++)
            {
                if (r == 0)
                {
                    v1 = 0;
                    v2 = 1;
                    v3 = 2;
                }
                else if (r % 2 != 0)
                {
                    v2 = v1;
                    v1 = v3;
                    v3 = l--;
                }
                else
                {
                    v2 = v1;
                    v1 = v3;
                    v3 = v2 + 1;
                }

                if (r % 2 == 0)
                {
                    w->vertIndex[0] = c->indices[v1];
                    w->vertIndex[1] = c->indices[v2];
                    w->vertIndex[2] = c->indices[v3];
                }
                else
                {
                    w->vertIndex[2] = c->indices[v1];
                    w->vertIndex[1] = c->indices[v2];
                    w->vertIndex[0] = c->indices[v3];
                }

                w->matIndex = c->matIndex;
                w->pUserdata = c->pUserData;
                w++;
            }

            c++;
        }

    }

    c = geometryList->polygons;
    for (i = 0; i < geometryList->numPolygons; i++)
    {
        RtGCondFreeIndices(c);
        c++;
    }
    RtGCondFreePolygons(geometryList);
    RtGCondFreeVertices(geometryList);

    RWRETURNVOID();
}
/**
 * \ingroup rtworldimport
 * \ref RtWorldImportCreateWorld is used to create a BSP world from the
 * specified import world according to the given conversion parameters.
 *
 * A world uses a BSP mechanism to make collision detection and rendering
 * faster. An import world has no such mechanism and is a large collection of
 * vertices and polygons that describe the world.
 *
 * The world plugin must be attached before using this function. The include
 * file rtimport.h and the library file rtimport.lib are also required.
 *
 * \note All triangles must have an associated material before the import
 * world is converted to a BSP world.
 *
 * \param nohsworld  A pointer to the import world.
 * \param params  A pointer to the world convert structure.
 *
 * \return Returns a pointer to the new world if successful or NULL if there
 * is an error.
 *
 * \see RtWorldImportParametersInit
 * \see RtWorldImportParametersSet
 * \see RtWorldImportParametersGet
 * \see RtWorldImportSetBuildCallBacks
 * \see RtWorldImportSetBuildCallBacksUserData
 * \see RtWorldImportSetStandardBuildPartitionSelector
 * \see RtWorldImportSetProgressCallBack
 * \see RtWorldImportDestroy
 * \see RpWorldPluginAttach
 */

RpWorld            *
RtWorldImportCreateWorld(RtWorldImport *nohsworld,
                         RtWorldImportParameters *params)
{
    RpWorld             *result = (RpWorld *) NULL;
    RwBool              nullterminationsodefault = FALSE;
    RwBool              nullpartitionsodefault = FALSE;
    static RwReal       maxClosestCheck = 20;
    RtWorldImport       *newWorldImport;
    RtGCondGeometryList geometryList;

    RWAPIFUNCTION(RWSTRING("RtWorldImportCreateWorld"));
    RWASSERT(nohsworld);
    RWASSERT(params);
    RWASSERT(params->numTexCoordSets <= rwMAXTEXTURECOORDS);

    /* verify RtWorldImport */
    {
        RwInt32             nI;

        for (nI = 0; nI < nohsworld->numPolygons; nI++)
        {
            RwInt32             nK;

            const RwInt32       matIndex = nohsworld->polygons[nI].matIndex;
            RwInt32            *const vertIndex = nohsworld->polygons[nI].vertIndex;

            if (matIndex < 0)
            {
                RWRETURN((RpWorld *) NULL);
            }
            if (matIndex >= rpMaterialListGetNumMaterials(&nohsworld->matList))
            {
                RWRETURN((RpWorld *) NULL);
            }

            for (nK = 0; nK < 3; nK++)
            {
                if (vertIndex[nK] < 0)
                {
                    RWRETURN((RpWorld *) NULL);
                }
                if (vertIndex[nK] >= nohsworld->numVertices)
                {
                    RWRETURN((RpWorld *) NULL);
                }
            }
        }
    }

    RtWorldImportParametersSet(params);

    /* Multitexture specification consistency */
    if (params->numTexCoordSets == 0)
    {
        /* Set num tex coords from backward compatibility flags */
        params->numTexCoordSets =
            (params->flags & rpWORLDTEXTURED2) ? 2 :
                ((params->flags & rpWORLDTEXTURED) ? 1 : 0);
    }
    else
    {
        /* Make sure backward compatibility flags set appropriately */
        params->flags &= ~(rpWORLDTEXTURED|rpWORLDTEXTURED2);
        params->flags |=
            (params->numTexCoordSets == 1) ? rpWORLDTEXTURED :
                ((params->numTexCoordSets > 1) ? rpWORLDTEXTURED2 : 0);

    }

    /* Do the geometry conditioning. */
    if ((nohsworld->numVertices > 0) &&
        (params->conditionGeometry || params->calcNormals))
    {
        if (!ConvertWorldImportToGeometryList(nohsworld, &geometryList))
        {
            RWRETURN(result);
        }

        RtGCondSetUserdataCallBacks(
          UserDataCallBacks.cloneVertexUserdata,
          UserDataCallBacks.interpVertexUserdata,
          UserDataCallBacks.splitPolygonUserdata,
          UserDataCallBacks.destroyVertexUserdata,
          UserDataCallBacks.destroyPolygonUserdata);

        /* Rebuild the normals. */
        if (params->calcNormals)
        {
            RtGCondBuildNormalsPipelineNode(&geometryList);
        }

        /* Call the main geometry conditioning. */
        if (params->conditionGeometry)
        {
            RtGCondApplyGeometryConditioningPipeline(&geometryList);
        }

        newWorldImport = RtWorldImportCreate();
        ConvertGeometryListToWorldImport(&geometryList,
            newWorldImport, &nohsworld->matList);
    }
    else
    {
        newWorldImport = nohsworld;
    }

    /* In case the user has neglected to set the scheme or terminators... */
    if (BuildCallBacks.terminationBuild == NULL)
    {
        nullterminationsodefault = TRUE;

        BuildCallBacks.terminationBuild = RtWorldImportNeverPartitionTerminator;
        BuildCallBacks.terminationUserData = (void *)NULL;
    }
    if (BuildCallBacks.partitionBuild == NULL)
    {
        nullpartitionsodefault = TRUE;

        BuildCallBacks.partitionBuild = RtWorldImportBalancedCullPartitionSelector;
        BuildCallBacks.partitionUserData = (void *)&maxClosestCheck;
    }

    if (newWorldImport)
    {
        RtWorldImportParametersSet(params);
        result = _rtImportWorldCreateWorld(newWorldImport, &ExitRequested, &UserDataCallBacks);
    }
    else
    {
        if (nullterminationsodefault) BuildCallBacks.terminationBuild = NULL; /* set back to null */
        if (nullpartitionsodefault) BuildCallBacks.partitionBuild = NULL; /* set back to null */
        RWERROR((E_RW_NULLP));
    }

    if (nullterminationsodefault) BuildCallBacks.terminationBuild = NULL; /* set back to null */
    if (nullpartitionsodefault) BuildCallBacks.partitionBuild = NULL; /* set back to null */

    if (newWorldImport != nohsworld)
    {
        RtWorldImportDestroy(newWorldImport);
    }

    RWRETURN(result);
}

/**
 * \ingroup rtworldimport
 * \ref RtWorldImportCreateGeometry is used to create a geometry from
 * the specified import world according to the given conversion parameters.
 * The geometry is created with one morph target and with the following flags:
 * \ref rpGEOMETRYLIGHT, \ref rpGEOMETRYNORMALS, \ref rpGEOMETRYTEXTURED and
 * \ref rpGEOMETRYPRELIT.
 *
 * The world plugin must be attached before using this function. The include
 * file rtimport.h and the library file rtimport.lib are also required.
 *
 * \note All triangles must have an associated material before the import
 * world is converted to a geometry.
 *
 * \param nohsworld  A pointer to the import world.
 * \param conversionParams  A pointer to the world convert
 *                          structure \ref RtWorldImportParameters
 *
 * \return Returns a pointer to the new geometry if successful or NULL if
 * there is an error.
 *
 * \see RtWorldImportCreateWorld
 * \see RtWorldImportParametersInit
 * \see RtWorldImportParametersSet
 * \see RtWorldImportParametersGet
 * \see RtWorldImportSetProgressCallBack
 * \see RtWorldImportDestroy
 * \see RpWorldPluginAttach
 */
RpGeometry *
RtWorldImportCreateGeometry(RtWorldImport * nohsworld,
                            RtWorldImportParameters * conversionParams)
{
    RtWorldImportBuildSector    *buildSector;
    RpGeometry                  *gpGeom,*sortedGeom;
    RpMorphTarget               *MorphTarget;
    RpTriangle                  *tpTriangle;
    RwV3d                       *vpPoints, *vpNormals;
    RwTexCoords                 *Texcoords0,*Texcoords1;
    RwSphere                    *sphere;
    RwRGBA                      *prelit;
    RwInt32                     i;
    RwUInt32                    gflags;
    RtWorldImport               *newWorldImport;
    RtGCondGeometryList         geometryList;

    RWAPIFUNCTION(RWSTRING("RtWorldImportCreateGeometry"));
    RWASSERT(nohsworld);
    RWASSERT(conversionParams);
    RWASSERT(nohsworld->numVertices < 65535);

    /* verify RtWorldImport */
    {
        RwInt32             nI;

        for (nI = 0; nI < nohsworld->numPolygons; nI++)
        {
            RwInt32             nK;

            const RwInt32       matIndex = nohsworld->polygons[nI].matIndex;
            RwInt32            *const vertIndex = nohsworld->polygons[nI].vertIndex;

            if (matIndex < 0)
            {
                RWRETURN((RpGeometry *) NULL);
            }
            if (matIndex >= rpMaterialListGetNumMaterials(&nohsworld->matList))
            {
                RWRETURN((RpGeometry *) NULL);
            }

            for (nK = 0; nK < 3; nK++)
            {
                if (vertIndex[nK] < 0)
                {
                    RWRETURN((RpGeometry *) NULL);
                }
                if (vertIndex[nK] >= nohsworld->numVertices)
                {
                    RWRETURN((RpGeometry *) NULL);
                }
            }
        }
    }

    RtWorldImportParametersSet(conversionParams);
    if (conversionParams->conditionGeometry && nohsworld->numVertices > 0)
    {
        if (!ConvertWorldImportToGeometryList(nohsworld, &geometryList))
        {
            RWRETURN(NULL);
        }

        RtGCondSetUserdataCallBacks(
          UserDataCallBacks.cloneVertexUserdata,
          UserDataCallBacks.interpVertexUserdata,
          UserDataCallBacks.splitPolygonUserdata,
          UserDataCallBacks.destroyVertexUserdata,
          UserDataCallBacks.destroyPolygonUserdata);

        /* Rebuild the normals. */
        if (conversionParams->calcNormals)
        {
            RtGCondBuildNormalsPipelineNode(&geometryList);
        }

        /* Call the main geometry conditioning. */
        if (conversionParams->conditionGeometry)
        {
            RtGCondApplyGeometryConditioningPipeline(&geometryList);
        }

        newWorldImport = RtWorldImportCreate();
        ConvertGeometryListToWorldImport(&geometryList, newWorldImport, &nohsworld->matList);
    }
    else
    {
        newWorldImport = nohsworld;
    }


    /* Create a sector ready for use (and condition geometry) */
    buildSector = _rtImportBuildSectorCreateFromNoHSWorld(newWorldImport, &newWorldImport->matList, &UserDataCallBacks);
    if (!buildSector)
    {
        RWRETURN((RpGeometry *) NULL);
    }

    gflags = rpGEOMETRYPOSITIONS;
    if (conversionParams->flags & rpWORLDTEXTURED)
    {
        gflags |= rpGEOMETRYTEXTURED;
        if (conversionParams->flags & rpWORLDTEXTURED2)
        {
            gflags |= rpGEOMETRYTEXTURED2;
        }
    }
    if (conversionParams->numTexCoordSets == 2)
    {
        gflags |= rpGEOMETRYTEXTURED | rpGEOMETRYTEXTURED2;
    }
    if (conversionParams->flags & rpWORLDPRELIT)
    {
        gflags |= rpGEOMETRYPRELIT;
    }
    if (conversionParams->flags & rpWORLDNORMALS)
    {
        gflags |= rpGEOMETRYNORMALS;
    }
    if (conversionParams->flags & rpWORLDLIGHT)
    {
        gflags |= rpGEOMETRYLIGHT;
    }
    if (conversionParams->flags & rpWORLDTRISTRIP)
    {
        gflags |= rpGEOMETRYTRISTRIP;
    }

    gpGeom = RpGeometryCreate(buildSector->numVertices, buildSector->numPolygons, gflags);

    RpGeometryLock(gpGeom, rpGEOMETRYLOCKALL);
    MorphTarget = RpGeometryGetMorphTarget(gpGeom, 0);
    tpTriangle = RpGeometryGetTriangles(gpGeom);
    Texcoords0 = RpGeometryGetVertexTexCoords(gpGeom, rwTEXTURECOORDINATEINDEX0);
    Texcoords1 = RpGeometryGetVertexTexCoords(gpGeom, rwTEXTURECOORDINATEINDEX1);
    prelit = RpGeometryGetPreLightColors(gpGeom);
    vpPoints = RpMorphTargetGetVertices(MorphTarget);
    vpNormals = RpMorphTargetGetVertexNormals(MorphTarget);

    rpMaterialListCopy(&gpGeom->matList, &newWorldImport->matList);

    /* add geometry */
    for (i = 0; i < buildSector->numVertices; i++)
    {
        vpPoints[i] = buildSector->vertices[i].OC;
        if (gflags & rpGEOMETRYNORMALS)
        {
            vpNormals[i] = buildSector->vertices[i].normal;
        }
        if (gflags & rpGEOMETRYTEXTURED)
        {
            Texcoords0[i] = buildSector->vertices[i].texCoords[0];
        }
        if (gflags & rpGEOMETRYTEXTURED2)
        {
            Texcoords1[i] = buildSector->vertices[i].texCoords[1];
        }
        if (gflags & rpGEOMETRYPRELIT)
        {
            prelit[i] = buildSector->vertices[i].preLitCol;
        }
    }

    for (i = 0; i < buildSector->numPolygons; i++)
    {
        RwInt32             matid;
        RwUInt16            tv[3];
        RtWorldImportBuildVertexMode *mode;

        matid = buildSector->boundaries[i * 4 + 3].pinfo.matIndex;
        RpGeometryTriangleSetMaterial(gpGeom, &tpTriangle[i], gpGeom->matList.materials[matid]);

        mode = &buildSector->boundaries[i * 4 + 0].mode;
        tv[0] = (RwUInt16) (mode->vpVert - buildSector->vertices);

        mode = &buildSector->boundaries[i * 4 + 1].mode;
        tv[1] = (RwUInt16) (mode->vpVert - buildSector->vertices);

        mode = &buildSector->boundaries[i * 4 + 2].mode;
        tv[2] = (RwUInt16) (mode->vpVert - buildSector->vertices);

        RpGeometryTriangleSetVertexIndices(gpGeom, &tpTriangle[i], tv[0], tv[1], tv[2]);
    }

    /* init bounding volume */
    sphere = RpMorphTargetGetBoundingSphere(MorphTarget);
    RpMorphTargetCalcBoundingSphere(MorphTarget, sphere);
    RpMorphTargetSetBoundingSphere(MorphTarget, sphere);

    /* all done */
    RpGeometryUnlock(gpGeom);
    sortedGeom = RpGeometrySortByMaterial(gpGeom, NULL);
    RpGeometryDestroy(gpGeom);
    gpGeom = sortedGeom;

    _rtImportBuildSectorDestroy(buildSector, &UserDataCallBacks);

    if (newWorldImport != nohsworld)
    {
        RtWorldImportDestroy(newWorldImport);
    }

    RWRETURN(gpGeom);
}

/**
 * \ingroup rtworldimport
 * \ref RtWorldImportCreate is used to create a new import world that
 * has no vertices or triangles.
 *
 * Vertices and triangles must be allocated and initialized before attempting
 * to create a world using \ref RtWorldImportCreateWorld. Use the function
 * \ref RtWorldImportDestroy to free this memory.
 *
 * Also, an \ref RtWorldImportParameters structure must be defined, and
 * initialized with \ref RtWorldImportParametersInit, for use in creating
 * a BSP world from the import world.
 *
 * The world plugin must be attached before using this function. The include
 * file rtimport.h and the library file rtimport.lib are also required.
 *
 * \return Returns a pointer to a new import world if successful or NULL if
 * there is an error.
 *
 * \see RtWorldImportDestroy
 * \see RtWorldImportAddNumVertices
 * \see RtWorldImportAddNumTriangles
 * \see RtWorldImportAddMaterial
 * \see RtWorldImportCreateWorld
 * \see RtWorldImportParametersInit
 * \see RtWorldImportParametersSet
 * \see RtWorldImportParametersGet
 * \see RtWorldImportSetBuildCallBacks
 * \see RtWorldImportSetBuildCallBacksUserData
 * \see RtWorldImportSetStandardBuildPartitionSelector
 * \see RtWorldImportRead
 * \see RtWorldImportWrite
 * \see RpWorldPluginAttach
 */
RtWorldImport      *
RtWorldImportCreate(void)
{
    RtWorldImport      *nohsworld;

    RWAPIFUNCTION(RWSTRING("RtWorldImportCreate"));

    nohsworld = (RtWorldImport *) RwMalloc(sizeof(RtWorldImport),
                       rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);

    if (!nohsworld)
    {
        RWRETURN((RtWorldImport *) NULL);
    }

    rpMaterialListInitialize(&nohsworld->matList);
    /* this world has no vertices or polygons */
    nohsworld->vertices = (RtWorldImportVertex *) NULL;
    nohsworld->numVertices = 0;
    nohsworld->polygons = (RtWorldImportTriangle *) NULL;
    nohsworld->numPolygons = 0;

    RWRETURN(nohsworld);
}

/**
 * \ingroup rtworldimport
 * \ref RtWorldImportDestroy is used to destroy the specified import
 * world. This function should be used to free the memory allocated to
 * triangles, vertices and materials created via \ref RtWorldImportCreate or
 * \ref RtWorldImportRead.
 *
 * The world plugin must be attached before using this function. The include
 * file rtimport.h and the library file rtimport.lib are also required.
 *
 * \param nohsworld  A pointer to the import world.
 *
 * \return Returns TRUE if successful or FALSE if there is an error
 *
 * \see RtWorldImportCreate
 * \see RtWorldImportRead
 * \see RpWorldPluginAttach
 */
RwBool
RtWorldImportDestroy(RtWorldImport * nohsworld)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportDestroy"));
    RWASSERT(nohsworld);

    if (nohsworld)
    {
        /* Destroy all of the materials being referenced */
        rpMaterialListDeinitialize(&nohsworld->matList);

        /* Free the polygons */
        ImportWorldFreeTriangles(nohsworld);

        /* Free the vertices */
        ImportWorldFreeVertices(nohsworld);

        /* Free the world!! */
        RwFree(nohsworld);

        RWRETURN(TRUE);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN(FALSE);
}

/**
 * \ingroup hints
 * \ref RtWorldImportHintsSetGroup is used to set the hints that should be used
 * while building the \ref RpWorld
 *
 * \param hints  A pointer to the hints
 * \param group  An index from \ref RtWorldImportHintGroup to specify the hint group
 */
void
RtWorldImportHintsSetGroup(RtWorldImportHints *hints, RtWorldImportHintGroup group)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportHintsSetGroup"));

    WorldImportHints[group] = hints;

    RWRETURNVOID();
}


/**
 * \ingroup hints
 * \ref RtWorldImportHintsGetGroup is used to get the hints that should be used
 * while building the \ref RpWorld
 *
 * \param group  An index from \ref RtWorldImportHintGroup to specify the hint group
 *
 * \returns  A pointer to the hints
 */
RtWorldImportHints *
RtWorldImportHintsGetGroup(RtWorldImportHintGroup group)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportHintsGetGroup"));
    RWRETURN(WorldImportHints[group]);
}


/**
 * \ingroup hints
 * \ref RtWorldImportHintsCreate Creates a new set of hints, ready for populating.
 *
 * \return  A pointer to the \ref RtWorldImportHints
 *
 * \see RtWorldImportHintsSetGroup
 * \see RtWorldImportHintsGetGroup
 * \see RtWorldImportHintsDestroy
 * \see RtWorldImportHintsAddBoundingBoxes
 * \see RtWorldImportMaterialGroupHintGenerator
 */
RtWorldImportHints *
RtWorldImportHintsCreate(void)
{
    RtWorldImportHints *hints;

    RWAPIFUNCTION(RWSTRING("RtWorldImportHintsCreate"));

    hints = (RtWorldImportHints *) RwMalloc(sizeof(RtWorldImportHints),
                            rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);

    if(!hints)
    {
        RWRETURN((RtWorldImportHints *) NULL);
    }

    hints->numBoundingBoxes = 0;
    hints->boundingBoxes = (RtWorldImportBBoxHintDesc *)NULL;

    RWRETURN(hints);
}


/**
 * \ingroup hints
 * \ref RtWorldImportHintsDestroy Destroys a set of hints.
 *
 * \param hints A pointer to the \ref RtWorldImportHints
 *
 * \return      TRUE if successful, FALSE otherwise.
 *
 * \see RtWorldImportHintsSetGroup
 * \see RtWorldImportHintsGetGroup
 * \see RtWorldImportHintsCreate
 * \see RtWorldImportHintsAddBoundingBoxes
 * \see RtWorldImportMaterialGroupHintGenerator
 */
RwBool
RtWorldImportHintsDestroy(RtWorldImportHints *hints)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportHintsDestroy"));
    RWASSERT(hints);

    if(hints)
    {
        /* Free the bounding boxes */
        RwFree(hints->boundingBoxes);

        /* Free the hints structure */
        RwFree(hints);

        RWRETURN(TRUE);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN(FALSE);
}

/**
 * \ingroup hints
 * \ref RtWorldImportHintsAddBoundingBoxes Allocates memory for a number of bounding
 * boxes to the hints
 *
 * \param hints             A pointer to the \ref RtWorldImportHints
 * \param numBoundingBoxes  The number of boxes to be added
 *
 * \return A pointer to the \ref RtWorldImportHints if successful, NULL otherwise.
 *
 * \see RtWorldImportHintsSetGroup
 * \see RtWorldImportHintsGetGroup
 * \see RtWorldImportHintsCreate
 * \see RtWorldImportHintsDestroy
 * \see RtWorldImportMaterialGroupHintGenerator
 */
RtWorldImportHints *
RtWorldImportHintsAddBoundingBoxes(RtWorldImportHints *hints, RwInt32 numBoundingBoxes)
{
    RtWorldImportBBoxHintDesc *bboxHints;

    RWAPIFUNCTION(RWSTRING("RtWorldImportHintsAddBoundingBoxes"));
    RWASSERT(hints);

    if(!hints)
    {
        RWERROR((E_RW_NULLP));
    }
    else if(numBoundingBoxes > 0)
    {
        hints->numBoundingBoxes += numBoundingBoxes;

        if (!hints->boundingBoxes)
        {
            /* for initial memory allocation use RwMalloc() */
            hints->boundingBoxes = (RtWorldImportBBoxHintDesc*) RwMalloc(sizeof(RtWorldImportBBoxHintDesc)*hints->numBoundingBoxes,
                                                             rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT | rwMEMHINTFLAG_RESIZABLE);
        }
        else
        {
            /* use RwRealloc for expanding memory */
            bboxHints = (RtWorldImportBBoxHintDesc*) RwRealloc(hints->boundingBoxes, sizeof(RtWorldImportBBoxHintDesc)*hints->numBoundingBoxes,
                                                             rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT | rwMEMHINTFLAG_RESIZABLE);
            if(bboxHints)
            {
                hints->boundingBoxes = bboxHints;
            }
            else
            {
                RWRETURN((RtWorldImportHints *) NULL);
            }
        }

        if (!hints->boundingBoxes)
        {
            hints->numBoundingBoxes = 0;
            hints->boundingBoxes = (RtWorldImportBBoxHintDesc*)NULL;

            RWRETURN((RtWorldImportHints *) NULL);
        }
    }

    RWRETURN(hints);
}


