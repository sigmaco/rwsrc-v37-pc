/***********************************************************************
 *
 * Module:  bavagl.c
 *
 * Purpose: OpenGL Vertex Array
 *
 ***********************************************************************/

/* =====================================================================
 *  Includes
 * ===================================================================== */
#if defined(WIN32)
#include <windows.h>
#endif /* defined(WIN32) */
#include <gl/gl.h>

#include "batypes.h"
#include "barwtyp.h"

#include "baogl.h"
#include "drvmodel.h"
#include "drvfns.h"
#include "barstate.h"

#include "bavagl.h"


/* =====================================================================
 *  Defines
 * ===================================================================== */


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */


/* =====================================================================
 *  Static variables
 * ===================================================================== */
/**
 * \ingroup rwenginevertexarraysopengl
 * \page vertexarrayoverview Overview
 *
 * OpenGL RenderWare Graphics supports vertex arrays for system memory,
 * NVIDIA vertex array range (VAR), and ATI vertex array object (VAO)
 * architectures.
 *
 * VAR and VAO provide server side storage for vertex data and are used
 * automatically if OpenGL RenderWare Graphics detects support for them.
 *
 * To improve performance, an internal vertex array state cache is maintained
 * to avoid enabling or disabling client states except where necessary. A
 * wrapper is provided to maintain this cache and it is recommended that this
 * be used in custom render pipelines.
 *
 * The wrapper contains two sets of similar functions, due to the similar, but
 * not exactly the same, functionality between VAR and VAO. VAO functions are
 * postfixed with the characters 'ATI'. There are also corresponding client
 * state disabling functions that provide a fast path over their siblings.
 *
 * \par Positions
 * \li \ref RwOpenGLVASetPosition
 * \li \ref RwOpenGLVASetPositionATI
 * \li \ref RwOpenGLVADisablePosition
 *
 * \par Normals
 * \li \ref RwOpenGLVASetNormal
 * \li \ref RwOpenGLVASetNormalATI
 * \li \ref RwOpenGLVADisableNormal
 *
 * \par Colors (prelight)
 * \li \ref RwOpenGLVASetColor
 * \li \ref RwOpenGLVASetColorATI
 * \li \ref RwOpenGLVADisableColor
 *
 * \par Single texture unit texture coordinates
 * \li \ref RwOpenGLVASetTexCoord
 * \li \ref RwOpenGLVASetTexCoordATI
 *
 * \par Multiple texture unit texture coordinates
 * \li \ref RwOpenGLVASetMultiTexCoord
 * \li \ref RwOpenGLVASetMultiTexCoordATI
 * \li \ref RwOpenGLVADisableTexCoord
 *
 * Note that the texture coordinate functions above will automatically set
 * the client active texture unit. As to be expected, the multitexturing
 * functions assume that OpenGL 1.2 and/or the multitexturing extension is
 * supported, so code paths that call these functions should not be used
 * on non-multitexturing systems, or unexpected results may occur.
 *
 * Note that the \e client active texture unit is set independently of the active
 * texture unit, used to bind textures etc. For the latter, see
 * \ref RwOpenGLSetActiveTextureUnit.
 *
 * OpenGL RenderWare Graphics provides no wrapper for drawing vertex data so
 * the usual \c glDrawElements or \c glDrawArrays should be used.
 */


/* =====================================================================
 *  Global variables
 * ===================================================================== */
RwBool  _rwOpenGLVAPositionEnabled;
RwBool  _rwOpenGLVANormalEnabled;
RwBool  _rwOpenGLVAColorEnabled;
RwBool  *_rwOpenGLVATexCoordEnabled;
RwUInt32 _rwOpenGLVATexUnit;

/* =====================================================================
 *  Static function prototypes
 * ===================================================================== */


/* =====================================================================
 *  Global function definitions
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLVAOpen
 *
 *  Purpose : Open the OpenGL vertex array module.
 *
 *  On entry: Nothing.
 *
 *  On exit : RwBool, TRUE if successfully opened the module, FALSE otherwise.
 * --------------------------------------------------------------------- */
