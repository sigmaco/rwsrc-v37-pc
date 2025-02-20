/***********************************************************************
 *
 * Module:  bavagl.c
 *
 * Purpose: OpenGL Vertex Array
 *
 ***********************************************************************/

#if !defined( BAVAGL_H )
#define BAVAGL_H

/* =====================================================================
 *  Includes
 * ===================================================================== */


/* =====================================================================
 *  Defines
 * ===================================================================== */

/* RWPUBLIC */

/* Macros for GL_VERTEX_ARRAY */
#define RwOpenGLVASetPositionMacro(_numComponents,_baseType,            \
                                   _stride,_memAddr)                    \
MACRO_START                                                             \
{                                                                       \
    if ( FALSE == _rwOpenGLVAPositionEnabled )                          \
    {                                                                   \
        glEnableClientState( GL_VERTEX_ARRAY );                         \
        _rwOpenGLVAPositionEnabled = TRUE;                              \
    }                                                                   \
    glVertexPointer((_numComponents),(_baseType),(_stride),_memAddr);   \
}                                                                       \
MACRO_STOP

#define RwOpenGLVASetPositionATIMacro(_numComponents,_baseType,         \
                                      _stride,_vaoName,_offset)         \
MACRO_START                                                             \
{                                                                       \
    if ( FALSE == _rwOpenGLVAPositionEnabled )                          \
    {                                                                   \
        glEnableClientState( GL_VERTEX_ARRAY );                         \
        _rwOpenGLVAPositionEnabled = TRUE;                              \
    }                                                                   \
    _rwOpenGLExt.ArrayObjectATI( GL_VERTEX_ARRAY,                       \
                                 (_numComponents),                      \
                                 (_baseType),                           \
                                 (_stride),                             \
                                 (_vaoName),                            \
                                 (RwUInt32)(_offset) );                 \
}                                                                       \
MACRO_STOP

/* Macros for GL_NORMAL_ARRAY */
#define RwOpenGLVASetNormalMacro(_enableTest,_baseType,                     \
                                 _stride,_memAddr)                          \
MACRO_START                                                                 \
{                                                                           \
    if ( FALSE != (_enableTest) )                                           \
    {                                                                       \
        if ( FALSE == _rwOpenGLVANormalEnabled )                            \
        {                                                                   \
            glEnableClientState( GL_NORMAL_ARRAY );                         \
            _rwOpenGLVANormalEnabled = TRUE;                                \
        }                                                                   \
        glNormalPointer((_baseType),(_stride),(const GLvoid *)_memAddr);    \
    }                                                                       \
    else if ( FALSE != _rwOpenGLVANormalEnabled )                           \
    {                                                                       \
        glDisableClientState( GL_NORMAL_ARRAY );                            \
        _rwOpenGLVANormalEnabled = FALSE;                                   \
    }                                                                       \
}                                                                           \
MACRO_STOP

    
#define RwOpenGLVASetNormalATIMacro(_enableTest,_baseType,      \
                                    _stride,_vaoName,_offset)   \
MACRO_START                                                     \
{                                                               \
    if ( FALSE != (_enableTest) )                               \
    {                                                           \
        if ( FALSE == _rwOpenGLVANormalEnabled )                \
        {                                                       \
            glEnableClientState( GL_NORMAL_ARRAY );             \
            _rwOpenGLVANormalEnabled = TRUE;                    \
        }                                                       \
        _rwOpenGLExt.ArrayObjectATI( GL_NORMAL_ARRAY,           \
                                     3,                         \
                                     (_baseType),               \
                                     (_stride),                 \
                                     (_vaoName),                \
                                     (RwUInt32)(_offset) );     \
    }                                                           \
    else if ( FALSE != _rwOpenGLVANormalEnabled )               \
    {                                                           \
        glDisableClientState( GL_NORMAL_ARRAY );                \
        _rwOpenGLVANormalEnabled = FALSE;                       \
    }                                                           \
}                                                               \
MACRO_STOP

/* Macros for GL_COLOR_ARRAY */
#define RwOpenGLVASetColorMacro(_enableTest,_numComponents,_baseType,   \
                                _stride,_memAddr)                       \
MACRO_START                                                             \
{                                                                       \
    if ( FALSE != (_enableTest) )                                       \
    {                                                                   \
        if ( FALSE == _rwOpenGLVAColorEnabled )                         \
        {                                                               \
            glEnableClientState( GL_COLOR_ARRAY );                      \
            _rwOpenGLVAColorEnabled = TRUE;                             \
        }                                                               \
        glColorPointer((_numComponents),(_baseType),                    \
                       (_stride),(const GLvoid *)_memAddr);             \
    }                                                                   \
    else if ( FALSE != _rwOpenGLVAColorEnabled )                        \
    {                                                                   \
        glDisableClientState( GL_COLOR_ARRAY );                         \
        _rwOpenGLVAColorEnabled = FALSE;                                \
    }                                                                   \
}                                                                       \
MACRO_STOP

