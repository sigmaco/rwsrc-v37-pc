
/****************************************************************************
 Includes
 */

#include <stdlib.h>
#include <string.h>

#include "rpplugin.h"
#include <rpdbgerr.h>
#include <rwcore.h>

#include "rtltmapcnv.h"

/****************************************************************************
 Local Defines
 */

/****************************************************************************
 Public Globals
 */

#if (defined(RWDEBUG))
long                RtLtMapConverStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

RwChar rtLtMapCnvUVtxUserDataName[] = RWSTRING("rtLtMapCnvVTXDATA_U");
RwChar rtLtMapCnvVVtxUserDataName[] = RWSTRING("rtLtMapCnvVTXDATA_V");
RwChar rtLtMapCnvPolyUserDataName[] = RWSTRING("rtLtMapCnvPOLYDATA");

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Structures
 */

/****************************************************************************
 Image Cache
 */

/****************************************************************************
 * Image linked list system :
 * - the list parameters can be null at all time
 * - there is a double check : assert + error recovery, as should have all
 *   the fonctions of the toolkit.
 */

rtLtMapCnvImageList *
_rtLtMapCnvImageCacheGet(   const rtLtMapCnvImageList *list,
                                const RwChar *name)
{
    rtLtMapCnvImageList *item;
    RWFUNCTION(RWSTRING("_rtLtMapCnvImageCacheGet"));
    RWASSERT(NULL != name);

    if( NULL != list && NULL != name)
    {
        (const rtLtMapCnvImageList *) item = list;

        while( NULL != item )
        {
            if( 0 == strcmp(item->name,name) )
            {
                RWRETURN(item);
            }

            item = item->next;
        }

    }

    RWRETURN((rtLtMapCnvImageList *)NULL);
}

rtLtMapCnvImageList *
_rtLtMapCnvImageCacheAdd(   const rtLtMapCnvImageList *list,
                                const RwChar *name)
{
    rtLtMapCnvImageList *item = NULL;
    RWFUNCTION(RWSTRING("_rtLtMapCnvImageCacheAdd"));
    RWASSERT(NULL != name);

    if( NULL != name )
    {
        item = RwMalloc(sizeof(rtLtMapCnvImageList),
            rwID_LTMAPCNVTOOLKIT | rwMEMHINTDUR_EVENT);
        if( NULL != item )
        {
            (const rtLtMapCnvImageList *) item->next = list;
            item->texture = NULL;
            item->image = NULL;

            rwstrdup(item->name,name);
        }
    }

    RWRETURN(item);
}

void
_rtLtMapCnvImageCacheDestroy(rtLtMapCnvImageList *list)
{
    rtLtMapCnvImageList *item = NULL;
    rtLtMapCnvImageList *next = NULL;

    RWFUNCTION(RWSTRING("_rtLtMapCnvImageCacheDestroy"));
    RWASSERT(NULL != list);

    if( NULL != list )
    {
        item = (rtLtMapCnvImageList *)list;

        while( NULL != item )
        {
            next = item->next;

            if( NULL != item->texture )
            {
                RwTextureDestroy(item->texture);
            }

            if( NULL != item->image )
            {
                RwImageDestroy(item->image);
            }

            if( NULL != item->name )
            {
                RwFree(item->name);
            }

            RwFree(item);

            item = next;
        }
    }

    RWRETURNVOID();
}

#if ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) )
#endif /* ( defined(RWDEBUG) || defined(RWSUPPRESSINLINE) ) */

