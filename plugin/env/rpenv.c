/*
 * RenderWare environment plugin
 */
/**
 * \ingroup rpenv
 * \page rpenvoverview RpEnv Plugin Overview
 *
 * The RpEnvironment plugin can be used by tools to set a background color
 * to assets.
 *
 */ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rpplugin.h"
#include "rpdbgerr.h"
#include "rwcore.h"
#include "rpworld.h"
#include "rpenv.h"

#if (defined(RWDEBUG))
long                rpEnvironmentStackDepth = 0;
#endif /* (defined(RWDEBUG)) */

/**
 * \ingroup rpenv
 * \ref RpEnvironmentCreate
 * creates and returns a standard RpEnvironment. The RpEnvironment created will 
 * initialize the corresponding background color to the following values:
 * \li red   = 0
 * \li green = 0
 * \li blue  = 0
 * \li alpha = 255
 *
 * \return A pointer to the new RpEnvironment if successful, NULL otherwise. 
 *
 * \see RpEnvironmentDestroy
 * \see RpEnvironmentStreamGetSize
 * \see RpEnvironmentStreamRead
 * \see RpEnvironmentStreamWrite
 *
 */
RpEnvironment *
RpEnvironmentCreate( void )
{
    RpEnvironment *environment = NULL;

    RWAPIFUNCTION(RWSTRING("RpEnvironmentCreate"));

    environment = (RpEnvironment *) RwMalloc(sizeof(RpEnvironment), 
                        rwMEMHINTDUR_EVENT | rwID_ENVIRONMENTPLUGIN);

    environment->bgColor.red = 0;
    environment->bgColor.green = 0;
    environment->bgColor.blue = 0;
    environment->bgColor.alpha = 255;
    
    RWRETURN((environment));
}

/**
 * \ingroup rpenv
 * \ref RpEnvironmentDestroy
 * destroys a RpEnvironment setting.
 *
 * \param environment   A pointer to the RpEnvironment settings to destroy 
 *
 * \see RpEnvironmentCreate
 * \see RpEnvironmentStreamGetSize
 * \see RpEnvironmentStreamRead
 * \see RpEnvironmentStreamWrite
 *
 */
void
RpEnvironmentDestroy(RpEnvironment *environment)
{
    RWAPIFUNCTION(RWSTRING("RpEnvironmentDestroy"));
    
    RwFree(environment);
    environment = (RpEnvironment *)NULL;

    RWRETURNVOID();
}

/**
 * \ingroup rpenv
 * \ref RpEnvironmentStreamGetSize
 * returns the size of an RpEnvironment object.
 * \param environment   A pointer to the RpEnvironment settings to get 
 *                      the size from.
 *
 * \return The size of the stream
 * \see RpEnvironmentCreate
 * \see RpEnvironmentDestroy
 * \see RpEnvironmentStreamRead
 * \see RpEnvironmentStreamWrite
 *
 */
RwInt32
RpEnvironmentStreamGetSize(RpEnvironment *environment __RWUNUSED__)
{
    RwInt32             size = 0;

    RWAPIFUNCTION(RWSTRING("RpEnvironmentStreamGetSize"));

    size += sizeof(RpEnvironment) + rwCHUNKHEADERSIZE;

    RWRETURN(size);
}

/**
 * \ingroup rpenv
 * \ref RpEnvironmentStreamRead
 * reads the RpEnvironment chunk from a Renderware stream.
 *
 * \param stream The stream to read the RpEnvironment object from.
 *
 * \return The RpEnvironment object within the stream.
 *
 * \see RpEnvironmentCreate
 * \see RpEnvironmentDestroy
 * \see RpEnvironmentStreamGetSize
 * \see RpEnvironmentStreamWrite
 *
 */
RpEnvironment        *
RpEnvironmentStreamRead(RwStream * stream)
{
    RpEnvironment        *environment = RpEnvironmentCreate();
    RpEnvironment         _environment;
    RwUInt32            length;
    RwUInt32            ver;

    RWAPIFUNCTION(RWSTRING("RpEnvironmentStreamRead"));

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &length, &ver))
    {
        RpEnvironmentDestroy(environment);
        RWRETURN(NULL);
    }
    RwStreamRead(stream, &_environment, sizeof(_environment));

    environment->bgColor = _environment.bgColor;

    RWRETURN((environment));
}

/**
 * \ingroup rpenv
 * \ref RpEnvironmentStreamWrite
 * streams out an RpEnvironment object to a RenderWare stream.
 *
 * \param environment   A pointer to the RpEnvironment settings to write out.
 * \param stream        A pointer to the stream to write the RpEnvironment to.
 *
 * \return A pointer to the RpEnvironment writen out.
 * \see RpEnvironmentCreate
 * \see RpEnvironmentDestroy
 * \see RpEnvironmentStreamGetSize
 * \see RpEnvironmentStreamRead
 *
 */
RpEnvironment *
RpEnvironmentStreamWrite(RpEnvironment *environment, RwStream * stream)
{
    RpEnvironment         _environment;

    RWAPIFUNCTION(RWSTRING("RpEnvironmentStreamWrite"));

    RwStreamWriteChunkHeader(stream, rwID_ENVIRONMENT,
                             RpEnvironmentStreamGetSize(environment));

    RwStreamWriteChunkHeader(stream, rwID_STRUCT, sizeof(RpEnvironment));

    _environment.bgColor = environment->bgColor;
    
    RwStreamWrite(stream, &_environment, sizeof(_environment));

    RWRETURN((environment));
}

/**
 * \ingroup rpenv
 * \ref RpEnvironmentSetBackground
 * sets the background color for a specific RpEnvironment.
 *
 * \param environment   A pointer to the RpEnvironment object to set the color
 *                      for.
 * \param color         The new background color.
 *
 * \see RpEnvironmentCreate
 * \see RpEnvironmentDestroy
 * \see RpEnvironmentStreamGetSize
 * \see RpEnvironmentStreamRead
 *
 */
void 
RpEnvironmentSetBackground(RpEnvironment *environment, RwRGBA color)
{
    RWAPIFUNCTION(RWSTRING("RpEnvironmentSetBackground"));
    RWASSERT(environment);

    environment->bgColor = color;

    RWRETURNVOID();
}

/**
 * \ingroup rpenv
 * \ref RpEnvironmentGetBackground
 * returns the current background color for a specific RpEnvironment object.
 *
 * \param environment   A pointer to the RpEnvironment object to get the color
 *                      from.
 *
 * \return              A pointer to the background color of the specified 
 *                      RpEnvironment.
 *
 * \see RpEnvironmentCreate
 * \see RpEnvironmentDestroy
 * \see RpEnvironmentStreamGetSize
 * \see RpEnvironmentStreamRead
 *
 */
RwRGBA *
RpEnvironmentGetBackground(RpEnvironment *environment)
{
    RWAPIFUNCTION(RWSTRING("RpEnvironmentGetBackground")); 
    RWASSERT(environment);
    RWRETURN(&environment->bgColor);
}


