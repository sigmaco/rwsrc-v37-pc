/****************************************************************************
 *                                                                          *
 *  Module  :   x86matbl.c                                                  *
 *                                                                          *
 *  Purpose :   Intel x86 matrix-blending functions                         *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 Includes
 */

#include "rpplugin.h"
#include "rwcore.h"
#include "rpdbgerr.h"
#include "rpskin.h"
#include "skin.h"
#include "x86matbl.h"

/****************************************************************************
 Local Types
 */

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Defines
 */

#if (defined(__VECTORC__))
#define not0 -1
#endif /* (defined(__VECTORC__)) */

#if (!defined(not0))
#define not0 not 0
#endif /* (!defined(not0)) */

#if (!defined(NOASM))

#define mat_mxx                 0
#define mat_mxy                 4
#define mat_mxz                 8
#define mat_myx                 16
#define mat_myy                 20
#define mat_myz                 24
#define mat_mzx                 32
#define mat_mzy                 36
#define mat_mzz                 40
#define mat_mwx                 48
#define mat_mwy                 52
#define mat_mwz                 56

#define _rwV3d_nX               0
#define _rwV3d_nY               4
#define _rwV3d_nZ               8
#define sizeof_rwV3d            12

#define sizeof_RwMatrixWeights  16

#define sizeof_RwUInt32         4

#define sizeof_RwReal           4

#else /* !defined(NOASM) */

#define FLOATASINT(f) (*((const RwInt32 *)&(f)))

#define MATRIXPROCESSX(m, o, w)                                     \
    ( ( (m->right.x * o.x) +                                        \
        (m->up.x    * o.y) +                                        \
        (m->at.x    * o.z) +                                        \
        (m->pos.x) ) * w )

#define MATRIXPROCESSY(m, o, w)                                     \
    ( ( (m->right.y * o.x) +                                        \
        (m->up.y    * o.y) +                                        \
        (m->at.y    * o.z) +                                        \
        (m->pos.y) ) * w )

#define MATRIXPROCESSZ(m, o, w)                                     \
    ( ( (m->right.z * o.x) +                                        \
        (m->up.z    * o.y) +                                        \
        (m->at.z    * o.z) +                                        \
        (m->pos.z) ) * w )

#define MATRIXSKIN(v, m, o, w)                                      \
MACRO_START                                                         \
{                                                                   \
    v->x = MATRIXPROCESSX( m, o, w );                                \
    v->y = MATRIXPROCESSY( m, o, w );                                \
    v->z = MATRIXPROCESSZ( m, o, w );                                \
}                                                                   \
MACRO_STOP

#define MATRIXPLUSSKIN(v, m, o, w)                                  \
MACRO_START                                                         \
{                                                                   \
    v->x += MATRIXPROCESSX( m, o, w );                               \
    v->y += MATRIXPROCESSY( m, o, w );                               \
    v->z += MATRIXPROCESSZ( m, o, w );                               \
}                                                                   \
MACRO_STOP

#define MATRIXNORMALPROCESSX(m, o, w)                               \
    ( ( (m->right.x * o.x) +                                        \
        (m->up.x    * o.y) +                                        \
        (m->at.x    * o.z) ) * w )

#define MATRIXNORMALPROCESSY(m, o, w)                               \
    ( ( (m->right.y * o.x) +                                        \
        (m->up.y    * o.y) +                                        \
        (m->at.y    * o.z) ) * w )

#define MATRIXNORMALPROCESSZ(m, o, w)                               \
    ( ( (m->right.z * o.x) +                                        \
        (m->up.z    * o.y) +                                        \
        (m->at.z    * o.z) ) * w )

#define MATRIXNORMALSKIN(n, m, o, w)                                \
MACRO_START                                                         \
{                                                                   \
    n->x = MATRIXNORMALPROCESSX( m, o, w );                          \
    n->y = MATRIXNORMALPROCESSY( m, o, w );                          \
    n->z = MATRIXNORMALPROCESSZ( m, o, w );                          \
}                                                                   \
MACRO_STOP

#define MATRIXNORMALPLUSSKIN(n, m, o, w)                            \
MACRO_START                                                         \
{                                                                   \
    n->x += MATRIXNORMALPROCESSX( m, o, w );                         \
    n->y += MATRIXNORMALPROCESSY( m, o, w );                         \
    n->z += MATRIXNORMALPROCESSZ( m, o, w );                         \
}                                                                   \
MACRO_STOP

#endif /* !defined(NOASM) */

/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   Functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/****************************************************************************/