RwBool
_rwOpenGLVAOpen( void )
{
    RwUInt8 texUnit;


    RWFUNCTION( RWSTRING( "_rwOpenGLVAOpen" ) );

    /* initialize state cache */
    if ( GL_FALSE == glIsEnabled( GL_VERTEX_ARRAY ) )
    {
        _rwOpenGLVAPositionEnabled = FALSE;
    }
    else
    {
        _rwOpenGLVAPositionEnabled = TRUE;
    }

    if ( GL_FALSE == glIsEnabled( GL_NORMAL_ARRAY ) )
    {
        _rwOpenGLVANormalEnabled = FALSE;
    }
    else
    {
        _rwOpenGLVANormalEnabled = TRUE;
    }

    if ( GL_FALSE == glIsEnabled( GL_COLOR_ARRAY ) )
    {
        _rwOpenGLVAColorEnabled = FALSE;
    }
    else
    {
        _rwOpenGLVAColorEnabled = TRUE;
    }

    _rwOpenGLVATexCoordEnabled = (RwBool *)RwDriverMalloc( sizeof(RwBool) *
                                                           _rwOpenGLExt.MaxTextureUnits,
                                                           rwID_DRIVERMODULE | rwMEMHINTDUR_GLOBAL);
    if ( NULL == _rwOpenGLVATexCoordEnabled )
    {
        RWRETURN( FALSE );
    }

    if ( 1 == _rwOpenGLExt.MaxTextureUnits )
    {
        _rwOpenGLVATexUnit = 0;
        if ( GL_FALSE == glIsEnabled( GL_TEXTURE_COORD_ARRAY ) )
        {
            _rwOpenGLVATexCoordEnabled[0] = FALSE;
        }
        else
        {
            _rwOpenGLVATexCoordEnabled[0] = TRUE;
        }
    }
    else
    {
        _rwOpenGLVATexUnit = -1;
        for ( texUnit = 0; texUnit < _rwOpenGLExt.MaxTextureUnits; texUnit += 1 )
        {
            RwOpenGLVASetTexUnitMacro( texUnit );
            if ( GL_FALSE == glIsEnabled( GL_TEXTURE_COORD_ARRAY ) )
            {
                _rwOpenGLVATexCoordEnabled[texUnit] = FALSE;
            }
            else
            {
                _rwOpenGLVATexCoordEnabled[texUnit] = TRUE;
            }
        }
        RwOpenGLVASetTexUnitMacro( 0 );
    }

    RWRETURN( TRUE );
}


/* ---------------------------------------------------------------------
 *  Function: _rwOpenGLVAClose
 *
 *  Purpose : Close the OpenGL vertex array module.
 *
 *  On entry: Nothing.
 *
 *  On exit : Nothing.
 * --------------------------------------------------------------------- */
void
_rwOpenGLVAClose( void )
{
    RwUInt8 texUnit;


    RWFUNCTION( RWSTRING( "_rwOpenGLVAClose" ) );

    if ( FALSE != _rwOpenGLVANormalEnabled )
    {
        glDisableClientState( GL_NORMAL_ARRAY );
    }

    if ( FALSE != _rwOpenGLVAColorEnabled )
    {
        glDisableClientState( GL_COLOR_ARRAY );
    }

    if ( 1 == _rwOpenGLExt.MaxTextureUnits )
    {
        if ( FALSE != _rwOpenGLVATexCoordEnabled[0] )
        {
            glDisableClientState( GL_TEXTURE_COORD_ARRAY );
        }
    }
    else
    {
        for ( texUnit = 0; texUnit < _rwOpenGLExt.MaxTextureUnits; texUnit += 1 )
        {
            if ( FALSE != _rwOpenGLVATexCoordEnabled[texUnit] )
            {
                RwOpenGLVASetTexUnitMacro( texUnit );
                glDisableClientState( GL_TEXTURE_COORD_ARRAY );
            }
        }
        RwOpenGLVASetTexUnitMacro( 0 );
    }

    RwDriverFree( _rwOpenGLVATexCoordEnabled );
    _rwOpenGLVATexCoordEnabled = (RwBool *)NULL;

    glDisableClientState( GL_VERTEX_ARRAY );

    RWRETURNVOID();
}


#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))

/**
 * \ingroup rwenginevertexarraysopengl
 * \ref RwOpenGLVASetPosition is used to set the vertex array position data for
 * OpenGL's \c GL_VERTEX_ARRAY. This function is suitable for use with system
 * memory or NVIDIA Vertex Array Range vertex arrays.
 *
 * The \c GL_VERTEX_ARRAY client state is enabled (if it is not already).
 *
 * The enable state of \c GL_VERTEX_ARRAY is cached internally by RenderWare
 * Graphics to avoid unnecessary state changes.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param numComponents Constant \ref RwUInt32 containing the number of
 *                      components in the position, minimum of 2, maximum of 4.
 *
 * \param baseType Constant \ref RwInt32 mapped onto one of the OpenGL types:
 *                 \c GL_SHORT, \c GL_INT, \c GL_FLOAT or \c GL_DOUBLE.
 *
 * \param stride Constant \ref RwUInt32 containing the vertex stride. Note that
 *               0 means \e numComponents * sizeof(\e baseType).
 *
 * \param memAddr Void pointer, such that \e memAddr refers to the \e start of
 *                the position data.
 *
 * \see \ref RwOpenGLVASetPositionATI
 * \see \ref RwOpenGLVADisablePosition
 */
