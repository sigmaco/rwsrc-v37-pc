/*
 * Potentially Visible Set plug-in
 */

/**********************************************************************
 *
 * file :     rppvsaux.c
 *
 * abstract : handle culling of worldsectors in RenderWare
 *
 **********************************************************************
 *
 * This file is a product of Criterion Software Ltd.
 *
 * This file is provided as is with no warranties of any kind and is
 * provided without any obligation on Criterion Software Ltd. or
 * Canon Inc. to assist in its use or modification.
 *
 * Criterion Software Ltd. will not, under any
 * circumstances, be liable for any lost revenue or other damages arising
 * from the use of this file.
 *
 * Copyright (c) 2001 Criterion Software Ltd.
 * All Rights Reserved.
 *
 * RenderWare is a trademark of Canon Inc.
 *
 ************************************************************************/

#include "rwcore.h"
#include "rpworld.h"

#include "rppvs.h"
#include "rppvsaux.h"

#include "rpplugin.h"
#include "rpdbgerr.h"


_rpPVSPolyListPtr
_rpPVScreateNode(_rpPVSPolyRecord * t)
{
    _rpPVSPolyListPtr         addition;

    RWFUNCTION(RWSTRING("_rpPVScreateNode"));

    addition = (_rpPVSPolyListPtr) RwMalloc(sizeof(_rpPVSPolyList),
                             rwID_GPVSPLUGIN | rwMEMHINTDUR_EVENT);
    addition->data = *t;
    addition->next = NULL;

    RWRETURN(addition);
}

void
_rpPVSaddendII(_rpPVSPolyListPtr * end, _rpPVSPolyRecord * t)
{
    _rpPVSPolyListPtr         addition;

    RWFUNCTION(RWSTRING("_rpPVSaddendII"));

    addition = (_rpPVSPolyListPtr) RwMalloc(sizeof(_rpPVSPolyList),
                             rwID_GPVSPLUGIN | rwMEMHINTDUR_EVENT);

    addition->data = *t;
    addition->next = NULL;

    if (*end != NULL)
    {
        (*end)->next = addition;
        (*end) = (*end)->next;
    }
    else
        (*end) = addition;

    RWRETURNVOID();
}

void
_rpPVSaddendIIp(_rpPVSPolyPtr * end, RwV3d * t)
{
    _rpPVSPolyPtr             addition;

    RWFUNCTION(RWSTRING("_rpPVSaddendIIp"));

    addition = (_rpPVSPolyPtr) RwMalloc(sizeof(_rpPVSPoly),
                        rwID_GPVSPLUGIN | rwMEMHINTDUR_EVENT);

    addition->v.x = t->x;
    addition->v.y = t->y;
    addition->v.z = t->z;

    addition->next = NULL;

    if (*end != NULL)
    {
        (*end)->next = addition;
        (*end) = (*end)->next;
    }
    else
        (*end) = addition;

    RWRETURNVOID();
}

void
_rpPVSDestroyPolyList(_rpPVSPolyListPtr * P)
{
    _rpPVSPolyListPtr         temp;

    RWFUNCTION(RWSTRING("_rpPVSDestroyPolyList"));

    while ((*P) != NULL)
    {
        temp = (*P);
        (*P) = (*P)->next;
        RwFree(temp);
        temp = NULL;
    }

    RWRETURNVOID();
}

void
_rpPVSDestroyPoly(_rpPVSPolyPtr * P)
{
    _rpPVSPolyPtr             temp;

    RWFUNCTION(RWSTRING("_rpPVSDestroyPoly"));

    while ((*P) != NULL)
    {
        temp = (*P);
        (*P) = (*P)->next;
        RwFree(temp);
        temp = NULL;
    }

    RWRETURNVOID();
}

RwInt32
_rpPVSLinkCardinality(_rpPVSPolyListPtr list)
{
    RwInt32             i = 0;
    _rpPVSPolyListPtr         temp = list;

    RWFUNCTION(RWSTRING("_rpPVSLinkCardinality"));
    while (temp != NULL)
    {
        i++;
        temp = temp->next;
    }
    RWRETURN(i);
}

RwInt32
_rpPVSLinkCardinality2(_rpPVSPolyPtr list)
{
    RwInt32             i = 0;
    _rpPVSPolyPtr             temp = list;

    RWFUNCTION(RWSTRING("_rpPVSLinkCardinality2"));

    while (temp != NULL)
    {
        i++;
        temp = temp->next;
    }
    RWRETURN(i);
}

static RpWorldSector *
PVSGetNumSectors(RpWorldSector * worldSector, void *pData)
{
    RWFUNCTION(RWSTRING("PVSGetNumSectors"));
    RWASSERT(worldSector);
    RWASSERT(pData);

    if (worldSector && pData)
    {
        /* Increase the counter */
        (*(RwInt32 *) pData)++;

        RWRETURN(worldSector);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN((RpWorldSector *)NULL);
}

RwInt32
_rpPVSWorldGetNumWorldSectors(RpWorld * world)
{
    RWFUNCTION(RWSTRING("_rpPVSWorldGetNumWorldSectors"));
    RWASSERT(world);

    if (world)
    {
        RwInt32             nCount = 0;

        RpWorldForAllWorldSectors(world, PVSGetNumSectors, &nCount);

        RWRETURN(nCount);
    }

    RWERROR((E_RW_NULLP));
    RWRETURN(-1);
}
