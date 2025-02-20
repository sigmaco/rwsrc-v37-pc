vs_2_0

#include "skindefs.h"

dcl_position0 VSIN_REG_POS
dcl_blendweight0 VSIN_REG_WEIGHTS
dcl_blendindices0 VSIN_REG_INDICES

;------------------------------------------------------------------------------
; Bone space transforms
;------------------------------------------------------------------------------

    ; Get matrix indices
    mova    a0.xy, VSIN_REG_INDICES

    ; Transform position
    dp4		VSTMP_REG_POS_TMP.x, VSIN_REG_POS, c[a0.x + 0 + VSCONST_REG_MATRIX_OFFSET]
    dp4		VSTMP_REG_POS_TMP.y, VSIN_REG_POS, c[a0.x + 1 + VSCONST_REG_MATRIX_OFFSET]
    dp4		VSTMP_REG_POS_TMP.z, VSIN_REG_POS, c[a0.x + 2 + VSCONST_REG_MATRIX_OFFSET]

    ; Scale transformed point by weight
    mul     VSTMP_REG_POS_ACCUM.xyz, VSTMP_REG_POS_TMP.xyz, VSIN_REG_WEIGHTS.x

;------------------------------------------------------------------------------

    ; Transform position
    dp4		VSTMP_REG_POS_TMP.x, VSIN_REG_POS, c[a0.y + 0 + VSCONST_REG_MATRIX_OFFSET]
    dp4		VSTMP_REG_POS_TMP.y, VSIN_REG_POS, c[a0.y + 1 + VSCONST_REG_MATRIX_OFFSET]
    dp4		VSTMP_REG_POS_TMP.z, VSIN_REG_POS, c[a0.y + 2 + VSCONST_REG_MATRIX_OFFSET]

    ; Scale transformed point by weight and add to previous
    mad     VSTMP_REG_POS_ACCUM.xyz, VSTMP_REG_POS_TMP.xyz, VSIN_REG_WEIGHTS.y, VSTMP_REG_POS_ACCUM.xyz

;------------------------------------------------------------------------------

    ; Copy w
    mov		VSTMP_REG_POS_ACCUM.w, VSIN_REG_POS.w

;------------------------------------------------------------------------------
; Combined camera & projection matrix
;------------------------------------------------------------------------------

    dp4		oPos.x, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_X
    dp4		oPos.y, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_Y
    dp4		oPos.z, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_Z
    dp4     oPos.w, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_W