void
RwOpenGLVASetPosition( const RwUInt32 numComponents,
                       const RwInt32 baseType,
                       const RwUInt32 stride,
                       const void *memAddr )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLVASetPosition" ) );

    RWASSERT( numComponents >= 2 );
    RWASSERT( numComponents <= 4 );
    RWASSERT( (GL_SHORT == baseType)          ||
              (GL_INT == baseType)            ||
              (GL_FLOAT == baseType)          ||
              (GL_DOUBLE == baseType) );

    RwOpenGLVASetPositionMacro( numComponents,
                                baseType,
                                stride,
                                memAddr );

    RWRETURNVOID();
}


/**
 * \ingroup rwenginevertexarraysopengl
 * \ref RwOpenGLVASetPositionATI is used to set the vertex array position data
 * for OpenGL's \c GL_VERTEX_ARRAY. This function is suitable for use with ATI
 * vertex array objects only.
 *
 * The \c GL_VERTEX_ARRAY client state is enabled (if it is not already).
 *
 * The enable state of \c GL_VERTEX_ARRAY is cached internally by RenderWare
 * Graphics to avoid unnecessary state changes.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param numComponents Constant \ref RwUInt32 containing the number of
 *                      components in the position, minimum of 2, maximum of 4.
 *
 * \param baseType Constant \ref RwInt32 mapped onto one of the OpenGL types:
 *                 \c GL_SHORT, \c GL_INT, \c GL_FLOAT or \c GL_DOUBLE.
 *
 * \param stride Constant \ref RwUInt32 containing the vertex stride. Note that
 *               0 means \e numComponents * sizeof(\e baseType).
 *
 * \param vaoName Constant \ref RwUInt32 containing the OpenGL assigned vertex
 *                array object (VAO) name that contains the position data.
 *
 * \param offset Void pointer, such that \e offset contains the offset, in
 *               bytes, in the VAO at which the position data begins.
 *
 * \see \ref RwOpenGLVASetPosition
 * \see \ref RwOpenGLVADisablePosition
 */
void
RwOpenGLVASetPositionATI( const RwUInt32 numComponents,
                          const RwInt32 baseType,
                          const RwUInt32 stride,
                          const RwUInt32 vaoName,
                          const void *offset )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLVASetPositionATI" ) );

    RWASSERT( numComponents >= 2 );
    RWASSERT( numComponents <= 4 );
    RWASSERT( (GL_SHORT == baseType)          ||
              (GL_INT == baseType)            ||
              (GL_FLOAT == baseType)          ||
              (GL_DOUBLE == baseType) );
    RWASSERT( NULL != _rwOpenGLExt.IsObjectBufferATI );
    RWASSERT( GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( vaoName ) );

    RwOpenGLVASetPositionATIMacro( numComponents,
                                   baseType,
                                   stride,
                                   vaoName,
                                   offset );

    RWRETURNVOID();
}


/**
 * \ingroup rwenginevertexarraysopengl
 * \ref RwOpenGLVASetNormal is used to set the vertex array normal data for
 * OpenGL's \c GL_NORMAL_ARRAY. This function is suitable for use with system
 * memory or NVIDIA Vertex Array Range vertex arrays.
 *
 * If \e enableTest evaluates to TRUE, then the \c GL_NORMAL_ARRAY client state
 * is enabled (if it is not already), and the normal data set. If \e enableTest
 * evaluates to FALSE, \c GL_NORMAL_ARRAY is disabled (if it is not already)
 * and all other arguments are ignored.
 *
 * The enable state of \c GL_NORMAL_ARRAY is cached internally by RenderWare
 * Graphics to avoid unnecessary state changes.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param enableTest Constant \ref RwBool containing TRUE to enable normals and
 *                   set the specified data, or FALSE to disable normals.
 *
 * \param baseType Constant \ref RwInt32 mapped onto one of the OpenGL types:
 *                 \c GL_BYTE, \c GL_SHORT, \c GL_INT, \c GL_FLOAT or
 *                 \c GL_DOUBLE.
 *
 * \param stride Constant \ref RwUInt32 containing the vertex stride. Note that
 *               0 means \e numComponents * sizeof(\e baseType).
 *
 * \param memAddr Void pointer, such that \e memAddr refers to the \e start of
 *                the normal data.
 *
 * \see \ref RwOpenGLVASetNormalATI
 * \see \ref RwOpenGLVADisableNormal
 */
void
RwOpenGLVASetNormal( const RwBool enableTest,
                     const RwInt32 baseType,
                     const RwUInt32 stride,
                     const void *memAddr )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLVASetNormal" ) );

    RWASSERT( (GL_BYTE == baseType)  ||
              (GL_SHORT == baseType) ||
              (GL_INT == baseType)   ||
              (GL_FLOAT == baseType) ||
              (GL_DOUBLE == baseType) );

    RwOpenGLVASetNormalMacro( enableTest,
                              baseType,
                              stride,
                              memAddr );

    RWRETURNVOID();
}