/****************************************************************************
 _rpSkinPrepareAtomicMatrix

 Purpose: Setup matrices for skinning.

 On entry: atomic    - Pointer to RpAtomic containing geometry data
           skin      - Pointer to RpSkin containing bones and weights data
           hierarchy - Pointer to RpHAnimHierarchy
 On exit : Pointer to RwMatrix, which is the base of the array of skinning matrices
           or NULL if there is no skinning data.
*/
RwMatrix *
_rpSkinPrepareAtomicMatrix(RpAtomic *atomic, RpSkin *skin, RpHAnimHierarchy *hierarchy)
{
    RwMatrix    *matrixArray = NULL;

    RWFUNCTION(RWSTRING("_rpSkinPrepareAtomicMatrix"));
    RWASSERT(atomic);

    if(NULL != hierarchy)
    {
        RwInt32         i;
        const RwMatrix  *skinToBone;

        matrixArray = _rpSkinGlobals.matrixCache.aligned;

        /* Get the bone information. */
        skinToBone = RpSkinGetSkinToBoneMatrices(skin);
        RWASSERT(NULL != skinToBone);

        if(hierarchy->flags & rpHANIMHIERARCHYNOMATRICES)
        {
            RwFrame *frame;
            RwMatrix *ltm;

            RwMatrix inverseAtomicLTM;
            RwMatrix temmatrix;
            RwInt32 i;

            frame = RpAtomicGetFrame(atomic);
            RWASSERT(NULL != frame);
            ltm = RwFrameGetLTM(frame);
            RWASSERT(NULL != ltm);

            RwMatrixInvert(&inverseAtomicLTM, ltm);

            for( i = 0; i < hierarchy->numNodes; i++ )
            {
                RwFrame *node;
                RwMatrix *ltm;

                node = hierarchy->pNodeInfo[i].pFrame;
                RWASSERT(NULL != node);
                ltm = RwFrameGetLTM(node);
                RWASSERT(NULL != ltm);

                RwMatrixMultiply( &temmatrix,
                                  &skinToBone[i],
                                  ltm );
                RwMatrixMultiply( &matrixArray[i],
                                  &temmatrix,
                                  &inverseAtomicLTM );
            }
        }
        else if(hierarchy->flags & rpHANIMHIERARCHYLOCALSPACEMATRICES)
        {
            for( i = 0; i < hierarchy->numNodes; i++ )
            {
                RwMatrixMultiply( &matrixArray[i],
                                  &skinToBone[i],
                                  &(hierarchy->pMatrixArray[i]) );
            }
        }
        else
        {
            RwMatrix inverseAtomicLTM;
            RwMatrix temmatrix;

            RwMatrixInvert(&inverseAtomicLTM,
                           RwFrameGetLTM(RpAtomicGetFrame
                                         (atomic)));

            for( i = 0; i < hierarchy->numNodes; i++)
            {
                RwMatrixMultiply( &temmatrix,
                                  &skinToBone[i],
                                  &(hierarchy->pMatrixArray[i]) );
                RwMatrixMultiply( &matrixArray[i],
                                  &temmatrix,
                                  &inverseAtomicLTM );
            }
        }
    }

    RWRETURN(matrixArray);
}

/****************************************************************************
 _rpSkinPrepareAtomicMatrixWorldSpace

 Purpose: Setup matrices for skinning.

 On entry: atomic    - Pointer to RpAtomic containing geometry data
           skin      - Pointer to RpSkin containing bones and weights data
           hierarchy - Pointer to RpHAnimHierarchy
 On exit : Pointer to RwMatrix, which is the base of the array of skinning matrices
           or NULL if there is no skinning data.
*/
RwMatrix *
_rpSkinPrepareAtomicMatrixWorldSpace(RpAtomic *atomic, RpSkin *skin, RpHAnimHierarchy *hierarchy)
{
    RwMatrix    *matrixArray = NULL;

    RWFUNCTION(RWSTRING("_rpSkinPrepareAtomicMatrixWorldSpace"));
    RWASSERT(atomic);

    if(NULL != hierarchy)
    {
        RwInt32         i;
        const RwMatrix  *skinToBone;

        matrixArray = _rpSkinGlobals.matrixCache.aligned;

        /* Get the bone information. */
        skinToBone = RpSkinGetSkinToBoneMatrices(skin);
        RWASSERT(NULL != skinToBone);

        if(hierarchy->flags & rpHANIMHIERARCHYNOMATRICES)
        {
            for( i = 0; i < hierarchy->numNodes; i++ )
            {
                const RwMatrix *ltm =
                                RwFrameGetLTM(hierarchy->pNodeInfo[i].pFrame);

                RWASSERT(NULL != ltm);

                RwMatrixMultiply( &matrixArray[i],
                                  &skinToBone[i],
                                  ltm );
            }
        }
        else if(hierarchy->flags & rpHANIMHIERARCHYLOCALSPACEMATRICES)
        {
            const RwMatrix *localToWorld =
                            RwFrameGetLTM(RpAtomicGetFrame(atomic));

            RwMatrix temmatrix;

            for( i = 0; i < hierarchy->numNodes; i++ )
            {
                RwMatrixMultiply( &temmatrix,
                                  &skinToBone[i],
                                  &(hierarchy->pMatrixArray[i]) );

                RwMatrixMultiply( &matrixArray[i],
                                  &temmatrix,
                                  localToWorld );
            }
        }
        else
        {
            for( i = 0; i < hierarchy->numNodes; i++)
            {
                RwMatrixMultiply( &matrixArray[i],
                                  &skinToBone[i],
                                  &(hierarchy->pMatrixArray[i]) );
            }
        }
    }

    RWRETURN(matrixArray);
}

