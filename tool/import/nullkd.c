#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "rwcore.h"
#include "rpworld.h"

#include "rpdbgerr.h"

#include "rtimport.h"

#include "nhsstats.h"
#include "nhsutil.h"
#include "nhsworld.h"
#include "rtgcond.h"
//#include "nhswing.h"

#include "nhssplit.h"
#include "nhsscheme.h"

#include "nullkd.h"

void
_rtWorldImportGuideKDStackDestroy(_rtWorldImportGuideKDStack *stack)
{
    _rtWorldImportGuideKDStackElement *temp;

    RWFUNCTION(RWSTRING("_rtWorldImportGuideKDStackDestroy"));

    while(stack->top)
    {
        temp = stack->top;
        stack->top = stack->top->next;
        RwFree(temp);
    }

    stack->bottom = NULL;
    stack->top = NULL;

    RWRETURNVOID();
}

void
_rtWorldImportGuideKDPush(RtWorldImportGuideKDTree *tree, _rtWorldImportGuideKDStack *stack)
{
    _rtWorldImportGuideKDStackElement *temp, *ptr;

    RWFUNCTION(RWSTRING("_rtWorldImportGuideKDPush"));

    if (stack->top==NULL)
    {
        stack->top = (_rtWorldImportGuideKDStackElement*)RwMalloc(sizeof(_rtWorldImportGuideKDStackElement),
                                                                  rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
        stack->top->next = NULL;
        stack->bottom = stack->top;
        ptr = stack->top;
    }
    else
    {
        temp = (_rtWorldImportGuideKDStackElement*)RwMalloc(sizeof(_rtWorldImportGuideKDStackElement),
                                                            rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
        stack->bottom->next = temp;
        stack->bottom = temp;
        stack->bottom->next = NULL;
        ptr = stack->bottom;
    }
    ptr->node = tree;
    if (tree->partition.type==-1) ptr->terminal = TRUE;
    else ptr->terminal = FALSE;

    RWRETURNVOID();
}


void
_rtWorldImportGuideKDPop(_rtWorldImportGuideKDStack *stack)
{
    RWFUNCTION(RWSTRING("_rtWorldImportGuideKDPop"));

    stack->current = stack->current->next;

    RWRETURNVOID();
}

void
_rtWorldImportGuideKDEncodeAsStack(RtWorldImportGuideKDTree *tree, _rtWorldImportGuideKDStack *stack)
{
    RWFUNCTION(RWSTRING("_rtWorldImportGuideKDEncodeAsStack"));

    if (tree!=NULL)
    {
        _rtWorldImportGuideKDPush(tree, stack);
        _rtWorldImportGuideKDEncodeAsStack(tree->left, stack);
        _rtWorldImportGuideKDEncodeAsStack(tree->right, stack);
    }

    RWRETURNVOID();
}

/**
 * \ingroup kd
 * \ref RtWorldImportGuideKDCreate Creates a single node \ref RtWorldImportGuideKDTree
 *
 * \param bbox  The bounding box of the world.
 *
 * \return      The root of the \ref RtWorldImportGuideKDTree
 *
 * \see RtWorldImportGuideKDRead
 * \see RtWorldImportGuideKDWrite
 * \see RtWorldImportGuideKDDestroy
 * \see RtWorldImportGuideKDDeletePartition
 * \see RtWorldImportGuideKDAddPartition
 */
RtWorldImportGuideKDTree*
RtWorldImportGuideKDCreate(RwBBox * bbox)
{
    RtWorldImportGuideKDTree * KD;

    RWAPIFUNCTION(RWSTRING("RtWorldImportGuideKDCreate"));



    KD = (RtWorldImportGuideKDTree*)RwMalloc(sizeof(RtWorldImportGuideKDTree),
                                  rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);

    /* This is a sector, so all partitioning values are -1 */
    KD->partition.type = -1;
    KD->partition.value= -1;
    KD->partition.maxLeftValue= -1;
    KD->partition.maxRightValue= -1;
    KD->parent = NULL;
    KD->order = 0;
    KD->bbox.inf = bbox->inf; /* IF IT FAILS HERE, MAX DEPTH WAS EXCEEDED ON BUILD! */
    KD->bbox.sup = bbox->sup;

    /* Sector is a leaf node, so set children to NULL */
    KD->left = NULL;
    KD->right = NULL;

    RWRETURN(KD);
}

/**
 * \ingroup kd
 * \ref RtWorldImportGuideKDDestroy Destroys the \ref RtWorldImportGuideKDTree
 *
 * \param KD    The \ref RtWorldImportGuideKDTree
 *
 * \see RtWorldImportGuideKDRead
 * \see RtWorldImportGuideKDWrite
 * \see RtWorldImportGuideKDDeletePartition
 * \see RtWorldImportGuideKDAddPartition
 * \see RtWorldImportGuideKDCreate
 */
void
RtWorldImportGuideKDDestroy(RtWorldImportGuideKDTree* KD)
{
    RWAPIFUNCTION(RWSTRING("RtWorldImportGuideKDDestroy"));

    if (KD!=NULL)
    {
        RtWorldImportGuideKDDestroy(KD->left);
        RtWorldImportGuideKDDestroy(KD->right);
        RwFree(KD);

    }
    RWRETURNVOID();
}

static RwBool
CheckPartitionAgainstBox(RwBBox * box, RwReal newvalue, RwInt32 newtype)
{
    RWFUNCTION(RWSTRING("CheckPartitionAgainstBox"));

    if (newvalue>GETCOORD(box->sup, newtype) ||
        newvalue<GETCOORD(box->inf, newtype))
    {
        RWRETURN(FALSE);
    }
    RWRETURN(TRUE);
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

void
_rtWorldImportGuideKDCopy(RtWorldImportGuideKDTree* KD, RpSector * spSector, RwInt32 depth)
{
    RWFUNCTION(RWSTRING("_rtWorldImportGuideKDCopy"));

    if (spSector->type >= 0)
    {
        /* It's a plane */
        RpPlaneSector      *pspPlane = (RpPlaneSector *) spSector;

        KD->partition.type = pspPlane->type;
        KD->partition.value= pspPlane->value;
        KD->partition.maxLeftValue= pspPlane->leftValue;
        KD->partition.maxRightValue= pspPlane->rightValue;


        KD->left = (RtWorldImportGuideKDTree*)RwMalloc(sizeof(RtWorldImportGuideKDTree),
                                               rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);
        KD->right = (RtWorldImportGuideKDTree*)RwMalloc(sizeof(RtWorldImportGuideKDTree),
                                               rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);

        KD->left->partition.type = -1;
        KD->left->partition.value= -1;
        KD->left->partition.maxLeftValue= -1;
        KD->left->partition.maxRightValue= -1;
        KD->left->parent = KD;
        KD->left->order = 0;
        KD->left->left = NULL;
        KD->left->right = NULL;

        KD->right->partition.type = -1;
        KD->right->partition.value= -1;
        KD->right->partition.maxLeftValue= -1;
        KD->right->partition.maxRightValue= -1;
        KD->right->parent = KD;
        KD->right->order = 1;
        KD->right->left = NULL;
        KD->right->right = NULL;

        DivideBox(&(KD->bbox), KD->partition.maxLeftValue, KD->partition.maxRightValue,
             &(KD->left->bbox), &(KD->right->bbox),
             KD->partition.type);

        /* Recursively subdivide */
        _rtWorldImportGuideKDCopy((KD->left), pspPlane->leftSubTree, depth+1);
        _rtWorldImportGuideKDCopy((KD->right), pspPlane->rightSubTree, depth+1);

    }
    RWRETURNVOID();
}

/**
 * \ingroup kd
 * \ref RtWorldImportGuideKDAddPartition Modifies a given leaf node by partitioning
 * it, turning the given node into a non-leaf node and creating two new leaf-node children.
 * To use this function, you need to setup the global import parameters by using 
 * RtWorldImportParametersSet.
 *
 * \param KD    The leaf node of the \ref RtWorldImportGuideKDTree
 * \param type  The orientation of the partition 0, 4, 8 for normal to the x, y or z-axis
 * \param value The signed distance of the plane from the origin
 *
 * \return      The new node if successful, NULL otherwise.
 *              A partition is invalid if it is not inside the half space of its parent
 *
 * \see RtWorldImportGuideKDRead
 * \see RtWorldImportGuideKDWrite
 * \see RtWorldImportGuideKDDestroy
 * \see RtWorldImportGuideKDDeletePartition
 * \see RtWorldImportGuideKDCreate
 */
RtWorldImportGuideKDTree *
RtWorldImportGuideKDAddPartition(RtWorldImportGuideKDTree * KD, RwInt32 type, RwReal value)
{
    RtWorldImportParameters *conversionParams = RtWorldImportParametersGet();

    RWAPIFUNCTION(RWSTRING("RtWorldImportGuideKDAddPartition"));
    RWASSERT(KD->partition.type==-1); /* This should be a sector - can't partition! */
    RWASSERT(conversionParams != NULL);

    if (!CheckPartitionAgainstBox(&KD->bbox, value, type)) RWRETURN(NULL); /* invalid */

    /* Create attribute of non-leaf node */
    KD->partition.type = type;
    KD->partition.value = value;
    BuildSectorSetOverlapsMacro(KD->bbox, KD->partition, *conversionParams);


    /* Create left leaf */
    KD->left = (RtWorldImportGuideKDTree*)RwMalloc(sizeof(RtWorldImportGuideKDTree),
                                           rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);

    KD->left->partition.type = -1;
    KD->left->partition.value= -1;
    KD->left->partition.maxLeftValue= -1;
    KD->left->partition.maxRightValue= -1;
    KD->left->parent = KD;
    KD->left->order = 0;
    KD->left->left = NULL;
    KD->left->right = NULL;



    /* create right leaf */
    KD->right = (RtWorldImportGuideKDTree*)RwMalloc(sizeof(RtWorldImportGuideKDTree),
                                          rwID_NOHSWORLDPLUGIN | rwMEMHINTDUR_EVENT);

    KD->right->partition.type = -1;
    KD->right->partition.value= -1;
    KD->right->partition.maxLeftValue= -1;
    KD->right->partition.maxRightValue= -1;
    KD->right->parent = KD;
    KD->right->order = 1;
    KD->right->left = NULL;
    KD->right->right = NULL;


    DivideBox(&(KD->bbox), KD->partition.maxLeftValue, KD->partition.maxRightValue,
             &(KD->left->bbox), &(KD->right->bbox),
             type);


    RWRETURN(KD);
}

/**
 * \ingroup kd
 * \ref RtWorldImportGuideKDDeletePartition Modifies a given node, by destroying
 * its childern, turning the given node into a leaf node.
 *
 * \param KD    The node of the \ref RtWorldImportGuideKDTree
 *
 * \see RtWorldImportGuideKDRead
 * \see RtWorldImportGuideKDWrite
 * \see RtWorldImportGuideKDDestroy
 * \see RtWorldImportGuideKDAddPartition
 * \see RtWorldImportGuideKDCreate
 */
void
RtWorldImportGuideKDDeletePartition(RtWorldImportGuideKDTree * KD)
{
    /* Deletes the children of the given KD, but maintains the node and its partitioner */

    RWAPIFUNCTION(RWSTRING("RtWorldImportGuideKDDeletePartition"));

    if (KD==NULL) RWRETURNVOID();

    RtWorldImportGuideKDDeletePartition(KD->left);
    RtWorldImportGuideKDDeletePartition(KD->right);

    if ((KD->partition.type!=-1) && (KD->left->partition.type == -1) && (KD->right->partition.type == -1))
    {
        RwFree(KD->left);
        KD->left = NULL;

        RwFree(KD->right);
        KD->right = NULL;

        KD->partition.type=-1;
        KD->partition.value=-1;
        KD->partition.maxRightValue=-1;
        KD->partition.maxLeftValue=-1;
    }
    RWRETURNVOID();
}