/**
 * \ingroup rwenginevertexarraysopengl
 * \ref RwOpenGLVASetNormalATI is used to set the vertex array normal data for
 * OpenGL's \c GL_NORMAL_ARRAY. This function is suitable for use with ATI
 * vertex array objects only.
 *
 * If \e enableTest evaluates to TRUE, then the \c GL_NORMAL_ARRAY client state
 * is enabled (if it is not already), and the normal data set. If \e enableTest
 * evaluates to FALSE, \c GL_NORMAL_ARRAY is disabled (if it is not already)
 * and all other arguments are ignored.
 *
 * The enable state of \c GL_NORMAL_ARRAY is cached internally by RenderWare
 * Graphics to avoid unnecessary state changes.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param enableTest Constant \ref RwBool containing TRUE to enable normals and
 *                   set the specified data, or FALSE to disable normals.
 *
 * \param baseType Constant \ref RwInt32 mapped onto one of the OpenGL types:
 *                 \c GL_BYTE, \c GL_SHORT, \c GL_INT, \c GL_FLOAT or
 *                 \c GL_DOUBLE.
 *
 * \param stride Constant \ref RwUInt32 containing the vertex stride. Note that
 *               0 means \e numComponents * sizeof(\e baseType).
 *
 * \param vaoName Constant \ref RwUInt32 containing the OpenGL assigned vertex
 *                array object (VAO) name that contains the normal data.
 *
 * \param offset Void pointer, such that \e offset contains the offset, in
 *               bytes, in the VAO at which the normal data begins.
 *
 * \see \ref RwOpenGLVASetNormal
 * \see \ref RwOpenGLVADisableNormal
 */
void
RwOpenGLVASetNormalATI( const RwBool enableTest,
                        const RwInt32 baseType,
                        const RwUInt32 stride,
                        const RwUInt32 vaoName,
                        const void *offset )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLVASetNormalATI" ) );

    RWASSERT( (GL_BYTE == baseType)  ||
              (GL_SHORT == baseType) ||
              (GL_INT == baseType)   ||
              (GL_FLOAT == baseType) ||
              (GL_DOUBLE == baseType) );
    RWASSERT( NULL != _rwOpenGLExt.IsObjectBufferATI );
    RWASSERT( GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( vaoName ) );

    RwOpenGLVASetNormalATIMacro( enableTest,
                                 baseType,
                                 stride,
                                 vaoName,
                                 offset );

    RWRETURNVOID();
}


/**
 * \ingroup rwenginevertexarraysopengl
 * \ref RwOpenGLVASetColor is used to set the vertex array color data for
 * OpenGL's \c GL_COLOR_ARRAY. This function is suitable for use with system
 * memory or NVIDIA Vertex Array Range vertex arrays.
 *
 * If \e enableTest evaluates to TRUE, then the \c GL_COLOR_ARRAY client state
 * is enabled (if it is not already), and the color data set. If \e enableTest
 * evaluates to FALSE, \c GL_COLOR_ARRAY is disabled (if it is not already)
 * and all other arguments are ignored.
 *
 * The enable state of \c GL_COLOR_ARRAY is cached internally by RenderWare
 * Graphics to avoid unnecessary state changes.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param enableTest Constant \ref RwBool containing TRUE to enable colors and
 *                   set the specified data, or FALSE to disable colors.
 *
 * \param numComponents Constant \ref RwUInt32 containing the number of
 *                      components in the color, minimum of 3, maximum of 4.
 *
 * \param baseType Constant \ref RwInt32 mapped onto one of the OpenGL types:
 *                 \c GL_BYTE, \c GL_UNSIGNED_BYTE, \c GL_SHORT,
 *                 \c GL_UNSIGNED_SHORT, \c GL_INT, \c GL_UNSIGNED_INT,
 *                 \c GL_FLOAT or \c GL_DOUBLE.
 *
 * \param stride Constant \ref RwUInt32 containing the vertex stride. Note that
 *               0 means \e numComponents * sizeof(\e baseType).
 *
 * \param memAddr Void pointer, such that \e memAddr refers to the \e start of
 *                the color data.
 *
 * \see \ref RwOpenGLVASetColorATI
 * \see \ref RwOpenGLVADisableColor
 */
void
RwOpenGLVASetColor( const RwBool enableTest,
                    const RwUInt32 numComponents,
                    const RwInt32 baseType,
                    const RwUInt32 stride,
                    const void *memAddr )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLVASetColor" ) );

    RWASSERT( numComponents >= 3 );
    RWASSERT( numComponents <= 4 );
    RWASSERT( (GL_BYTE == baseType)           ||
              (GL_UNSIGNED_BYTE == baseType)  ||
              (GL_SHORT == baseType)          ||
              (GL_UNSIGNED_SHORT == baseType) ||
              (GL_INT == baseType)            ||
              (GL_UNSIGNED_INT == baseType)   ||
              (GL_FLOAT == baseType)          ||
              (GL_DOUBLE == baseType) );

    RwOpenGLVASetColorMacro( enableTest,
                             numComponents,
                             baseType,
                             stride,
                             memAddr );

    RWRETURNVOID();
}