#define RwOpenGLVASetColorATIMacro(_enableTest,_numComponents,  \
                                   _baseType,_stride,_vaoName,  \
                                   _offset)                     \
MACRO_START                                                     \
{                                                               \
    if ( FALSE != (_enableTest) )                               \
    {                                                           \
        if ( FALSE == _rwOpenGLVAColorEnabled )                 \
        {                                                       \
            glEnableClientState( GL_COLOR_ARRAY );              \
            _rwOpenGLVAColorEnabled = TRUE;                     \
        }                                                       \
        _rwOpenGLExt.ArrayObjectATI( GL_COLOR_ARRAY,            \
                                     (_numComponents),          \
                                     (_baseType),               \
                                     (_stride),                 \
                                     (_vaoName),                \
                                     (RwUInt32)(_offset) );     \
    }                                                           \
    else if ( FALSE != _rwOpenGLVAColorEnabled )                \
    {                                                           \
        glDisableClientState( GL_COLOR_ARRAY );                 \
        _rwOpenGLVAColorEnabled = FALSE;                        \
    }                                                           \
}                                                               \
MACRO_STOP

/* Macros for GL_TEXTURE_COORD_ARRAY */
#define RwOpenGLVASetTexUnitMacro(_texUnit)                                     \
MACRO_START                                                                     \
{                                                                               \
    if ( (RwUInt32)(_texUnit) != _rwOpenGLVATexUnit )                           \
    {                                                                           \
        if ( NULL != _rwOpenGLExt.ClientActiveTextureARB )                      \
        {                                                                       \
            _rwOpenGLExt.ClientActiveTextureARB( GL_TEXTURE0_ARB + (_texUnit) );\
            _rwOpenGLVATexUnit = (_texUnit);                                    \
        }                                                                       \
    }                                                                           \
}                                                                               \
MACRO_STOP

#define RwOpenGLVASetTexCoordMacro(_enableTest,_numComponents,      \
                                   _baseType,_stride,_memAddr)      \
MACRO_START                                                         \
{                                                                   \
    RwOpenGLVASetTexUnitMacro(0);                                   \
    if ( FALSE != (_enableTest) )                                   \
    {                                                               \
        if ( FALSE == _rwOpenGLVATexCoordEnabled[0] )               \
        {                                                           \
            glEnableClientState( GL_TEXTURE_COORD_ARRAY );          \
            _rwOpenGLVATexCoordEnabled[0] = TRUE;                   \
        }                                                           \
        glTexCoordPointer((_numComponents),(_baseType),(_stride),   \
                          (const GLvoid *)_memAddr);                \
    }                                                               \
    else if ( FALSE != _rwOpenGLVATexCoordEnabled[0] )              \
    {                                                               \
        glDisableClientState( GL_TEXTURE_COORD_ARRAY );             \
        _rwOpenGLVATexCoordEnabled[0] = FALSE;                      \
    }                                                               \
}                                                                   \
MACRO_STOP


#define RwOpenGLVASetTexCoordATIMacro(_enableTest,_numComponents,   \
                                      _baseType,_stride,_vaoName,   \
                                      _offset)                      \
MACRO_START                                                         \
{                                                                   \
    RwOpenGLVASetTexUnitMacro(0);                                   \
    if ( FALSE != (_enableTest) )                                   \
    {                                                               \
        if ( FALSE == _rwOpenGLVATexCoordEnabled[0] )               \
        {                                                           \
            glEnableClientState( GL_TEXTURE_COORD_ARRAY );          \
            _rwOpenGLVATexCoordEnabled[0] = TRUE;                   \
        }                                                           \
        _rwOpenGLExt.ArrayObjectATI( GL_TEXTURE_COORD_ARRAY,        \
                                     (_numComponents),              \
                                     (_baseType),                   \
                                     (_stride),                     \
                                     (_vaoName),                    \
                                     (RwUInt32)(_offset) );         \
    }                                                               \
    else if ( FALSE != _rwOpenGLVATexCoordEnabled[0] )              \
    {                                                               \
        glDisableClientState( GL_TEXTURE_COORD_ARRAY );             \
        _rwOpenGLVATexCoordEnabled[0] = FALSE;                      \
    }                                                               \
}                                                                   \
MACRO_STOP


/* Macros for multitexture GL_TEXTURE_COORD_ARRAY */
#define RwOpenGLVASetMultiTexCoordMacro(_enableTest,_texUnit,         \
                                        _numComponents,_baseType,     \
                                        _stride,_memAddr)             \
MACRO_START                                                           \
{                                                                     \
    RwOpenGLVASetTexUnitMacro(_texUnit);                              \
    if ( FALSE != (_enableTest) )                                     \
    {                                                                 \
        if ( FALSE == _rwOpenGLVATexCoordEnabled[(_texUnit)])         \
        {                                                             \
            glEnableClientState( GL_TEXTURE_COORD_ARRAY );            \
            _rwOpenGLVATexCoordEnabled[(_texUnit)] = TRUE;            \
        }                                                             \
        glTexCoordPointer((_numComponents),(_baseType),(_stride),     \
                          (const GLvoid *)_memAddr);                  \
    }                                                                 \
    else if ( FALSE != _rwOpenGLVATexCoordEnabled[(_texUnit)] )       \
    {                                                                 \
        glDisableClientState( GL_TEXTURE_COORD_ARRAY );               \
        _rwOpenGLVATexCoordEnabled[(_texUnit)] = FALSE;               \
    }                                                                 \
}                                                                     \
MACRO_STOP

#define RwOpenGLVASetMultiTexCoordATIMacro(_enableTest,_texUnit,      \
                                           _numComponents,_baseType,  \
                                           _stride,_vaoName,          \
                                           _offset)                   \
MACRO_START                                                           \
{                                                                     \
    RwOpenGLVASetTexUnitMacro(_texUnit);                              \
    if ( FALSE != (_enableTest) )                                     \
    {                                                                 \
        if ( FALSE == _rwOpenGLVATexCoordEnabled[(_texUnit)])         \
        {                                                             \
            glEnableClientState( GL_TEXTURE_COORD_ARRAY );            \
            _rwOpenGLVATexCoordEnabled[(_texUnit)] = TRUE;            \
        }                                                             \
        _rwOpenGLExt.ArrayObjectATI( GL_TEXTURE_COORD_ARRAY,          \
                                     (_numComponents),                \
                                     (_baseType),                     \
                                     (_stride),                       \
                                     (_vaoName),                      \
                                     (RwUInt32)(_offset) );           \
    }                                                                 \
    else if ( FALSE != _rwOpenGLVATexCoordEnabled[(_texUnit)] )       \
    {                                                                 \
        glDisableClientState( GL_TEXTURE_COORD_ARRAY );               \
        _rwOpenGLVATexCoordEnabled[(_texUnit)] = FALSE;               \
    }                                                                 \
}                                                                     \
MACRO_STOP


#define RwOpenGLVADisablePositionMacro()            \
MACRO_START                                         \
    if ( FALSE != _rwOpenGLVAPositionEnabled )      \
    {                                               \
        glDisableClientState( GL_VERTEX_ARRAY );    \
        _rwOpenGLVAPositionEnabled = FALSE;         \
    }                                               \
MACRO_STOP      

#define RwOpenGLVADisableNormalMacro()              \
MACRO_START                                         \
    if ( FALSE != _rwOpenGLVANormalEnabled )        \
    {                                               \
        glDisableClientState( GL_NORMAL_ARRAY );    \
        _rwOpenGLVANormalEnabled = FALSE;           \
    }                                               \
MACRO_STOP

#define RwOpenGLVADisableColorMacro()           \
MACRO_START                                     \
    if ( FALSE != _rwOpenGLVAColorEnabled )     \
    {                                           \
        glDisableClientState( GL_COLOR_ARRAY ); \
        _rwOpenGLVAColorEnabled = FALSE;        \
    }                                           \
MACRO_STOP

#define RwOpenGLVADisableTexCoordMacro(_texUnit)                \
MACRO_START                                                     \
{                                                               \
    RwOpenGLVASetTexUnitMacro(_texUnit);                        \
    if ( FALSE != _rwOpenGLVATexCoordEnabled[(_texUnit)] )      \
    {                                                           \
        glDisableClientState( GL_TEXTURE_COORD_ARRAY );         \
        _rwOpenGLVATexCoordEnabled[(_texUnit)] = FALSE;         \
    }                                                           \
}                                                               \
MACRO_STOP

#if !(defined(RWDEBUG) || defined(RWSUPPRESSINLINE))

#define RwOpenGLVASetPosition   \
    RwOpenGLVASetPositionMacro

#define RwOpenGLVASetPositionATI    \
    RwOpenGLVASetPositionATIMacro

#define RwOpenGLVASetNormal  \
    RwOpenGLVASetNormalMacro

#define RwOpenGLVASetNormalATI  \
    RwOpenGLVASetNormalATIMacro

#define RwOpenGLVASetColor  \
    RwOpenGLVASetColorMacro

#define RwOpenGLVASetColorATI   \
    RwOpenGLVASetColorATIMacro

#define RwOpenGLVASetTexCoord   \
    RwOpenGLVASetTexCoordMacro

#define RwOpenGLVASetTexCoordATI    \
    RwOpenGLVASetTexCoordATIMacro

#define RwOpenGLVASetMultiTexCoord  \
    RwOpenGLVASetMultiTexCoordMacro

#define RwOpenGLVASetMultiTexCoordATI   \
    RwOpenGLVASetMultiTexCoordATIMacro

#define RwOpenGLVADisablePosition   \
    RwOpenGLVADisablePositionMacro

#define RwOpenGLVADisableNormal \
    RwOpenGLVADisableNormalMacro

#define RwOpenGLVADisableColor  \
    RwOpenGLVADisableColorMacro

#define RwOpenGLVADisableTexCoord   \
    RwOpenGLVADisableTexCoordMacro

#endif /* !(defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

/* RWPUBLICEND */


/* =====================================================================
 *  Module specific type definitions
 * ===================================================================== */


/* =====================================================================
 *  Extern variables
 * ===================================================================== */

/* RWPUBLIC */

#if defined( __cplusplus )
extern "C"
{
#endif /* defined( __cplusplus ) */

extern RwBool   _rwOpenGLVAPositionEnabled;
extern RwBool   _rwOpenGLVANormalEnabled;
extern RwBool   _rwOpenGLVAColorEnabled;
extern RwBool   *_rwOpenGLVATexCoordEnabled;
extern RwUInt32 _rwOpenGLVATexUnit;

/* RWPUBLICEND */

/* =====================================================================
 *  Extern function prototypes
 * ===================================================================== */

extern RwBool
_rwOpenGLVAOpen( void );

extern void
_rwOpenGLVAClose( void );

/* RWPUBLIC */

#if (defined(RWDEBUG) || defined(RWSUPPRESSINLINE))

extern void
RwOpenGLVASetPosition( const RwUInt32 numComponents,
                       const RwInt32 baseType,
                       const RwUInt32 stride,
                       const void *memAddr );
extern void
RwOpenGLVASetPositionATI( const RwUInt32 numComponents,
                          const RwInt32 baseType,
                          const RwUInt32 stride,
                          const RwUInt32 vaoName,
                          const void *offset );

extern void
RwOpenGLVASetNormal( const RwBool enableTest,
                     const RwInt32 baseType,
                     const RwUInt32 stride,
                     const void *memAddr );
extern void
RwOpenGLVASetNormalATI( const RwBool enableTest,
                        const RwInt32 baseType,
                        const RwUInt32 stride,
                        const RwUInt32 vaoName,
                        const void *offset );

extern void
RwOpenGLVASetColor( const RwBool enableTest,
                    const RwUInt32 numComponents,
                    const RwInt32 baseType,
                    const RwUInt32 stride,
                    const void *memAddr );
extern void
RwOpenGLVASetColorATI( const RwBool enableTest,
                       const RwUInt32 numComponents,
                       const RwInt32 baseType,
                       const RwUInt32 stride,
                       const RwUInt32 vaoName,
                       const void *offset );

extern void
RwOpenGLVASetTexCoord( const RwBool enableTest,
                       const RwUInt32 numComponents,
                       const RwInt32 baseType,
                       const RwUInt32 stride,
                       const void *memAddr );
extern void
RwOpenGLVASetTexCoordATI( const RwBool enableTest,
                          const RwUInt32 numComponents,
                          const RwInt32 baseType,
                          const RwUInt32 stride,
                          const RwUInt32 vaoName,
                          const void *offset );

extern void
RwOpenGLVASetMultiTexCoord( const RwBool enableTest,
                            const RwInt32 texUnit,
                            const RwUInt32 numComponents,
                            const RwInt32 baseType,
                            const RwUInt32 stride,
                            const void *memAddr );
extern void
RwOpenGLVASetMultiTexCoordATI( const RwBool enableTest,
                               const RwInt32 texUnit,
                               const RwUInt32 numComponents,
                               const RwInt32 baseType,
                               const RwUInt32 stride,
                               const RwUInt32 vaoName,
                               const void *offset );

extern void
RwOpenGLVADisablePosition( void );

extern void
RwOpenGLVADisableNormal( void );

extern void
RwOpenGLVADisableColor( void );

extern void
RwOpenGLVADisableTexCoord( const RwInt32 texUnit );

#endif /* (defined(RWDEBUG) || defined(RWSUPPRESSINLINE)) */

#if defined( __cplusplus )
}
#endif /* defined( __cplusplus ) */

/* RWPUBLICEND */

#endif /* !defined( BAVAGL_H ) */
