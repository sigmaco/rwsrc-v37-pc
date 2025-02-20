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
#include "ssematbl.h"

/****************************************************************************
 Local Types
 */

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Defines
 */

#if !defined(NOASM) && !defined(NOSSEASM)

#define _SHUFFLE(fp3,fp2,fp1,fp0) (((fp3) << 6) | ((fp2) << 4) | \
                                     ((fp1) << 2) | ((fp0)))

#define FLOATASINT(f) (*((const RwInt32 *)&(f)))

#define X0X0X0X0      0x00
#define X1X1X1X1      0x55
#define X2X2X2X2      0xAA
#define X2X2X1X0      0xA4
#define X1X0X3X2      0x4E

/****************************************************************************
 Local (Static) variables
 */

static const RwReal One = 1.f;
static const RwReal Zero = 0.f;

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   Functions

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/****************************************************************************/

/****************************************************************************
 _rpSkinIntelSSEMatrixBlend1Matrix
 */
void
_rpSkinIntelSSEMatrixBlend1Matrix(RwInt32 numVertices __RWUNUSED__,
                                  const RwMatrix *matrix __RWUNUSED__,
                                  RwUInt8 *vertices __RWUNUSED__,
                                  const RwV3d *originalVertices __RWUNUSED__,
                                  RwUInt8 *normals __RWUNUSED__,
                                  const RwV3d *originalNormals __RWUNUSED__,
                                  RwUInt32 stride __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("_rpSkinIntelSSEMatrixBlend1Matrix"));

#if !defined(__GNUC__)
    __asm
    {
        mov     ecx, numVertices
        mov     ebx, matrix
        mov     edx, stride
        mov     esi, originalVertices
        mov     edi, originalNormals

        movaps  xmm3, [ebx]
        movaps  xmm4, [ebx + 16]
        movaps  xmm5, [ebx + 32]
        movaps  xmm6, [ebx + 48]

        mov     eax, vertices
        mov     ebx, normals

beginloop1Matrix:

        movss   xmm0, [esi]
        movss   xmm1, [esi + 4]
        movss   xmm2, [esi + 8]

        shufps  xmm0, xmm0, X0X0X0X0
        shufps  xmm1, xmm1, X0X0X0X0
        shufps  xmm2, xmm2, X0X0X0X0

        mulps   xmm0, xmm3
        mulps   xmm1, xmm4
        mulps   xmm2, xmm5

        addps   xmm1, xmm0
        addps   xmm2, xmm6

        add     esi, 12

        addps   xmm2, xmm1

        movhlps xmm0, xmm2
        movlps  [eax], xmm2
        movss   [eax + 8], xmm0

        add     eax, edx

        test    edi, edi
        jz      nonormals1Matrix

        movss   xmm0, [edi]
        movss   xmm1, [edi + 4]
        movss   xmm2, [edi + 8]

        shufps  xmm0, xmm0, X0X0X0X0
        shufps  xmm1, xmm1, X0X0X0X0
        shufps  xmm2, xmm2, X0X0X0X0

        mulps   xmm0, xmm3
        mulps   xmm1, xmm4
        mulps   xmm2, xmm5

        addps   xmm1, xmm0

        add     edi, 12

        addps   xmm2, xmm1

        movhlps xmm0, xmm2
        movlps  [ebx], xmm2
        movss   [ebx + 8], xmm0

        add     ebx, edx

nonormals1Matrix:

        dec ecx
        jnz beginloop1Matrix
    }
#endif /* !defined(__GNUC__) */

    RWRETURNVOID();
}

/****************************************************************************
 _rpSkinIntelSSEMatrixBlend1Weight
 */
