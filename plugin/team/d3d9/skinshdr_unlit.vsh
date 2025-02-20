vs.1.1

#include "skindefs.h"

dcl_position0 VSIN_REG_POS
dcl_blendweight0 VSIN_REG_WEIGHTS
dcl_blendindices0 VSIN_REG_INDICES

;------------------------------------------------------------------------------
; Bone space transforms
;------------------------------------------------------------------------------

    ; Get matrix index 1
    mov     a0.x, VSIN_REG_INDICES.x

    ; Transform position
    dp4		VSTMP_REG_POS_TMP.x, VSIN_REG_POS, c[a0.x + 0 + VSCONST_REG_MATRIX_OFFSET]
    dp4		VSTMP_REG_POS_TMP.y, VSIN_REG_POS, c[a0.x + 1 + VSCONST_REG_MATRIX_OFFSET]
    dp4		VSTMP_REG_POS_TMP.z, VSIN_REG_POS, c[a0.x + 2 + VSCONST_REG_MATRIX_OFFSET]

    ; Scale transformed point by weight
    mul     VSTMP_REG_POS_ACCUM.xyz, VSTMP_REG_POS_TMP.xyz, VSIN_REG_WEIGHTS.x

;------------------------------------------------------------------------------

    ; Get matrix index 2
    mov     a0.x, VSIN_REG_INDICES.y

    ; Transform position
    dp4		VSTMP_REG_POS_TMP.x, VSIN_REG_POS, c[a0.x + 0 + VSCONST_REG_MATRIX_OFFSET]
    dp4		VSTMP_REG_POS_TMP.y, VSIN_REG_POS, c[a0.x + 1 + VSCONST_REG_MATRIX_OFFSET]
    dp4		VSTMP_REG_POS_TMP.z, VSIN_REG_POS, c[a0.x + 2 + VSCONST_REG_MATRIX_OFFSET]

    ; Scale transformed point by weight and add to previous
    mad     VSTMP_REG_POS_ACCUM.xyz, VSTMP_REG_POS_TMP.xyz, VSIN_REG_WEIGHTS.y, VSTMP_REG_POS_ACCUM.xyz

;------------------------------------------------------------------------------

    ; Copy w
    mov		VSTMP_REG_POS_ACCUM.w, VSIN_REG_POS.w

;------------------------------------------------------------------------------
; Combined camera & projection matrix
;------------------------------------------------------------------------------

    dp4     r11.x, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_W
    dp4		oPos.x, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_X
    dp4		oPos.y, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_Y
    dp4		oPos.z, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_Z
; Can't read from the oPos register so I need this extra copy for the fog
    mov     oPos.w, r11.x

;------------------------------------------------------------------------------
; Fogging - remove it if you don't use it ever to save a cycle
;------------------------------------------------------------------------------

    mov     oFog, r11.x