#if (defined(RWDEBUG) && !defined(NOASM))

void
_rpSkinIntelx86ConfirmConstants(void)
{
    RWFUNCTION( RWSTRING( "_rpSkinIntelx86ConfirmConstants" ) );

    /* checking RwMatrix magic number offsets */
    RWASSERT( mat_mxx == offsetof(RwMatrix,right.x) );
    RWASSERT( mat_mxy == offsetof(RwMatrix,right.y) );
    RWASSERT( mat_mxz == offsetof(RwMatrix,right.z) );
    RWASSERT( mat_myx == offsetof(RwMatrix,up.x) );
    RWASSERT( mat_myy == offsetof(RwMatrix,up.y) );
    RWASSERT( mat_myz == offsetof(RwMatrix,up.z) );
    RWASSERT( mat_mzx == offsetof(RwMatrix,at.x) );
    RWASSERT( mat_mzy == offsetof(RwMatrix,at.y) );
    RWASSERT( mat_mzz == offsetof(RwMatrix,at.z) );
    RWASSERT( mat_mwx == offsetof(RwMatrix,pos.x) );
    RWASSERT( mat_mwy == offsetof(RwMatrix,pos.y) );
    RWASSERT( mat_mwz == offsetof(RwMatrix,pos.z) );

    /* checking RwV3d magic number offsets */
    RWASSERT( _rwV3d_nX == offsetof(RwV3d,x) );
    RWASSERT( _rwV3d_nY == offsetof(RwV3d,y) );
    RWASSERT( _rwV3d_nZ == offsetof(RwV3d,z) );
    RWASSERT( sizeof_rwV3d == sizeof(RwV3d) );

    /* checking RwMatrixWeights magic numbers */
    RWASSERT( sizeof_RwMatrixWeights == sizeof(RwMatrixWeights) );

    /* checking RwUInt32 magic numbers */
    RWASSERT( sizeof_RwUInt32 == sizeof(RwUInt32) );

    /* checking RwReal magic numbers */
    RWASSERT( sizeof_RwReal == sizeof(RwReal) );

    RWRETURNVOID();
}

#endif /* !defined(NOASM) && defined(RWDEBUG) */

#if (!defined(NOASM))

/****************************************************************************
 _rpSkinIntelx86MatrixBlend

 Assembler encapsulated matrix blending.
 Register usage:

    eax         1) pointer to vertices to write to
                2) pointer to normals to write to
                3) temporary result storage

    ebx         pointer to current skinning RwMatrix

    ecx         vertex counter

    edx         1) pointer to vertices to read from
                2) pointer to normals to read from

    edi         pointer to current skin weights

    esi         1) pointer to index of current skin matrix
                2) boolean to indicate whether the blending is
                   using the first weight or not (to assign rather
                   than add) [0 == not first weight, ~0 == first weight]

 Note: Doesn't check whether w0 = 1.0. This might be faster, but difficult
       to implement in the manner used. (Useful because w0+w1+w2+w3 = 1)

 Inputs :   pSkin->totalVertices        number of vertices
            pSkin->pMatrixWeightsMap    array of matrix weights
            pSkin->pMatrixIndexMap      array of matrix indices
            pMatrixArray                array of matrices
            pVertices                   array of vertices to write to
            pOriginalVertices           array of vertices to read from
            pNormals                    array of normals to write to
            pOriginalNormals            array of normals to read from

 Outputs:   Nothing
 *
 */