/**
 * \ingroup rwenginevertexarraysopengl
 * \ref RwOpenGLVASetColorATI is used to set the vertex array color data for
 * OpenGL's \c GL_COLOR_ARRAY. This function is suitable for use with ATI
 * vertex array objects only.
 *
 * If \e enableTest evaluates to TRUE, then the \c GL_COLOR_ARRAY client state
 * is enabled (if it is not already), and the color data set. If \e enableTest
 * evaluates to FALSE, \c GL_COLOR_ARRAY is disabled (if it is not already)
 * and all other arguments are ignored.
 *
 * The enable state of \c GL_COLOR_ARRAY is cached internally by RenderWare
 * Graphics to avoid unnecessary state changes.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param enableTest Constant \ref RwBool containing TRUE to enable colors and
 *                   set the specified data, or FALSE to disable colors.
 *
 * \param numComponents Constant \ref RwUInt32 containing the number of
 *                      components in the color, minimum of 3, maximum of 4.
 *
 * \param baseType Constant \ref RwInt32 mapped onto one of the OpenGL types:
 *                 \c GL_BYTE, \c GL_UNSIGNED_BYTE, \c GL_SHORT,
 *                 \c GL_UNSIGNED_SHORT, \c GL_INT, \c GL_UNSIGNED_INT,
 *                 \c GL_FLOAT or \c GL_DOUBLE.
 *
 * \param stride Constant \ref RwUInt32 containing the vertex stride. Note that
 *               0 means \e numComponents * sizeof(\e baseType).
 *
 * \param vaoName Constant \ref RwUInt32 containing the OpenGL assigned vertex
 *                array object (VAO) name that contains the color data.
 *
 * \param offset Void pointer, such that \e offset contains the offset, in
 *               bytes, in the VAO at which the color data begins.
 *
 * \see \ref RwOpenGLVASetColor
 * \see \ref RwOpenGLVADisableColor
 */
void
RwOpenGLVASetColorATI( const RwBool enableTest,
                       const RwUInt32 numComponents,
                       const RwInt32 baseType,
                       const RwUInt32 stride,
                       const RwUInt32 vaoName,
                       const void *offset )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLVASetColorATI" ) );

    RWASSERT( numComponents >= 3 );
    RWASSERT( numComponents <= 4 );
    RWASSERT( (GL_BYTE == baseType)           ||
              (GL_UNSIGNED_BYTE == baseType)  ||
              (GL_SHORT == baseType)          ||
              (GL_UNSIGNED_SHORT == baseType) ||
              (GL_INT == baseType)            ||
              (GL_UNSIGNED_INT == baseType)   ||
              (GL_FLOAT == baseType)          ||
              (GL_DOUBLE == baseType) );
    RWASSERT( NULL != _rwOpenGLExt.IsObjectBufferATI );
    RWASSERT( GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( vaoName ) );

    RwOpenGLVASetColorATIMacro( enableTest,
                                numComponents,
                                baseType,
                                stride,
                                vaoName,
                                offset );

    RWRETURNVOID();
}


/**
 * \ingroup rwenginevertexarraysopengl
 * \ref RwOpenGLVASetTexCoord is used to set the vertex array texture
 * coordinate data for OpenGL's \c GL_TEXTURE_COORD_ARRAY. This function is
 * suitable for use with system memory or NVIDIA Vertex Array Range vertex
 * arrays.
 *
 * \note This function only affects the \e first texture unit. See \ref
 * RwOpenGLVASetMultiTexCoord to set the texture coordinates for other
 * available texture units.
 *
 * If \e enableTest evaluates to TRUE, then the \c GL_TEXTURE_COORD_ARRAY
 * client state is enabled (if it is not already), and the texture coordinate
 * data set. If \e enableTest evaluates to FALSE, \c GL_TEXTURE_COORD_ARRAY is
 * disabled (if it is not already) and all other arguments are ignored.
 *
 * The enable state of \c GL_TEXTURE_COORD_ARRAY is cached internally by
 * RenderWare Graphics to avoid unnecessary state changes.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param enableTest Constant \ref RwBool containing TRUE to enable texture
 *                   coordinates and set the specified data, or FALSE to
 *                   disable texture coordinates.
 *
 * \param numComponents Constant \ref RwUInt32 containing the number of
 *                      components in the texture coordinates, minimum of 1,
 *                      maximum of 4.
 *
 * \param baseType Constant \ref RwInt32 mapped onto one of the OpenGL types:
 *                 \c GL_SHORT, \c GL_INT, \c GL_FLOAT or \c GL_DOUBLE.
 *
 * \param stride Constant \ref RwUInt32 containing the vertex stride. Note that
 *               0 means \e numComponents * sizeof(\e baseType).
 *
 * \param memAddr Void pointer, such that \e memAddr refers to the \e start of
 *                the texture coordinate data.
 *
 * \see \ref RwOpenGLVASetTexCoordATI
 * \see \ref RwOpenGLVASetMultiTexCoord
 * \see \ref RwOpenGLVASetMultiTexCoordATI
 * \see \ref RwOpenGLVADisableTexCoord
 */