void
_rpSkinIntelSSEMatrixBlend1Weight(RwInt32 numVertices __RWUNUSED__,
                                  const RwUInt32 *matrixIndexMap __RWUNUSED__,
                                  const RwMatrix *matrixArray __RWUNUSED__,
                                  RwUInt8 *vertices __RWUNUSED__,
                                  const RwV3d *originalVertices __RWUNUSED__,
                                  RwUInt8 *normals __RWUNUSED__,
                                  const RwV3d *originalNormals __RWUNUSED__,
                                  RwUInt32 stride __RWUNUSED__ )
{
    RWFUNCTION(RWSTRING("_rpSkinIntelSSEMatrixBlend1Weight"));

#if !defined(__GNUC__)
    __asm
    {
        mov     ecx, numVertices
        mov     edx, stride
        mov     esi, originalVertices
        mov     edi, originalNormals

beginloop1Matrix:

        movss   xmm0, [esi]
        movss   xmm1, [esi + 4]
        movss   xmm2, [esi + 8]

        shufps  xmm0, xmm0, X0X0X0X0
        shufps  xmm1, xmm1, X0X0X0X0
        shufps  xmm2, xmm2, X0X0X0X0

        mov     ebx, matrixIndexMap
        xor     eax, eax
        mov     al, BYTE PTR [ebx]
        add     ebx, 4
        mov     matrixIndexMap, ebx
        shl     eax, 6
        mov     ebx, matrixArray
        lea     ebx, [ebx + eax]

        movaps  xmm3, [ebx]
        movaps  xmm4, [ebx + 16]
        movaps  xmm5, [ebx + 32]
        movaps  xmm6, [ebx + 48]

        mulps   xmm0, xmm3
        mulps   xmm1, xmm4
        mulps   xmm2, xmm5

        addps   xmm1, xmm0
        addps   xmm2, xmm6

        mov     eax, vertices

        addps   xmm2, xmm1

        add     esi, 12

        movss   [eax], xmm2
        movhlps xmm0, xmm2
        shufps  xmm2, xmm2, _SHUFFLE(3,2,1,1)
        movss   [eax + 4], xmm2
        movss   [eax + 8], xmm0

        add     eax, edx
        mov     vertices, eax

        test    edi, edi
        jz      nonormals1Matrix

        movss   xmm0, [edi]
        movss   xmm1, [edi + 4]
        movss   xmm2, [edi + 8]

        shufps  xmm0, xmm0, X0X0X0X0
        shufps  xmm1, xmm1, X0X0X0X0
        shufps  xmm2, xmm2, X0X0X0X0

        mulps   xmm0, xmm3
        mulps   xmm1, xmm4
        mulps   xmm2, xmm5

        addps   xmm1, xmm0

        mov     eax, normals

        addps   xmm2, xmm1

        add     edi, 12

        movss   [eax], xmm2
        movhlps xmm0, xmm2
        shufps  xmm2, xmm2, _SHUFFLE(3,2,1,1)
        movss   [eax + 4], xmm2
        movss   [eax + 8], xmm0

        add     eax, edx
        mov     normals, eax

nonormals1Matrix:

        dec ecx
        jnz beginloop1Matrix
    }
#endif /* !defined(__GNUC__) */

    RWRETURNVOID();
}

/****************************************************************************
 _rpSkinIntelSSEMatrixBlend

 SSE assembler encapsulated matrix blending.

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
_rpSkinIntelSSEMatrixBlend( RwInt32 numVertices __RWUNUSED__,
                            const RwMatrixWeights *matrixWeightsMap __RWUNUSED__,
                            const RwUInt32 *matrixIndexMap __RWUNUSED__,
                            const RwMatrix *matrixArray __RWUNUSED__,
                            RwUInt8 *vertices __RWUNUSED__,
                            const RwV3d *originalVertices __RWUNUSED__,
                            RwUInt8 *normals __RWUNUSED__,
                            const RwV3d *originalNormals __RWUNUSED__,
                            RwUInt32 stride __RWUNUSED__ )
{
#if !defined(__GNUC__)
    __declspec(align(16)) RwReal weights[4];
#endif /* !defined(__GNUC__) */

    RWFUNCTION(RWSTRING("_rpSkinIntelSSEMatrixBlend"));