void
_rpSkinIntelx86MatrixBlend( RwInt32 numVertices __RWUNUSED__,
                            const RwMatrixWeights *matrixWeightsMap __RWUNUSED__,
                            const RwUInt32 *matrixIndexMap __RWUNUSED__,
                            const RwMatrix *matrixArray __RWUNUSED__,
                            RwV3d *vertices __RWUNUSED__,
                            const RwV3d *originalVertices __RWUNUSED__,
                            RwV3d *normals __RWUNUSED__,
                            const RwV3d *originalNormals __RWUNUSED__,
                            RwUInt32 stride __RWUNUSED__ )
{
    RwInt32         vertexOffset __RWUNUSED__;

    RWFUNCTION( RWSTRING( "_rpSkinIntelx86MatrixBlend" ) );

#if !defined(__GNUC__)
    __asm
    {
        push    ecx                                 ; need the counter
        mov     ecx, dword ptr [numVertices]        ; loop counter for number of vertices

        push    edi                                 ; data index for weights map
        mov     edi, sizeof_RwMatrixWeights         ; sizeof(RwMatrixWeights)
        imul    edi, ecx                            ; sizeof_RwMatrixWeights * numVertices
        sub     edi, sizeof_RwMatrixWeights         ; sizeof_RwMatrixWeights * (numVertices - 1)
        add     edi, dword ptr [matrixWeightsMap]   ; offset to last set of RwMatrixWeights

        push    esi                                 ; data index for index map
        mov     esi, sizeof_RwUInt32                ; sizeof(RwUInt32)
        imul    esi, ecx                            ; sizeof(RwUInt32) * numVertices
        sub     esi, sizeof_RwUInt32                ; sizeof(RwUInt32) * (numVertices - 1)
        add     esi, dword ptr [matrixIndexMap]     ; offset to last set of matrix indices

        push    eax                                 ; we will always need it
        push    ebx                                 ; for the matrix
        push    edx                                 ; for vertices and originalVertices

        mov     eax, stride
        imul    eax, ecx                            ; stride * numVertices
        sub     eax, stride                         ; stride * (numVertices - 1)
        mov     vertexOffset, eax                   ; save the independent vertex offset

        mov     edx, sizeof_rwV3d                   ; sizeof(RwV3d)
        imul    edx, ecx                            ; sizeof_rwV3d * numVertices
        sub     edx, sizeof_rwV3d                   ; sizeof_rwV3d * (numVertices - 1)
        add     edx, dword ptr [originalVertices]   ; offset to last read vertex

weight1:
        push    edi                                 ; store the current weight map index

        cmp     dword ptr [edi], 0                  ; compare w0 with 0
        jle     nextVertex                          ; goto nextVertex if w0 <= 0

        mov     ebx, [esi]                          ; = offset to last set of matrix indices
        and     ebx, 255                            ; mask out everything but the first 8 bits
        shl     ebx, 6                              ; * 64 (where 64 is sizeof(RwMatrix)
        add     ebx, dword ptr [matrixArray]        ; offset to the appropriate matrix

        mov     eax, dword ptr [vertexOffset]       ; set up eax to point to the offset
        add     eax, dword ptr [vertices]           ; of the vertex to write to

        push    esi                                 ; save the matrix index offset
        mov     esi, not0                           ; now indicates the first weight

        call    blendPoints                         ; blend the vertex points

        mov     eax, dword ptr [normals]            ; load the normals pointer into eax
        cmp     eax, 0                              ; if the pointer is NULL
        je      short weight2                       ; do not do the normal calculations

        add     eax, dword ptr [vertexOffset]       ; set up eax to the normal to write to
        sub     edx, dword ptr [originalVertices]   ; set up edx to point to the offset
        add     edx, dword ptr [originalNormals]    ; of the normals to read from

        call    blendNormals                        ; blend the vertex normals

        sub     edx, dword ptr [originalNormals]    ; reset edx to point to the offset of
        add     edx, dword ptr [originalVertices]   ; the vertices to read from

weight2:
        pop     esi                                 ; restore the matrix index offset

        add     edi, sizeof_RwReal                  ; move to the next weight
        cmp     dword ptr [edi], 0                  ; compare w1 with 0
        jle     nextVertex                          ; goto nextVertex if w1 <= 0

        mov     ebx, [esi]                          ; = offset to last set of matrix indices
        shr     ebx, 8                              ; right shift the offset by 8
        and     ebx, 255                            ; mask out everything but the first 8 bits
        shl     ebx, 6                              ; * 64 (where 64 is sizeof(RwMatrix)
        add     ebx, dword ptr [matrixArray]        ; offset to the appropriate matrix

        mov     eax, dword ptr [vertexOffset]       ; set up eax to point
        add     eax, dword ptr [vertices]           ; vertex to write

        push    esi                                 ; save the matrix index offset
        mov     esi, 0                              ; now indicates (not) the first weight

        call    blendPoints                         ; blend the vertex points

        mov     eax, dword ptr [normals]            ; load the normals pointer into eax
        cmp     eax, 0                              ; if the pointer is NULL
        je      short weight3                       ; do not do the normal calculations

        add     eax, dword ptr [vertexOffset]       ; set up eax to the normal to write to
        sub     edx, dword ptr [originalVertices]   ; set up edx to point to the offset
        add     edx, dword ptr [originalNormals]    ; of the normals to read from

        call    blendNormals                        ; blend the vertex normals

        sub     edx, dword ptr [originalNormals]    ; reset edx to point to the offset of
        add     edx, dword ptr [originalVertices]   ; the vertices to read from

weight3:
        pop     esi                                 ; restore the matrix index offset

        add     edi, sizeof_RwReal                  ; move to the next weight
        cmp     dword ptr [edi], 0                  ; compare w2 with 0
        jle     nextVertex                          ; goto nextVertex if w2 <= 0

        mov     ebx, [esi]                          ; = offset to last set of matrix indices
        shr     ebx, 16                             ; right shift the offset by 16
        and     ebx, 255                            ; mask out everything but the first 8 bits
        shl     ebx, 6                              ; * 64 (where 64 is sizeof(RwMatrix)
        add     ebx, dword ptr [matrixArray]        ; offset to the appropriate matrix

        mov     eax, dword ptr [vertexOffset]       ; set up eax to point
        add     eax, dword ptr [vertices]           ; vertex to write

        push    esi                                 ; save the matrix index offset
        mov     esi, 0                              ; now indicates (not) the first weight

        call    blendPoints                         ; blend the vertex points

        mov     eax, dword ptr [normals]            ; load the normals pointer into eax
        cmp     eax, 0                              ; if the pointer is NULL
        je      short weight4                       ; do not do the normal calculations

        add     eax, dword ptr [vertexOffset]       ; set up eax to the normal to write to
        sub     edx, dword ptr [originalVertices]   ; set up edx to point to the offset
        add     edx, dword ptr [originalNormals]    ; of the normals to read from

        call    blendNormals                        ; blend the vertex normals

        sub     edx, dword ptr [originalNormals]    ; reset edx to point to the offset of
        add     edx, dword ptr [originalVertices]   ; the vertices to read from

weight4:
        pop     esi                                 ; restore the matrix index offset

        add     edi, sizeof_RwReal                  ; move to the next weight
        cmp     dword ptr [edi], 0                  ; compare w3 with 0
        jle     short nextVertex                    ; goto nextVertex if w3 <= 0

        mov     ebx, [esi]                          ; = offset to last set of matrix indices
        shr     ebx, 24                             ; right shift the offset by 24
        and     ebx, 255                            ; mask out everything but the first 8 bits
        shl     ebx, 6                              ; * 64 (where 64 is sizeof(RwMatrix)
        add     ebx, dword ptr [matrixArray]        ; offset to the appropriate matrix

        mov     eax, dword ptr [vertexOffset]       ; set up eax to point
        add     eax, dword ptr [vertices]           ; vertex to write

        push    esi                                 ; save the matrix index offset
        mov     esi, 0                              ; now indicates (not) the first weight

        call    blendPoints                         ; blend the vertex points

        mov     eax, dword ptr [normals]            ; load the normals pointer into eax
        cmp     eax, 0                              ; if the pointer is NULL
        je      short pre_nextVertex                ; do not do the normal calculations

        add     eax, dword ptr [vertexOffset]       ; set up eax to the normal to write to
        sub     edx, dword ptr [originalVertices]   ; set up edx to point to the offset
        add     edx, dword ptr [originalNormals]    ; of the normals to read from

        call    blendNormals                        ; blend the vertex normals

        sub     edx, dword ptr [originalNormals]    ; reset edx to point to the offset of
        add     edx, dword ptr [originalVertices]   ; the vertices to read from

pre_nextVertex:
        pop     esi                                 ; restore the matrix index offset
nextVertex:
        pop     edi                                 ; restore the previous weight map index

        mov     eax, dword ptr [vertexOffset]
        sub     edx, sizeof_rwV3d                   ; previous RwV3d
        sub     eax, dword ptr [stride]             ; reduce the vertex offset
        mov     dword ptr [vertexOffset], eax
        sub     edi, sizeof_RwMatrixWeights         ; previous RwMatrixWeights
        sub     esi, sizeof_RwUInt32                ; previous matrix indices

        dec     ecx                                 ; numVertices -= 1
        jne     weight1                             ; move onto the next vertex

        jmp     finishedSkin

blendPoints:
        fld     dword ptr [edx + _rwV3d_nX]         ; := u(x)
        fld     dword ptr [ebx + mat_mxx]           ; := R(x)
        fmul    st(0), st(1)                        ; := u(x) * R(x), keep u(x) on the stack
        fld     dword ptr [ebx + mat_mxy]           ; := R(y)
        fmul    st(0), st(2)                        ; := u(x) * R(y), keep u(x) on the stack
        fld     dword ptr [ebx + mat_mxz]           ; := R(z)
        fmulp   st(3), st(0)                        ; := u(x) * R(z)

        fld     dword ptr [edx + _rwV3d_nY]         ; := u(y)
        fld     dword ptr [ebx + mat_myx]           ; := U(x)
        fmul    st(0), st(1)                        ; := u(y) * U(x), keep u(y) on the stack
        faddp   st(3), st(0)                        ; := u(x) * R(x) + u(y) * U(x)
        fld     dword ptr [ebx + mat_myy]           ; := U(y)
        fmul    st(0), st(1)                        ; := u(y) * U(y), keep u(y) on the stack
        faddp   st(2), st(0)                        ; := u(x) * R(y) + u(y) * U(y)
        fld     dword ptr [ebx + mat_myz]           ; := U(z)
        fmulp   st(1), st(0)                        ; := u(y) * U(z)
        faddp   st(3), st(0)                        ; := u(x) * R(z) + u(y) * U(z)

        fld     dword ptr [edx + _rwV3d_nZ]         ; := u(z)
        fld     dword ptr [ebx + mat_mzx]           ; := A(x)
        fmul    st(0), st(1)                        ; := u(z) * A(x), keep u(z) on the stack
        faddp   st(3), st(0)                        ; := u(x) * R(x) + u(y) * U(x) + u(z) * A(x)
        fld     dword ptr [ebx + mat_mwx]           ; := P(x)
        faddp   st(3), st(0)                        ; := u(x) * R(x) + u(y) * U(x) + u(z) * A(x) + P(x)
        fld     dword ptr [ebx + mat_mzy]           ; := A(y)
        fmul    st(0), st(1)                        ; := u(z) * A(y), keep u(z) on the stack
        fld     dword ptr [edi]                     ; := weight
        fmulp   st(4), st(0)                        ; := weight * (u(x) * R(x) + u(y) * U(x) + u(z) * A(x) + P(x)), pop weight
        faddp   st(2), st(0)                        ; := u(x) * R(y) + u(y) * U(y) + u(z) * A(y)
        fld     dword ptr [ebx + mat_mwy]           ; := P(y)
        faddp   st(2), st(0)                        ; := u(x) * R(y) + u(y) * U(y) + u(z) * A(y) + P(y)
        fld     dword ptr [ebx + mat_mzz]           ; := A(z)
        fmulp   st(1), st(0)                        ; := u(z) * A(z)
        fld     dword ptr [edi]                     ; := weight
        fmulp   st(2), st(0)                        ; := weight * (u(x) * R(y) + u(y) * U(y) + u(z) * A(y) + P(y)), pop weight
        faddp   st(3), st(0)                        ; := u(x) * R(z) + u(y) * U(z) + u(z) * A(z)
        fld     dword ptr [ebx + mat_mwz]           ; := P(z)
        faddp   st(3), st(0)                        ; := u(x) * R(z) + u(y) * U(z) + u(z) * A(z) + P(z)

        cmp     esi, 0                              ; compare esi (first weight or not?)
        jne     short blendPoints_firstweight

        fld     dword ptr [eax + _rwV3d_nX]         ; := v(x)
        faddp   st(2), st(0)                        ; := v(x) + weight * (u(x) * R(x) + u(y) * U(x) + u(z) * A(x) + P(x)), pop v(x)
        fld     dword ptr [edi]                     ; := weight
        fmulp   st(3), st(0)                        ; := weight * ( u(x) * R(z) + u(y) * U(z) + u(z) * A(z) + P(z)), pop weight
        fld     dword ptr [eax + _rwV3d_nY]         ; := v(y)
        faddp   st(1), st(0)                        ; := v(y) + weight * (u(x) * R(y) + u(y) * U(y) + u(z) * A(y) + P(y)), pop v(y)

        fxch    st(1)                               ; swap v(x) + weight * (u(x) * R(x) + u(y) * U(x) + u(z) * A(x) + P(x)), back to st(0)
        fstp    dword ptr [eax + _rwV3d_nX]         ; v(x) = st(0)
        fld     dword ptr [eax + _rwV3d_nZ]         ; := v(z)
        faddp   st(2), st(0)                        ; := v(z) + weight * (u(x) * R(z) + u(y) * U(z) + u(z) * A(z) + P(z)), pop v(z)
        fstp    dword ptr [eax + _rwV3d_nY]         ; v(y) = st(1)
        fstp    dword ptr [eax + _rwV3d_nZ]         ; v(z) = st(2)
        ret

blendPoints_firstweight:
        fxch    st(1)                               ; swap weight * (u(x) * R(x) + u(y) * U(x) + u(z) * A(x) + P(x)), back to st(0)
        fstp    dword ptr [eax + _rwV3d_nX]         ; v(x) = st(0)
        fld     dword ptr [edi]                     ; := weight
        fmulp   st(2), st(0)                        ; := weight * ( u(x) * R(z) + u(y) * U(z) + u(z) * A(z) + P(z)), pop weight
        fstp    dword ptr [eax + _rwV3d_nY]         ; v(y) = st(1)
        fstp    dword ptr [eax + _rwV3d_nZ]         ; v(z) = st(2)
        ret

blendNormals:
        fld     dword ptr [edx + _rwV3d_nX]         ; := u(x)
        fld     dword ptr [ebx + mat_mxx]           ; := R(x)
        fmul    st(0), st(1)                        ; := u(x) * R(x), keep u(x) on the stack
        fld     dword ptr [ebx + mat_mxy]           ; := R(y)
        fmul    st(0), st(2)                        ; := u(x) * R(y), keep u(x) on the stack
        fld     dword ptr [ebx + mat_mxz]           ; := R(z)
        fmulp   st(3), st(0)                        ; := u(x) * R(z)

        fld     dword ptr [edx + _rwV3d_nY]         ; := u(y)
        fld     dword ptr [ebx + mat_myx]           ; := U(x)
        fmul    st(0), st(1)                        ; := u(y) * U(x), keep u(y) on the stack
        faddp   st(3), st(0)                        ; := u(x) * R(x) + u(y) * U(x)
        fld     dword ptr [ebx + mat_myy]           ; := U(y)
        fmul    st(0), st(1)                        ; := u(y) * U(y), keep u(y) on the stack
        faddp   st(2), st(0)                        ; := u(x) * R(y) + u(y) * U(y)
        fld     dword ptr [ebx + mat_myz]           ; := U(z)
        fmulp   st(1), st(0)                        ; := u(y) * U(z)
        faddp   st(3), st(0)                        ; := u(x) * R(z) + u(y) * U(z)

        fld     dword ptr [edx + _rwV3d_nZ]         ; := u(z)
        fld     dword ptr [ebx + mat_mzx]           ; := A(x)
        fmul    st(0), st(1)                        ; := u(z) * A(x), keep u(z) on the stack
        faddp   st(3), st(0)                        ; := u(x) * R(x) + u(y) * U(x) + u(z) * A(x)
        fld     dword ptr [ebx + mat_mzy]           ; := A(y)
        fmul    st(0), st(1)                        ; := u(z) * A(y), keep u(z) on the stack
        fld     dword ptr [edi]                     ; := weight
        fmulp   st(4), st(0)                        ; := weight * (u(x) * R(x) + u(y) * U(x) + u(z) * A(x)), pop weight
        faddp   st(2), st(0)                        ; := u(x) * R(y) + u(y) * U(y) + u(z) * A(y)
        fld     dword ptr [ebx + mat_mzz]           ; := A(z)
        fmulp   st(1), st(0)                        ; := u(z) * A(z)
        fld     dword ptr [edi]                     ; := weight
        fmulp   st(2), st(0)                        ; := weight * (u(x) * R(y) + u(y) * U(y) + u(z) * A(y)), pop weight
        faddp   st(3), st(0)                        ; := u(x) * R(z) + u(y) * U(z) + u(z) * A(z)

        cmp     esi, 0                              ; compare esi (first weight or not?)
        jne     short blendNormals_firstweight

        fld     dword ptr [eax + _rwV3d_nX]         ; := v(x)
        faddp   st(2), st(0)                        ; := v(x) + weight * (u(x) * R(x) + u(y) * U(x) + u(z) * A(x)), pop v(x)
        fld     dword ptr [edi]                     ; := weight
        fmulp   st(3), st(0)                        ; := weight * (u(x) * R(z) + u(y) * U(z) + u(z) * A(z)), pop weight
        fld     dword ptr [eax + _rwV3d_nY]         ; := v(y)
        faddp   st(1), st(0)                        ; := v(y) + weight * (u(x) * R(y) + u(y) * U(y) + u(z) * A(y)), pop v(y)

        fxch    st(1)                               ; swap v(x) + weight * (u(x) * R(x) + u(y) * U(x) + u(z) * A(x)) back to st(0)
        fstp    dword ptr [eax + _rwV3d_nX]         ; v(x) = st(0)
        fld     dword ptr [eax + _rwV3d_nZ]         ; := v(z)
        faddp   st(2), st(0)                        ; := v(z) + weight * (u(x) * R(z) + u(y) * U(z) + u(z) * A(z)), pop v(z)
        fstp    dword ptr [eax + _rwV3d_nY]         ; v(y) = st(1)
        fstp    dword ptr [eax + _rwV3d_nZ]         ; v(z) = st(2)
        ret

blendNormals_firstweight:
        fxch    st(1)                               ; swap weight * (u(x) * R(x) + u(y) * U(x) + u(z) * A(x)) back to st(0)
        fstp    dword ptr [eax + _rwV3d_nX]         ; v(x) = st(0)
        fld     dword ptr [edi]                     ; := weight
        fmulp   st(2), st(0)                        ; := weight * (u(x) * R(z) + u(y) * U(z) + u(z) * A(z)), pop weight
        fstp    dword ptr [eax + _rwV3d_nY]         ; v(y) = st(1)
        fstp    dword ptr [eax + _rwV3d_nZ]         ; v(z) = st(2)
        ret

finishedSkin:
        pop     edx
        pop     ebx
        pop     eax                                 ; pop the most used register :)

        pop     esi                                 ; pop the stack index
        pop     edi                                 ; pop the data index
        pop     ecx                                 ; pop the counter
    }
#endif /* !defined(__GNUC__) */

    RWRETURNVOID();
}

#else /* !defined(NOASM) */

/****************************************************************************
 _rpSkinGenericMatrixBlend

 Purpose: Generic skin matrix blending

 Inputs :   pSkin->totalVertices        number of vertices
            pSkin->pMatrixWeightsMap    array of matrix weights
            pSkin->pMatrixIndexMap      array of matrix indices
            pMatrixArray                array of matrices
            pVertices                   array of vertices to write to
            pOriginalVertices           array of vertices to read from
            pNormals                    array of normals to write to
            pOriginalNormals            array of normals to read from

 Outputs:   Nothing
 *
 */
void
_rpSkinGenericMatrixBlend(RwInt32 numVertices,
                          const RwMatrixWeights *matrixWeightsMap,
                          const RwUInt32 *matrixIndexMap,
                          const RwMatrix *matrixArray,
                          RwUInt8 *vertices,
                          const RwV3d *originalVertices,
                          RwUInt8 *normals,
                          const RwV3d *originalNormals,
                          RwUInt32 stride)
{
    RwInt32 i;

    RWFUNCTION(RWSTRING("_rpSkinGenericMatrixBlend"));

    for ( i = 0;
        i < numVertices;
        i++,
        vertices += stride,
        normals += stride )
    {
        const RwUInt32 matrixIndices = matrixIndexMap[i];
        const RwMatrixWeights *matrixWeights = &(matrixWeightsMap[i]);
        const RwMatrix *matrix;

        /* Hideously slow matrix operations follow... */
        if (FLOATASINT(matrixWeights->w0) > 0)
        {
            matrix = &matrixArray[matrixIndices & 0xFF];

            MATRIXSKIN( ((RwV3d *)vertices),
                        matrix,
                        originalVertices[i],
                        matrixWeights->w0 );

            if(NULL != originalNormals)
            {
                MATRIXNORMALSKIN( ((RwV3d *)normals),
                                  matrix,
                                  originalNormals[i],
                                  matrixWeights->w0 );
            }
        }
        else
        {
            continue;
        }

        if (FLOATASINT(matrixWeights->w1) > 0)
        {
            matrix = &matrixArray[(matrixIndices >> 8) & 0xFF];

            MATRIXPLUSSKIN( ((RwV3d *)vertices),
                            matrix,
                            originalVertices[i],
                            matrixWeights->w1 );

            if(NULL != originalNormals)
            {
                MATRIXNORMALPLUSSKIN( ((RwV3d *)normals),
                                      matrix,
                                      originalNormals[i],
                                      matrixWeights->w1 );
            }
        }
        else
        {
            continue;
        }

        if (FLOATASINT(matrixWeights->w2) > 0)
        {
            matrix = &matrixArray[(matrixIndices >> 16) & 0xFF];

            MATRIXPLUSSKIN( ((RwV3d *)vertices),
                            matrix,
                            originalVertices[i],
                            matrixWeights->w2 );

            if(NULL != originalNormals)
            {
                MATRIXNORMALPLUSSKIN( ((RwV3d *)normals),
                                      matrix,
                                      originalNormals[i],
                                      matrixWeights->w2 );
            }
        }
        else
        {
            continue;
        }

        if (FLOATASINT(matrixWeights->w3) > 0)
        {
            matrix = &matrixArray[(matrixIndices >> 24) & 0xFF];

            MATRIXPLUSSKIN( ((RwV3d *)vertices),
                            matrix,
                            originalVertices[i],
                            matrixWeights->w3 );

            if(NULL != originalNormals)
            {
                MATRIXNORMALPLUSSKIN( ((RwV3d *)normals),
                                      matrix,
                                      originalNormals[i],
                                      matrixWeights->w3 );
            }
        }
        else
        {
            continue;
        }
    }

    RWRETURNVOID();
}

#endif /* !defined(NOASM) */