void
RwOpenGLVASetTexCoord( const RwBool enableTest,
                       const RwUInt32 numComponents,
                       const RwInt32 baseType,
                       const RwUInt32 stride,
                       const void *memAddr )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLVASetTexCoord" ) );

    RWASSERT( numComponents >= 1 );
    RWASSERT( numComponents <= 4 );
    RWASSERT( (GL_SHORT == baseType)          ||
              (GL_INT == baseType)            ||
              (GL_FLOAT == baseType)          ||
              (GL_DOUBLE == baseType) );

    RwOpenGLVASetTexCoordMacro( enableTest,
                                numComponents,
                                baseType,
                                stride,
                                memAddr );

    RWRETURNVOID();
}


/**
 * \ingroup rwenginevertexarraysopengl
 * \ref RwOpenGLVASetTexCoordATI is used to set the vertex array texture
 * coordinate data for OpenGL's \c GL_TEXTURE_COORD_ARRAY. This function is
 * suitable for use with ATI vertex array objects only.
 *
 * \note This function only affects the \e first texture unit. See \ref
 * RwOpenGLVASetMultiTexCoordATI to set the texture coordinates for other
 * available texture units.
 *
 * If \e enableTest evaluates to TRUE, then the \c GL_TEXTURE_COORD_ARRAY
 * client state is enabled (if it is not already), and the texture coordinate
 * data set. If \e enableTest evaluates to FALSE, \c GL_TEXTURE_COORD_ARRAY is
 * disabled (if it is not already) and all other arguments are ignored.
 *
 * The enable state of \c GL_TEXTURE_COORD_ARRAY is cached internally by
 * RenderWare Graphics to avoid unnecessary state changes.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param enableTest Constant \ref RwBool containing TRUE to enable texture
 *                   coordinates and set the specified data, or FALSE to
 *                   disable texture coordinates.
 *
 * \param numComponents Constant \ref RwUInt32 containing the number of
 *                      components in the texture coordinates, minimum of 1,
 *                      maximum of 4.
 *
 * \param baseType Constant \ref RwInt32 mapped onto one of the OpenGL types:
 *                 \c GL_SHORT, \c GL_INT, \c GL_FLOAT or \c GL_DOUBLE.
 *
 * \param stride Constant \ref RwUInt32 containing the vertex stride. Note that
 *               0 means \e numComponents * sizeof(\e baseType).
 *
 * \param vaoName Constant \ref RwUInt32 containing the OpenGL assigned vertex
 *                array object (VAO) name that contains the texture coordinate
 *                data.
 *
 * \param offset Void pointer, such that \e offset contains the offset, in
 *               bytes, in the VAO at which the texture coordinate data begins.
 *
 * \see \ref RwOpenGLVASetTexCoord
 * \see \ref RwOpenGLVASetMultiTexCoord
 * \see \ref RwOpenGLVASetMultiTexCoordATI
 * \see \ref RwOpenGLVADisableTexCoord
 */
void
RwOpenGLVASetTexCoordATI( const RwBool enableTest,
                          const RwUInt32 numComponents,
                          const RwInt32 baseType,
                          const RwUInt32 stride,
                          const RwUInt32 vaoName,
                          const void *offset )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLVASetTexCoordATI" ) );

    RWASSERT( numComponents >= 1 );
    RWASSERT( numComponents <= 4 );
    RWASSERT( (GL_SHORT == baseType)          ||
              (GL_INT == baseType)            ||
              (GL_FLOAT == baseType)          ||
              (GL_DOUBLE == baseType) );
    RWASSERT( NULL != _rwOpenGLExt.IsObjectBufferATI );
    RWASSERT( GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( vaoName ) );

    RwOpenGLVASetTexCoordATIMacro( enableTest,
                                   numComponents,
                                   baseType,
                                   stride,
                                   vaoName,
                                   offset );

    RWRETURNVOID();
}