#if !defined(__GNUC__)
    __asm
    {
        mov     ecx, numVertices
        mov     edi, matrixWeightsMap
        mov     esi, matrixIndexMap

beginloop:
        mov     eax, [edi]
        cmp     eax, One
        jb      has2weights

        mov     eax, [esi]
        mov     edx, matrixArray
        and     eax, 0xff
        shl     eax, 6
        add     edx, eax

        mov     eax, originalVertices

        movss   xmm0, [eax]
        movss   xmm1, [eax + 4]
        movss   xmm2, [eax + 8]

        shufps  xmm0, xmm0, X0X0X0X0
        shufps  xmm1, xmm1, X0X0X0X0
        shufps  xmm2, xmm2, X0X0X0X0

        movaps  xmm3, [edx]
        movaps  xmm4, [edx + 16]
        movaps  xmm5, [edx + 32]
        movaps  xmm6, [edx + 48]

        mulps   xmm0, xmm3
        mulps   xmm1, xmm4
        mulps   xmm2, xmm5

        addps   xmm1, xmm0
        addps   xmm2, xmm6

        add     eax, 12
        mov     originalVertices, eax

        addps   xmm2, xmm1

        mov     eax, vertices
        movhlps xmm0, xmm2
        movss   [eax], xmm2
        shufps  xmm2, xmm2, _SHUFFLE(3,2,1,1)
        movss   [eax + 4], xmm2
        movss   [eax + 8], xmm0

        add     eax, stride
        mov     vertices, eax

        mov     eax, originalNormals
        test    eax, eax
        jz      nonormals1

        movss   xmm0, [eax]
        movss   xmm1, [eax + 4]
        movss   xmm2, [eax + 8]

        shufps  xmm0, xmm0, X0X0X0X0
        shufps  xmm1, xmm1, X0X0X0X0
        shufps  xmm2, xmm2, X0X0X0X0

        add     eax, 12

        mulps   xmm0, xmm3
        mulps   xmm1, xmm4
        mulps   xmm2, xmm5

        addps   xmm1, xmm0

        mov     originalNormals, eax

        addps   xmm2, xmm1

        mov     eax, normals
        movhlps xmm0, xmm2
        movss   [eax], xmm2
        shufps  xmm2, xmm2, _SHUFFLE(3,2,1,1)
        movss   [eax + 4], xmm2
        movss   [eax + 8], xmm0

        add     eax, stride
        mov     normals, eax

nonormals1:
        add     edi, 16
        add     esi, 4
        dec     ecx
        jnz     beginloop

        jmp     endloop

has2weights:

        movss   xmm0, [edi]

        mov     eax, [esi]

        shufps  xmm0, xmm0, X0X0X0X0

        mov     edx, matrixArray
        and     eax, 0xff

        movaps  weights, xmm0

        shl     eax, 6
        add     edx, eax

        movaps  xmm0, [edx]
        movaps  xmm1, [edx + 16]
        movaps  xmm2, [edx + 32]
        movaps  xmm3, [edx + 48]

        mulps   xmm0, weights
        mulps   xmm1, weights
        mulps   xmm2, weights
        mulps   xmm3, weights

        movss   xmm4, [edi + 4]

        mov     eax, [esi]

        shufps  xmm4, xmm4, X0X0X0X0

        and     eax, 0xff00

        movaps  weights, xmm4

        mov     edx, matrixArray
        shr     eax, 2
        add     edx, eax

        movaps  xmm4, [edx]
        movaps  xmm5, [edx + 16]
        movaps  xmm6, [edx + 32]
        movaps  xmm7, [edx + 48]

        mulps   xmm4, weights
        mulps   xmm5, weights
        mulps   xmm6, weights
        mulps   xmm7, weights

        addps   xmm4, xmm0
        addps   xmm5, xmm1
        addps   xmm6, xmm2
        addps   xmm7, xmm3

        movss   xmm0, [edi + 8]
        ucomiss xmm0, Zero
        jbe     transformvertex

        shufps  xmm0, xmm0, X0X0X0X0

        mov     eax, [esi]

        movaps  weights, xmm0

        and     eax, 0xff0000
        mov     edx, matrixArray
        shr     eax, 10
        add     edx, eax

        movaps  xmm0, [edx]
        movaps  xmm1, [edx + 16]
        movaps  xmm2, [edx + 32]
        movaps  xmm3, [edx + 48]

        mulps   xmm0, weights
        mulps   xmm1, weights
        mulps   xmm2, weights
        mulps   xmm3, weights

        addps   xmm4, xmm0
        addps   xmm5, xmm1
        addps   xmm6, xmm2
        addps   xmm7, xmm3

        movss   xmm0, [edi + 12]
        ucomiss xmm0, Zero
        jbe     transformvertex

        shufps  xmm0, xmm0, X0X0X0X0

        mov     eax, [esi]

        movaps  weights, xmm0

        and     eax, 0xff000000
        mov     edx, matrixArray
        shr     eax, 18
        add     edx, eax

        movaps  xmm0, [edx]
        movaps  xmm1, [edx + 16]
        movaps  xmm2, [edx + 32]
        movaps  xmm3, [edx + 48]

        mulps   xmm0, weights
        mulps   xmm1, weights
        mulps   xmm2, weights
        mulps   xmm3, weights

        addps   xmm4, xmm0
        addps   xmm5, xmm1
        addps   xmm6, xmm2
        addps   xmm7, xmm3

        /**/
transformvertex:
        mov     eax, originalVertices

        movss  xmm0, [eax]
        movss  xmm1, [eax + 4]
        movss  xmm2, [eax + 8]

        shufps  xmm0, xmm0, X0X0X0X0
        shufps  xmm1, xmm1, X0X0X0X0
        shufps  xmm2, xmm2, X0X0X0X0

        mulps   xmm0, xmm4
        mulps   xmm1, xmm5
        mulps   xmm2, xmm6

        addps   xmm1, xmm0
        addps   xmm2, xmm7

        add     eax, 12
        mov     originalVertices, eax

        addps   xmm2, xmm1

        mov     eax, vertices
        movss   [eax], xmm2
        movhlps xmm0, xmm2
        shufps  xmm2, xmm2, _SHUFFLE(3,2,1,1)
        movss   [eax + 4], xmm2
        movss   [eax + 8], xmm0

        add     eax, stride
        mov     vertices, eax

        mov     eax, originalNormals
        test    eax, eax
        jz      nextloop

        movss  xmm0, [eax]
        movss  xmm1, [eax + 4]
        movss  xmm2, [eax + 8]

        shufps  xmm0, xmm0, X0X0X0X0
        shufps  xmm1, xmm1, X0X0X0X0
        shufps  xmm2, xmm2, X0X0X0X0

        mulps   xmm0, xmm4
        mulps   xmm1, xmm5
        mulps   xmm2, xmm6

        addps   xmm1, xmm0

        add     eax, 12
        mov     originalNormals, eax

        addps   xmm2, xmm1

        mov     eax, normals
        movss   [eax], xmm2
        movhlps xmm0, xmm2
        shufps  xmm2, xmm2, _SHUFFLE(3,2,1,1)
        movss   [eax + 4], xmm2
        movss   [eax + 8], xmm0

        add     eax, stride
        mov     normals, eax

nextloop:
        add     edi, 16
        add     esi, 4
        dec     ecx
        jnz     beginloop

endloop:
    }
