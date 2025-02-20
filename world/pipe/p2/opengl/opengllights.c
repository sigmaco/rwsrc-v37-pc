/***********************************************************************
 *
 * Module:  opengllights.c
 *
 * Purpose: OpenGL Lighting
 *
 ***********************************************************************/

/* =====================================================================
 *  Includes
 * ===================================================================== */
#include "baworld.h"
#include "opengllights.h"
#include "openglpipe.h"


/* =====================================================================
 *  Defines
 * ===================================================================== */
#define COLORSCALAR 0.003921568627450980392156862745098f    /* 1.0f / 255.0f */

#define RAD_2_DEG   57.295779513082320876798154814105f

#define OPENGLLIGHTDATAFROMLIGHT(_light)                          \
    (_rpOpenGLLightData *)( ((RwInt8 *)_light) + rpOpenGLLightOffset )

#define OPENGLLIGHTDATAFROMCONSTLIGHT(_light)                      \
    (const _rpOpenGLLightData *)( ((const RwInt8 *)_light) + rpOpenGLLightOffset )

#define openglEnableLightMacro(_idx)            \
    RwOpenGLEnable( rwGL_LIGHT0 + (_idx) )

#define openglDisableLightMacro(_idx)           \
    RwOpenGLDisable( rwGL_LIGHT0 + (_idx) )

#if !(defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
#define openglEnableLight       openglEnableLightMacro
#define openglDisableLight      openglDisableLightMacro
#endif /* !(defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */
typedef struct _rpOpenGLLightData _rpOpenGLLightData;
struct _rpOpenGLLightData
{
    RwReal                      softSpotExponent;

    RpOpenGLLightAttentuation   attenuationParams;
};


/* =====================================================================
 *  Static variables
 * ===================================================================== */
/**
 * \ingroup worldextensionsenhancementsopengl
 * \page worldextenhancementsopengloverview Overview
 *
 * The functions in this section allow you to enhance the
 * performance of render pipelines by making use of
 * a cache, which is managed by RenderWare Graphics for OpenGL.
 *
 * These functions manage lighting aspects of rendering, such as material
 * properties, and normal normalization.
 *
 */

static RwInt32      rpOpenGLLightOffset;

static RwUInt32     openglLightsMaxLights;

static RwBool       openglUseAmbientLight;

static RwUInt32     openglPrevNumLightsApplied;

static RwUInt32     openglNumLightsApplied;

static const RwReal openglOpaqueBlack[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

static const RwReal openglOpaqueWhite[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

static RwRGBAReal   openglAmbientLight;


/* =====================================================================
 *  Global variables
 * ===================================================================== */


/* =====================================================================
 *  Static function prototypes
 * ===================================================================== */
static void *
openglLightConstructor( void *object,
                        RwInt32 offsetInObject,
                        RwInt32 sizeInObject );

static void *
openglLightCopy( void *dstObject,
                 const void *srcObject,
                 RwInt32 offsetInObject,
                 RwInt32 sizeInObject );

static void *
openglLightDestructor( void *object,
                       RwInt32 offsetInObject,
                       RwInt32 sizeInObject );

static void
openglApplyDirectionalLight( const RpLight *light,
                             const RwUInt32 lightIdx );

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))
static void
openglEnableLight( const RwInt32 lightIdx );

static void
openglDisableLight( const RwInt32 lightIdx );
#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */



/* =====================================================================
 *  Global function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: _rpOpenGLLightPluginAttach
 *
 *  Purpose : Attach the OpenGL lighting plugin.
 *
 *  On entry: Nothing.
 *
 *  On exit : RwBool, TRUE if successfully attached the plugin, FALSE if not.
 * --------------------------------------------------------------------- */