/**
 * \ingroup rwenginevertexarraysopengl
 * \ref RwOpenGLVASetMultiTexCoord is used to set the vertex array texture
 * coordinate data for OpenGL's \c GL_TEXTURE_COORD_ARRAY for a specified texture
 * unit. This function is suitable for use with system memory or NVIDIA Vertex
 * Array Range vertex arrays.
 *
 * If \e enableTest evaluates to TRUE, then the \c GL_TEXTURE_COORD_ARRAY
 * client state is enabled (if it is not already), and the texture coordinate
 * data set. If \e enableTest evaluates to FALSE, \c GL_TEXTURE_COORD_ARRAY is
 * disabled (if it is not already) and all other arguments are ignored.
 *
 * The enable state of \c GL_TEXTURE_COORD_ARRAY is cached internally by
 * RenderWare Graphics to avoid unnecessary state changes.
 *
 * \e texUnit must lie in the range [0...max texture units - 1]. (See
 * \ref RwOpenGLExtensions for querying the maximum number of texture units
 * supported.) \ref RwOpenGLVASetMultiTexCoord does \e not change the active
 * texture unit for binding textures, but does change the client active texture
 * unit for specifying vertex arrays.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param enableTest Constant \ref RwBool containing TRUE to enable texture
 *                   coordinates and set the specified data, or FALSE to
 *                   disable texture coordinates.
 *
 * \param texUnit Constant \ref RwInt32 containing the texture unit to
 *                      set the texture coordinates for, in the range
 *                      [0...max texture units - 1].
 *
 * \param numComponents Constant \ref RwUInt32 containing the number of
 *                      components in the texture coordinates, minimum of 1,
 *                      maximum of 4.
 *
 * \param baseType Constant \ref RwInt32 mapped onto one of the OpenGL types:
 *                 \c GL_SHORT, \c GL_INT, \c GL_FLOAT or \c GL_DOUBLE.
 *
 * \param stride Constant \ref RwUInt32 containing the vertex stride. Note that
 *               0 means \e numComponents * sizeof(\e baseType).
 *
 * \param memAddr Void pointer, such that \e memAddr refers to the \e start of
 *                the texture coordinate data.
 *
 * \see \ref RwOpenGLVASetMultiTexCoordATI
 * \see \ref RwOpenGLVADisableTexCoord
 * \see \ref RwOpenGLExtensions
 */
void
RwOpenGLVASetMultiTexCoord( const RwBool enableTest,
                            const RwInt32 texUnit,
                            const RwUInt32 numComponents,
                            const RwInt32 baseType,
                            const RwUInt32 stride,
                            const void *memAddr )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLVASetMultiTexCoord" ) );

    RWASSERT( texUnit >= 0 );
    RWASSERT( (RwUInt32)texUnit < _rwOpenGLExt.MaxTextureUnits );
    RWASSERT( numComponents >= 1 );
    RWASSERT( numComponents <= 4 );
    RWASSERT( (GL_SHORT == baseType)          ||
              (GL_INT == baseType)            ||
              (GL_FLOAT == baseType)          ||
              (GL_DOUBLE == baseType) );

    RwOpenGLVASetMultiTexCoordMacro( enableTest,
                                     texUnit,
                                     numComponents,
                                     baseType,
                                     stride,
                                     memAddr );

    RWRETURNVOID();
}


/**
 * \ingroup rwenginevertexarraysopengl
 * \ref RwOpenGLVASetMultiTexCoordATI is used to set the vertex array texture
 * coordinate data for OpenGL's \c GL_TEXTURE_COORD_ARRAY for a specified
 * texture unit. This function is suitable for use with with ATI vertex array
 * objects only.
 *
 * If \e enableTest evaluates to TRUE, then the \c GL_TEXTURE_COORD_ARRAY
 * client state is enabled (if it is not already), and the texture coordinate
 * data set. If \e enableTest evaluates to FALSE, \c GL_TEXTURE_COORD_ARRAY is
 * disabled (if it is not already) and all other arguments are ignored.
 *
 * The enable state of \c GL_TEXTURE_COORD_ARRAY is cached internally by
 * RenderWare Graphics to avoid unnecessary state changes.
 *
 * \e texUnit must lie in the range [0...max texture units - 1]. (See
 * \ref RwOpenGLExtensions for querying the maximum number of texture units
 * supported.) \ref RwOpenGLVASetMultiTexCoordATI does \e not change the active
 * texture unit for binding textures, but does change the client active texture
 * unit for specifying vertex arrays.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param enableTest Constant \ref RwBool containing TRUE to enable texture
 *                   coordinates and set the specified data, or FALSE to
 *                   disable texture coordinates.
 *
 * \param texUnit Constant \ref RwInt32 containing the texture unit to
 *                      set the texture coordinates for, in the range
 *                      [0...max texture units - 1].
 *
 * \param numComponents Constant \ref RwUInt32 containing the number of
 *                      components in the texture coordinates, minimum of 1,
 *                      maximum of 4.
 *
 * \param baseType Constant \ref RwInt32 mapped onto one of the OpenGL types:
 *                 \c GL_SHORT, \c GL_INT, \c GL_FLOAT or \c GL_DOUBLE.
 *
 * \param stride Constant \ref RwUInt32 containing the vertex stride. Note that
 *               0 means \e numComponents * sizeof(\e baseType).
 *
 * \param vaoName Constant \ref RwUInt32 containing the OpenGL assigned vertex
 *                array object (VAO) name that contains the texture coordinate
 *                data.
 *
 * \param offset Void pointer, such that \e offset contains the offset, in
 *               bytes, in the VAO at which the texture coordinate data begins.
 *
 * \see \ref RwOpenGLVASetMultiTexCoord
 * \see \ref RwOpenGLVADisableTexCoord
 * \see \ref RwOpenGLExtensions
 */