#endif /* !defined(__GNUC__) */

    RWRETURNVOID();
}

/****************************************************************************
 _rpSkinIntelSSEMatrixBlend2Weights

 SSE assembler encapsulated matrix blending.
 *
 */
void
_rpSkinIntelSSEMatrixBlend2Weights( RwInt32 numVertices __RWUNUSED__,
                            const RwMatrixWeights *matrixWeightsMap __RWUNUSED__,
                            const RwUInt32 *matrixIndexMap __RWUNUSED__,
                            const RwMatrix *matrixArray __RWUNUSED__,
                            RwUInt8 *vertices __RWUNUSED__,
                            const RwV3d *originalVertices __RWUNUSED__,
                            RwUInt8 *normals __RWUNUSED__,
                            const RwV3d *originalNormals __RWUNUSED__,
                            RwUInt32 stride __RWUNUSED__ )
{
#if !defined(__GNUC__)
     __declspec(align(16)) RwReal weights[4];
#endif /* !defined(__GNUC__) */

    RWFUNCTION(RWSTRING("_rpSkinIntelSSEMatrixBlend2Weights"));

#if !defined(__GNUC__)
    __asm
    {
        mov     ecx, numVertices
        mov     edi, matrixWeightsMap
        mov     esi, matrixIndexMap

beginloop:
        mov     eax, [edi]
        cmp     eax, One
        jb      has2weights

        mov     eax, [esi]
        mov     edx, matrixArray
        and     eax, 0xff
        shl     eax, 6
        add     edx, eax

        mov     eax, originalVertices

        movss   xmm0, [eax]
        movss   xmm1, [eax + 4]
        movss   xmm2, [eax + 8]

        shufps  xmm0, xmm0, X0X0X0X0
        shufps  xmm1, xmm1, X0X0X0X0
        shufps  xmm2, xmm2, X0X0X0X0

        movaps  xmm3, [edx]
        movaps  xmm4, [edx + 16]
        movaps  xmm5, [edx + 32]
        movaps  xmm6, [edx + 48]

        mulps   xmm0, xmm3
        mulps   xmm1, xmm4
        mulps   xmm2, xmm5

        addps   xmm1, xmm0
        addps   xmm2, xmm6

        add     eax, 12
        mov     originalVertices, eax

        addps   xmm2, xmm1

        mov     eax, vertices
        movhlps xmm0, xmm2
        movss   [eax], xmm2
        shufps  xmm2, xmm2, _SHUFFLE(3,2,1,1)
        movss   [eax + 4], xmm2
        movss   [eax + 8], xmm0

        add     eax, stride
        mov     vertices, eax

        mov     eax, originalNormals
        test    eax, eax
        jz      nonormals1

        movss   xmm0, [eax]
        movss   xmm1, [eax + 4]
        movss   xmm2, [eax + 8]

        shufps  xmm0, xmm0, X0X0X0X0
        shufps  xmm1, xmm1, X0X0X0X0
        shufps  xmm2, xmm2, X0X0X0X0

        mulps   xmm0, xmm3
        mulps   xmm1, xmm4
        mulps   xmm2, xmm5

        addps   xmm1, xmm0

        add     eax, 12
        mov     originalNormals, eax

        addps   xmm2, xmm1

        mov     eax, normals
        movhlps xmm0, xmm2
        movss   [eax], xmm2
        shufps  xmm2, xmm2, _SHUFFLE(3,2,1,1)
        movss   [eax + 4], xmm2
        movss   [eax + 8], xmm0

        add     eax, stride
        mov     normals, eax

nonormals1:
        add     edi, 16
        add     esi, 4
        dec     ecx
        jnz     beginloop

        jmp     endloop

has2weights:

        movss   xmm0, [edi]

        mov     eax, [esi]

        shufps  xmm0, xmm0, X0X0X0X0

        and     eax, 0xff

        movaps  weights, xmm0

        mov     edx, matrixArray
        shl     eax, 6
        add     eax, edx

        movaps  xmm0, [eax]
        movaps  xmm1, [eax + 16]
        movaps  xmm2, [eax + 32]
        movaps  xmm3, [eax + 48]

        mulps   xmm0, weights
        mulps   xmm1, weights
        mulps   xmm2, weights
        mulps   xmm3, weights

        movss   xmm4, [edi + 4]

        mov     eax, [esi]

        shufps  xmm4, xmm4, X0X0X0X0

        and     eax, 0xff00

        movaps  weights, xmm4

        shr     eax, 2
        add     eax, edx

        movaps  xmm4, [eax]
        movaps  xmm5, [eax + 16]
        movaps  xmm6, [eax + 32]
        movaps  xmm7, [eax + 48]

        mulps   xmm4, weights
        mulps   xmm5, weights
        mulps   xmm6, weights
        mulps   xmm7, weights

        addps   xmm4, xmm0
        addps   xmm5, xmm1
        addps   xmm6, xmm2
        addps   xmm7, xmm3

        /**/
        mov     eax, originalVertices

        movss  xmm0, [eax]
        movss  xmm1, [eax + 4]
        movss  xmm2, [eax + 8]

        shufps  xmm0, xmm0, X0X0X0X0
        shufps  xmm1, xmm1, X0X0X0X0
        shufps  xmm2, xmm2, X0X0X0X0

        mulps   xmm0, xmm4
        mulps   xmm1, xmm5
        mulps   xmm2, xmm6

        addps   xmm1, xmm0
        addps   xmm2, xmm7

        add     eax, 12
        mov     originalVertices, eax

        addps   xmm2, xmm1

        mov     eax, vertices
        movss   [eax], xmm2
        movhlps xmm0, xmm2
        shufps  xmm2, xmm2, _SHUFFLE(3,2,1,1)
        movss   [eax + 4], xmm2
        movss   [eax + 8], xmm0

        add     eax, stride
        mov     vertices, eax

        mov     eax, originalNormals
        test    eax, eax
        jz      nextloop

        movss  xmm0, [eax]
        movss  xmm1, [eax + 4]
        movss  xmm2, [eax + 8]

        shufps  xmm0, xmm0, X0X0X0X0
        shufps  xmm1, xmm1, X0X0X0X0
        shufps  xmm2, xmm2, X0X0X0X0

        mulps   xmm0, xmm4
        mulps   xmm1, xmm5
        mulps   xmm2, xmm6

        addps   xmm1, xmm0

        add     eax, 12
        mov     originalNormals, eax

        addps   xmm2, xmm1

        mov     eax, normals
        movss   [eax], xmm2
        movhlps xmm0, xmm2
        shufps  xmm2, xmm2, _SHUFFLE(3,2,1,1)
        movss   [eax + 4], xmm2
        movss   [eax + 8], xmm0

        add     eax, stride
        mov     normals, eax

nextloop:
        add     edi, 16
        add     esi, 4
        dec     ecx
        jnz     beginloop

endloop:
    }
#endif /* !defined(__GNUC__) */

    RWRETURNVOID();
}

#endif /* !defined(NOASM) && !defined(NOSSEASM) */