RwBool
_rpOpenGLLightPluginAttach( void )
{
    RWFUNCTION( RWSTRING( "_rpOpenGLLightPluginAttach" ) );

    rpOpenGLLightOffset = RpLightRegisterPlugin( sizeof(_rpOpenGLLightData),
                                                 rwID_LIGHTMODULE,
                                                 openglLightConstructor,
                                                 openglLightDestructor,
                                                 openglLightCopy );

    if ( rpOpenGLLightOffset < 0 )
    {
        RWRETURN( FALSE );
    }

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLLightsOpen
 *
 *  Purpose : Open the OpenGL lights module.
 *
 *  On entry: Nothing.
 *
 *  On exit : RwBool, TRUE if successfully opened the module, FALSE if not.
 * --------------------------------------------------------------------- */
RwBool
_rwOpenGLLightsOpen( void )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLLightsOpen" ) );

    glGetIntegerv( GL_MAX_LIGHTS, (GLint *)&openglLightsMaxLights );

    openglPrevNumLightsApplied = 0;

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLLightsClose
 *
 *  Purpose : Close the OpenGL lights module.
 *
 *  On entry: Nothing.
 *
 *  On exit : RwBool, TRUE if successfully opened the module, FALSE if not.
 * --------------------------------------------------------------------- */
RwBool
_rwOpenGLLightsClose( void )
{
    RWFUNCTION( RWSTRING( "_rwOpenGLLightsClose" ) );

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLLightsEnable
 *
 *  Purpose :
 *
 *  On entry: enableLighting - Constant RwBool, containing TRUE to enable
 *                             lighting, FALSE to disable.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
void
_rwOpenGLLightsEnable( const RwBool enableLighting )
{
    RwUInt32    idx;


    RWFUNCTION( RWSTRING( "_rwOpenGLLightsEnable" ) );

    if ( FALSE != enableLighting )
    {
        if ( FALSE != openglUseAmbientLight )
        {
            /* material ambient modulates with this */
            glLightModelfv( GL_LIGHT_MODEL_AMBIENT, (GLfloat *)&openglOpaqueWhite );
        }
        else
        {
            glLightModelfv( GL_LIGHT_MODEL_AMBIENT, (GLfloat *)&openglOpaqueBlack );
        }

        RwOpenGLEnable( rwGL_LIGHTING );
    }
    else
    {
        RwOpenGLDisable( rwGL_LIGHTING );
    }

    /* disable all unapplied lights that were enabled last time */
    for ( idx = openglNumLightsApplied; idx < openglPrevNumLightsApplied; idx += 1 )
    {
        openglDisableLight( idx );
    }

    openglPrevNumLightsApplied = openglNumLightsApplied;

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLLightsGlobalLightsEnable
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwBool
_rwOpenGLLightsGlobalLightsEnable(const RpLightFlag lightFlags,
                                  const RwBool initDirectionalLights)
{
    RwBool      lightingEnabled;


    RWFUNCTION( RWSTRING( "_rwOpenGLLightsGlobalLightsEnable" ) );

    /* assume lighting isn't enabled */
    lightingEnabled = FALSE;

    /* initialize the ambient light */
    openglUseAmbientLight = FALSE;
    openglAmbientLight.red   = 0.0f;
    openglAmbientLight.green = 0.0f;
    openglAmbientLight.blue  = 0.0f;
    openglAmbientLight.alpha = 1.0f;

    openglNumLightsApplied = 0;

    if (initDirectionalLights)
    {
        RpWorld     *world;

        RwLLLink    *curDirLight;

        RwLLLink    *endDirLight;


        world = (RpWorld *)RWSRCGLOBAL(curWorld);
        RWASSERT( NULL != world );

        curDirLight = rwLinkListGetFirstLLLink( &(world->directionalLightList) );
        endDirLight = rwLinkListGetTerminator( &(world->directionalLightList) );

        while ( curDirLight != endDirLight )
        {
            RpLight *light;


            light = rwLLLinkGetData( curDirLight, RpLight, inWorld );

            /* light may actually be a dummyTie from an enclosing ForAll */
            if ( (NULL != light) &&
                 (0 != rwObjectTestFlags(light, lightFlags)) )
            {
                if ( rpLIGHTDIRECTIONAL == RpLightGetType(light) )
                {
                    /* directional light */
                    if ( openglNumLightsApplied < openglLightsMaxLights )
                    {
                        openglApplyDirectionalLight( light, openglNumLightsApplied );

                        openglNumLightsApplied += 1;
                    }
#if defined(RWDEBUG)
                    else
                    {
                        RWMESSAGE( (RWSTRING( "Requests for more lights than the maximum allowed (%d) causes the extra lights to be ignored. "), openglLightsMaxLights ) );
                    }
#endif /* defined(RWDEBUG) */
                }
                else
                {
                    const RwRGBAReal    *ambLightColor;


                    /* ambient light */
                    ambLightColor = (const RwRGBAReal *)RpLightGetColor( light );

                    /* accumulate ambient light color - not alpha */
                    openglAmbientLight.red   += ambLightColor->red;
                    openglAmbientLight.green += ambLightColor->green;
                    openglAmbientLight.blue  += ambLightColor->blue;

                    openglUseAmbientLight = TRUE;
                }

                lightingEnabled = TRUE;
            }

            /* next RpLight */
            curDirLight = rwLLLinkGetNext( curDirLight );
        };
    }

    RWRETURN( lightingEnabled );
}


/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLLightsLocalLightEnable
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
RwBool
_rwOpenGLLightsLocalLightEnable( const RpLight * const localLight )
{
    RwReal                      lightRadius;


    RWFUNCTION( RWSTRING( "_rwOpenGLLightsLocalLightEnable" ) );

    RWASSERT( NULL != localLight );

    if ( openglNumLightsApplied >= openglLightsMaxLights )
    {
#if defined(RWDEBUG)
        RWMESSAGE( (RWSTRING( "Requests for more lights than the maximum allowed (%d) causes the extra lights to be ignored. "), openglLightsMaxLights ) );
#endif /* defined(RWDEBUG) */

        RWRETURN( TRUE );
    }

    lightRadius = RpLightGetRadius( localLight );

    /* no sense in using a light with zero radius */
    if ( 0.0f != lightRadius )
    {
        const GLenum                glLightIdx = GL_LIGHT0 + openglNumLightsApplied;

        const RwReal                invLightRadius = 1.0f / lightRadius;

        const RwRGBAReal            *lightColor;

        RwFrame                     *lightFrame;

        RwMatrix                    *lightLTM;

        const RwV3d                 *lightLTMPos;

        GLfloat                     lightPos[4];

        const _rpOpenGLLightData    *lightData;

        RpOpenGLLightAttentuation  attenuationParams;


        /* set the light's color */
        lightColor = RpLightGetColor( localLight );
        RWASSERT( NULL != lightColor );
        glLightfv( glLightIdx, GL_DIFFUSE, (const GLfloat *)lightColor );

        /* light's position */
        lightFrame = (RwFrame *)RpLightGetFrame( localLight );
        RWASSERT( NULL != lightFrame );

        lightLTM = (RwMatrix *)RwFrameGetLTM( lightFrame );
        RWASSERT( NULL != lightLTM );

        lightLTMPos = (const RwV3d *)RwMatrixGetPos( lightLTM );
        RWASSERT( NULL != lightLTMPos );

        lightPos[0] = lightLTMPos->x;
        lightPos[1] = lightLTMPos->y;
        lightPos[2] = lightLTMPos->z;
        lightPos[3] = 1.0f;
        glLightfv( glLightIdx, GL_POSITION, lightPos );

        /* attenuation parameters */
        lightData = OPENGLLIGHTDATAFROMCONSTLIGHT( localLight );
        RWASSERT( NULL != lightData );

        attenuationParams.constant = lightData->attenuationParams.constant;
        attenuationParams.linear = lightData->attenuationParams.linear * invLightRadius;
        attenuationParams.quadratic = lightData->attenuationParams.quadratic * invLightRadius * invLightRadius;

        glLightf( glLightIdx, GL_CONSTANT_ATTENUATION, attenuationParams.constant );
        glLightf( glLightIdx, GL_LINEAR_ATTENUATION, attenuationParams.linear );
        glLightf( glLightIdx, GL_QUADRATIC_ATTENUATION, attenuationParams.quadratic );

        switch ( RpLightGetType(localLight) )
        {
        case rpLIGHTPOINT:
            {
                /* reset the cutoff */
                glLightf( glLightIdx, GL_SPOT_CUTOFF, 180.0f );
            }
            break;

        case rpLIGHTSPOT:
            {
                const RwV3d *lightLTMAt;

                GLfloat     lightSpotDir[3];


                lightLTMAt = (const RwV3d *)RwMatrixGetAt( lightLTM );
                RWASSERT( NULL != lightLTMAt );

                lightSpotDir[0] = lightLTMAt->x;
                lightSpotDir[1] = lightLTMAt->y;
                lightSpotDir[2] = lightLTMAt->z;

                glLightfv( glLightIdx, GL_SPOT_DIRECTION, lightSpotDir );

                glLightf( glLightIdx, GL_SPOT_CUTOFF,
                          (GLfloat)(RpLightGetConeAngle(localLight) * RAD_2_DEG) );

                glLightf( glLightIdx, GL_SPOT_EXPONENT, 0.0f );
            }
            break;

        case rpLIGHTSPOTSOFT:
            {
                const RwV3d *lightLTMAt;

                GLfloat     lightSpotDir[3];


                lightLTMAt = (const RwV3d *)RwMatrixGetAt( lightLTM );
                RWASSERT( NULL != lightLTMAt );

                lightSpotDir[0] = lightLTMAt->x;
                lightSpotDir[1] = lightLTMAt->y;
                lightSpotDir[2] = lightLTMAt->z;

                glLightfv( glLightIdx, GL_SPOT_DIRECTION, lightSpotDir );

                glLightf( glLightIdx, GL_SPOT_CUTOFF,
                          (GLfloat)(RpLightGetConeAngle(localLight) * RAD_2_DEG) );

                glLightf( glLightIdx, GL_SPOT_EXPONENT, lightData->softSpotExponent );
            }
            break;

        default:
            {
                RWRETURN( FALSE );
            }
        }

        openglEnableLight( openglNumLightsApplied );
        openglNumLightsApplied += 1;
    }

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLLightsAddAmbientLight
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
void
_rwOpenGLLightsAddAmbientLight(const RpLight * const ambLight)
{
    const RwRGBAReal    *ambLightColor;


    RWFUNCTION(RWSTRING("_rwOpenGLLightsAddAmbientLight"));

    RWASSERT(ambLight);

    /* ambient light */
    ambLightColor = (const RwRGBAReal *)RpLightGetColor(ambLight);

    /* accumulate ambient light color - not alpha */
    openglAmbientLight.red   += ambLightColor->red;
    openglAmbientLight.green += ambLightColor->green;
    openglAmbientLight.blue  += ambLightColor->blue;

    openglUseAmbientLight = TRUE;

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLLightsAddDirectionalLight
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
void
_rwOpenGLLightsAddDirectionalLight(const RpLight * const dirLight)
{
    RWFUNCTION(RWSTRING("_rwOpenGLLightsAddDirectionalLight"));

    RWASSERT(dirLight);

    openglApplyDirectionalLight(dirLight,openglNumLightsApplied);
    openglNumLightsApplied += 1;

    RWRETURNVOID();
}


/**
 * \ingroup worldextensionsenhancementsopengl
 * \ref RpOpenGLWorldSetMaterialProperties is used to set the OpenGL material
 * properties from an \ref RpMaterial. This function should only be called
 * when lighting is enabled.
 *
 * The \c GL_COLOR_MATERIAL state is enabled if prelit geometry is detected.
 *
 * The \ref rpworldsub plugin must be attached before using this function.
 *
 * \param materialVoid Constant void pointer to an \ref RpMaterial.
 *
 * \param flags Constant \ref RwUInt32 containing geometry or world flags.
 *              Only \ref rxGEOMETRY_MODULATE and \ref rxGEOMETRY_PRELIT are
 *              of interest to this function.
 *
 */
void
RpOpenGLWorldSetMaterialProperties( const void *materialVoid,
                                    const RwUInt32 flags )
{
    const RpMaterial            *material;

    GLfloat                     diffuse[4];

    GLfloat                     ambient[4];

    const RwSurfaceProperties   *matSurfaceProps;

    const RwRGBA                *matColor;


    RWAPIFUNCTION( RWSTRING( "RpOpenGLWorldSetMaterialProperties" ) );

    RWASSERT( NULL != materialVoid );
    RWASSERT( FALSE != RwOpenGLIsEnabled( rwGL_LIGHTING ) );

    material = (const RpMaterial *)materialVoid;

    matSurfaceProps = RpMaterialGetSurfaceProperties( material );
    RWASSERT( NULL != matSurfaceProps );

    matColor = RpMaterialGetColor( material );
    RWASSERT( NULL != matColor );

    if ( (0 != (flags & rxGEOMETRY_MODULATE)) &&
         (0xFFFFFFFF != *((const RwUInt32 *)matColor)) )
    {
        RwReal  diffuseScale;

        RwReal  ambientScale;


        /* modulate and the material color isn't white */
        diffuseScale = matSurfaceProps->diffuse * COLORSCALAR;

        diffuse[0] = matColor->red * diffuseScale;
        diffuse[1] = matColor->green * diffuseScale;
        diffuse[2] = matColor->blue * diffuseScale;
        diffuse[3] = matColor->alpha * COLORSCALAR;

        ambientScale = matSurfaceProps->ambient * COLORSCALAR;

        ambient[0] = matColor->red * ambientScale *
                        openglAmbientLight.red;
        ambient[1] = matColor->green * ambientScale *
                        openglAmbientLight.green;
        ambient[2] = matColor->blue * ambientScale *
                        openglAmbientLight.blue;
        ambient[3] = matColor->alpha * COLORSCALAR;
    }
    else
    {
        /* don't modulate */
        diffuse[0] = diffuse[1] = diffuse[2] = matSurfaceProps->diffuse;
        diffuse[3] = 1.0f;

        if ( 1.0 != matSurfaceProps->ambient )
        {
            ambient[0] = matSurfaceProps->ambient *
                            openglAmbientLight.red;
            ambient[1] = matSurfaceProps->ambient *
                            openglAmbientLight.green;
            ambient[2] = matSurfaceProps->ambient *
                            openglAmbientLight.blue;
        }
        else
        {
            ambient[0] = openglAmbientLight.red;
            ambient[1] = openglAmbientLight.green;
            ambient[2] = openglAmbientLight.blue;
        }

        ambient[3] = 1.0f;
    }

    glMaterialfv( GL_FRONT, GL_DIFFUSE, diffuse );
    glMaterialfv( GL_FRONT, GL_AMBIENT, ambient );

    /* prelit and lit geometry */
    if ( 0 != (flags & rxGEOMETRY_PRELIT) )
    {
        RwOpenGLEnable( rwGL_COLOR_MATERIAL );
        glColorMaterial( GL_FRONT, GL_EMISSION );
    }
    else
    {
        RwOpenGLDisable( rwGL_COLOR_MATERIAL );
        glMaterialfv( GL_FRONT, GL_EMISSION, openglOpaqueBlack );
    }

    RWRETURNVOID();
}


/**
 * \ingroup rplightopengl
 * \ref RpOpenGLLightSetAttenuationParams sets the attenuation model of the
 * supplied \ref RpLight. The default model specifies constant, linear and quadratic
 * attenuation coefficients to be 1, 0, 5 respectively.
 *
 * The \ref rpworldsub plugin must be attached before using this function.
 *
 * \param voidLight Constant void pointer referring to the \ref RpLight to set the
 *        attenutation parameters of.
 *
 * \param params Pointer to constant \ref RpOpenGLLightAttentuation containing
 *        the attenuation parameters to use.
 *
 * \see \ref RpOpenGLLightAttentuation
 * \see RpOpenGLLightGetAttenuationParams
 * \see RpOpenGLLightGetSoftSpotExponent
 * \see RpOpenGLLightSetSoftSpotExponent
 *
 */
void
RpOpenGLLightSetAttenuationParams( void * const voidLight,
                                   const RpOpenGLLightAttentuation *params )
{
    RpLight             *light;

    _rpOpenGLLightData  *lightData;


    RWAPIFUNCTION( RWSTRING( "RpOpenGLLightSetAttenuationParams" ) );

    RWASSERT( NULL != voidLight );
    RWASSERT( NULL != params );

    light = (RpLight *)voidLight;

    lightData = OPENGLLIGHTDATAFROMLIGHT( light );
    RWASSERT( NULL != lightData );

    lightData->attenuationParams = *params;

    RWRETURNVOID();
}


/**
 * \ingroup rplightopengl
 * \ref RpOpenGLLightGetAttenuationParams gets the attenuation model of the
 * supplied \ref RpLight.
 *
 * The \ref rpworldsub plugin must be attached before using this function.
 *
 * \param voidLight Constant void pointer referring to a constant \ref RpLight
 *        to get the attenutation parameters of.
 *
 * \return RpOpenGLLightAttentuation containing the attenuation parameters
 *         of the \ref RpLight.
 *
 * \see \ref RpOpenGLLightAttentuation
 * \see RpOpenGLLightSetAttenuationParams
 * \see RpOpenGLLightGetSoftSpotExponent
 * \see RpOpenGLLightSetSoftSpotExponent
 *
 */
RpOpenGLLightAttentuation
RpOpenGLLightGetAttenuationParams( const void * const voidLight )
{
    const RpLight               *light;

    const _rpOpenGLLightData    *lightData;


    RWAPIFUNCTION( RWSTRING( "RpOpenGLLightGetAttenuationParams" ) );

    RWASSERT( NULL != voidLight );

    light = (const RpLight *)voidLight;

    lightData = OPENGLLIGHTDATAFROMCONSTLIGHT( light );
    RWASSERT( NULL != lightData );

    RWRETURN( lightData->attenuationParams );
}


/**
 * \ingroup rplightopengl
 * \ref RpOpenGLLightSetSoftSpotExponent sets the exponent for the supplied
 * soft spot \ref RpLight. This function has no effect on other types
 * of light.
 *
 * The \ref rpworldsub plugin must be attached before using this function.
 *
 * \param voidLight Constant void pointer to the soft spot \ref RpLight to
 *        set the exponent of.
 *
 * \param exponent Constant \ref RwReal containing the exponent value.
 *
 * \see RpOpenGLLightGetSoftSpotExponent
 *
 */
void
RpOpenGLLightSetSoftSpotExponent( void * const voidLight,
                                  const RwReal exponent )
{
    RpLight             *light;

    _rpOpenGLLightData  *lightData;


    RWAPIFUNCTION( RWSTRING( "RpOpenGLLightSetSoftSpotExponent" ) );

    RWASSERT( NULL != voidLight );

    light = (RpLight *)voidLight;

    /* early out */
    if ( rpLIGHTSPOTSOFT != RpLightGetType(light) )
    {
        RWRETURNVOID();
    }

    lightData = OPENGLLIGHTDATAFROMLIGHT( light );
    RWASSERT( NULL != lightData );

    lightData->softSpotExponent = exponent;

    RWRETURNVOID();
}


/**
 * \ingroup rplightopengl
 * \ref RpOpenGLLightGetSoftSpotExponent gets the exponent for the supplied
 * soft spot \ref RpLight. This function returns 0.0 for other types
 * of light.
 *
 * The \ref rpworldsub plugin must be attached before using this function.
 *
 * \param voidLight Constant void pointer referring to the constant soft
 *        spot \ref RpLight to get the exponent of.
 *
 * \return \ref RwReal containing the exponent of the soft spot \ref RpLight.
 *
 * \see RpOpenGLLightSetSoftSpotExponent
 *
 */
RwReal
RpOpenGLLightGetSoftSpotExponent( const void * const voidLight )
{
    const RpLight               *light;

    const _rpOpenGLLightData    *lightData;


    RWAPIFUNCTION( RWSTRING( "RpOpenGLLightGetSoftSpotExponent" ) );

    RWASSERT( NULL != voidLight );

    light = (const RpLight *)voidLight;

    /* early out */
    if ( rpLIGHTSPOTSOFT != RpLightGetType(light) )
    {
        RWRETURN( 0.0f );
    }

    lightData = OPENGLLIGHTDATAFROMCONSTLIGHT( light );
    RWASSERT( NULL != lightData );

    RWRETURN( lightData->softSpotExponent );
}


/* =====================================================================
 *  Static function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: openglLightConstructor
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void *
openglLightConstructor( void *object,
                        RwInt32 offsetInObject __RWUNUSED__,
                        RwInt32 sizeInObject __RWUNUSED__ )
{
    RWFUNCTION( RWSTRING( "openglLightConstructor" ) );

    if ( NULL != object )
    {
        _rpOpenGLLightData  *lightData;


        lightData = OPENGLLIGHTDATAFROMLIGHT( (RpLight *)object );
        RWASSERT( NULL != lightData );

        /* default attenuation */
        lightData->attenuationParams.constant = 1.0f;
        lightData->attenuationParams.linear = 0.0f;
        lightData->attenuationParams.quadratic = 5.0f;

        /* soft spot light */
        lightData->softSpotExponent = 5.0f;

        RWRETURN( object );
    }

    RWRETURN( NULL );
}


/* ---------------------------------------------------------------------
 *  Function: openglLightCopy
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void *
openglLightCopy( void *dstObject,
                 const void *srcObject,
                 RwInt32 offsetInObject __RWUNUSED__,
                 RwInt32 sizeInObject __RWUNUSED__ )
{
    RWFUNCTION( RWSTRING( "openglLightCopy" ) );

    if ( (NULL != srcObject) && (NULL != dstObject) )
    {
        const _rpOpenGLLightData    *srcLightData;

        _rpOpenGLLightData          *dstLightData;


        srcLightData = OPENGLLIGHTDATAFROMCONSTLIGHT( (const RpLight *)srcObject );
        RWASSERT( NULL != srcLightData );

        dstLightData = OPENGLLIGHTDATAFROMLIGHT((RpLight *)dstObject);

        *dstLightData = *srcLightData;

        RWRETURN( dstObject );
    }

    RWRETURN( NULL );
}


/* ---------------------------------------------------------------------
 *  Function: openglLightDestructor
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void *
openglLightDestructor( void *object,
                       RwInt32 offsetInObject __RWUNUSED__,
                       RwInt32 sizeInObject __RWUNUSED__ )
{
    RWFUNCTION( RWSTRING( "openglLightDestructor" ) );

    RWRETURN( object );
}


/* ---------------------------------------------------------------------
 *  Function: openglApplyDirectionalLight
 *
 *  Purpose :
 *
 *  On entry:
 *
 *  On exit :
 * --------------------------------------------------------------------- */
static void
openglApplyDirectionalLight( const RpLight *light,
                             const RwUInt32 lightIdx )
{
    RwFrame             *lightFrame;

    RwMatrix            *lightLTM;

    const RwV3d         *lightAt;

    GLfloat             lightDirection[4];

    const RwRGBAReal    *lightColor;


    RWFUNCTION( RWSTRING( "openglApplyDirectionalLight" ) );

    /* set the direction of the light */
    lightFrame = (RwFrame *)RpLightGetFrame( light );
    RWASSERT( NULL != lightFrame );

    lightLTM = (RwMatrix *)RwFrameGetLTM( lightFrame );
    RWASSERT( NULL != lightLTM );

    lightAt = RwMatrixGetAt( lightLTM );
    RWASSERT( NULL != lightAt );

    lightDirection[0] = -lightAt->x;
    lightDirection[1] = -lightAt->y;
    lightDirection[2] = -lightAt->z;
    lightDirection[3] = 0.0f; /* 0.0 = direction */

    glLightfv( GL_LIGHT0 + lightIdx, GL_POSITION, lightDirection );

    /* set the color of the light */
    glLightfv( GL_LIGHT0 + lightIdx, GL_AMBIENT, openglOpaqueBlack );

    lightColor = RpLightGetColor(light);
    RWASSERT( NULL != lightColor );
    glLightfv( GL_LIGHT0 + lightIdx, GL_DIFFUSE, (const GLfloat *)lightColor );

    /* reset the cutoff */
    glLightf( GL_LIGHT0 + lightIdx, GL_SPOT_CUTOFF, 180.0f );

    /* enable the light */
    openglEnableLight( lightIdx );

    RWRETURNVOID();
}


#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))

/* ---------------------------------------------------------------------
 *  Function: openglEnableLight
 *
 *  Purpose : Enable one of OpenGL's lights. The enabled status of each
 *            light is cached by RenderWare Graphics to avoid unnecessary
 *            state changes.
 *
 *  On entry: lightIdx - Constant RwInt32 containing the index between 0
 *                       and GL_MAX_LIGHTS of the light to enable.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
static void
openglEnableLight( const RwInt32 lightIdx )
{
    RWFUNCTION( RWSTRING( "openglEnableLight" ) );

    RWASSERT( lightIdx >= 0 );
    RWASSERT( (RwUInt32)lightIdx < openglLightsMaxLights );

    openglEnableLightMacro( lightIdx );

    RWRETURNVOID();
}


/* ---------------------------------------------------------------------
 *  Function: openglDisableLight
 *
 *  Purpose : Disable one of OpenGL's lights. The enabled status of each
 *            light is cached by RenderWare Graphics to avoid unnecessary
 *            state changes.
 *
 *  On entry: lightIdx - Constant RwInt32 containing the index between 0
 *                       and GL_MAX_LIGHTS of the light to disable.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
static void
openglDisableLight( const RwInt32 lightIdx )
{
    RWFUNCTION( RWSTRING( "openglDisableLight" ) );

    RWASSERT( lightIdx >= 0 );
    RWASSERT( (RwUInt32)lightIdx < openglLightsMaxLights );

    openglDisableLightMacro( lightIdx );

    RWRETURNVOID();
}

#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */
