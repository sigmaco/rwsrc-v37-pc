/****************************************************************************
 *                                                                          *
 *                                                                          *
 *                                                                          *
 *                                                                          *
 *                                                                          *
 ****************************************************************************/

#ifndef TRI_H
#define TRI_H

/****************************************************************************
 Includes
 */

/****************************************************************************
 Defines
 */
#define rt2dPOINTCLASSIFYFLAGLEFT           0x01
#define rt2dPOINTCLASSIFYFLAGRIGHT          0x02
#define rt2dPOINTCLASSIFYFLAGCOLINEAR       0x04
#define rt2dPOINTCLASSIFYFLAGINSIDE         0x08
#define rt2dPOINTCLASSIFYFLAGOUTSIDE        0x10

/****************************************************************************
 Global Types
 */

typedef struct _rt2dTriIndex _rt2dTriIndex;

struct _rt2dTriIndex
{
    RwInt32             currIdx;
    RwInt32             *idx;
};

typedef struct _rt2dTriVert _rt2dTriVert;

struct _rt2dTriVert
{
    _rt2dTriVert        *next;
    RwInt32             idx;
};


typedef struct _rt2dTriPoly _rt2dTriPoly;

struct _rt2dTriPoly
{
    _rt2dTriPoly        *next;
    _rt2dTriVert        *vert;

    _rt2dTriIndex       *triIndex;

    RwInt32             vCount;

    RwV2d               *pos;
};

/****************************************************************************
 Global variables (across program)
 */

/****************************************************************************
 Function prototypes
 */

extern Rt2dPath *
_rt2dPathTriangulate( Rt2dPath *path, RwInt32 *count, RwInt32 *index );

extern void
_rt2dTriangulateClose( void );

extern RwBool
_rt2dTriangulateOpen( void );

#endif /* TRI_H */