void
RwOpenGLVASetMultiTexCoordATI( const RwBool enableTest,
                               const RwInt32 texUnit,
                               const RwUInt32 numComponents,
                               const RwInt32 baseType,
                               const RwUInt32 stride,
                               const RwUInt32 vaoName,
                               const void *offset )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLVASetMultiTexCoordATI" ) );

    RWASSERT( texUnit >= 0 );
    RWASSERT( (RwUInt32)texUnit < _rwOpenGLExt.MaxTextureUnits );
    RWASSERT( numComponents >= 1 );
    RWASSERT( numComponents <= 4 );
    RWASSERT( (GL_SHORT == baseType)          ||
              (GL_INT == baseType)            ||
              (GL_FLOAT == baseType)          ||
              (GL_DOUBLE == baseType) );
    RWASSERT( NULL != _rwOpenGLExt.IsObjectBufferATI );
    RWASSERT( GL_FALSE != _rwOpenGLExt.IsObjectBufferATI( vaoName ) );

    RwOpenGLVASetMultiTexCoordATIMacro( enableTest,
                                        texUnit,
                                        numComponents,
                                        baseType,
                                        stride,
                                        vaoName,
                                        offset );

    RWRETURNVOID();
}


/**
 * \ingroup rwenginevertexarraysopengl
 * \ref RwOpenGLVADisablePosition is a fast path method of disabling the
 * \c GL_VERTEX_ARRAY client state, maintaining an internal cache to avoid
 * unnecessary state changes.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \see \ref RwOpenGLVASetPosition
 * \see \ref RwOpenGLVASetPositionATI
 */
void
RwOpenGLVADisablePosition( void )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLVADisablePosition" ) );

    RwOpenGLVADisablePositionMacro();

    RWRETURNVOID();
}


/**
 * \ingroup rwenginevertexarraysopengl
 * \ref RwOpenGLVADisableNormal is a fast path method of disabling the
 * \c GL_NORMAL_ARRAY client state, maintaining an internal cache to avoid
 * unnecessary state changes.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \see \ref RwOpenGLVASetNormal
 * \see \ref RwOpenGLVASetNormalATI
 */
void
RwOpenGLVADisableNormal( void )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLVADisableNormal" ) );

    RwOpenGLVADisableNormalMacro();

    RWRETURNVOID();
}


/**
 * \ingroup rwenginevertexarraysopengl
 * \ref RwOpenGLVADisableColor is a fast path method of disabling the
 * \c GL_COLOR_ARRAY client state, maintaining an internal cache to avoid
 * unnecessary state changes.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \see \ref RwOpenGLVASetColor
 * \see \ref RwOpenGLVASetColorATI
 */
void
RwOpenGLVADisableColor( void )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLVADisableColor" ) );

    RwOpenGLVADisableColorMacro();

    RWRETURNVOID();
}


/**
 * \ingroup rwenginevertexarraysopengl
 * \ref RwOpenGLVADisableTexCoord is a fast path method of disabling the
 * \c GL_TEXTURE_COORD_ARRAY client state for the specified texture unit,
 * maintaining an internal cache to avoid unnecessary state changes.
 *
 * \e texUnit must lie in the range [0...\e max \e texture \e units - 1]. (See
 * \ref RwOpenGLExtensions for querying the maximum number of texture units
 * supported.) \ref RwOpenGLVADisableTexCoord does \e not change the active
 * texture unit for binding textures, but does change the client active texture
 * unit for specifying vertex arrays.
 *
 * Note that this function is used for debug purposes only and, for
 * efficiency, is available as a macro for final release versions of an
 * application.
 *
 * \param texUnit Constant \ref RwInt32 containing the zero-based index of the
 *                texture unit to set the texture coordinates for, in the
 *                range [0...\e max \e texture \e units - 1].
 *
 * \see \ref RwOpenGLVASetTexCoord
 * \see \ref RwOpenGLVASetTexCoordATI
 * \see \ref RwOpenGLVASetMultiTexCoord
 * \see \ref RwOpenGLVASetMultiTexCoordATI
 * \see \ref RwOpenGLExtensions
 */
void
RwOpenGLVADisableTexCoord( const RwInt32 texUnit )
{
    RWAPIFUNCTION( RWSTRING( "RwOpenGLVADisableTexCoord" ) );

    RWASSERT( texUnit >= 0 );
    RWASSERT( (RwUInt32)texUnit < _rwOpenGLExt.MaxTextureUnits );

    RwOpenGLVADisableTexCoordMacro( texUnit );

    RWRETURNVOID();
}

#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */


/* =====================================================================
 *  Static function definitions
 * ===================================================================== */
