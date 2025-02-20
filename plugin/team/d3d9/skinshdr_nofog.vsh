vs.1.1

#include "skindefs.h" 

dcl_position0 VSIN_REG_POS
dcl_blendweight0 VSIN_REG_WEIGHTS
dcl_blendindices0 VSIN_REG_INDICES
dcl_normal0 VSIN_REG_NORMAL
dcl_texcoord0 VSIN_REG_TEXCOORDS

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

    ; Transform normal
    dp3		VSTMP_REG_NORMAL_TMP.x, VSIN_REG_NORMAL, c[a0.x + 0 + VSCONST_REG_MATRIX_OFFSET]
    dp3		VSTMP_REG_NORMAL_TMP.y, VSIN_REG_NORMAL, c[a0.x + 1 + VSCONST_REG_MATRIX_OFFSET]
    dp3		VSTMP_REG_NORMAL_TMP.z, VSIN_REG_NORMAL, c[a0.x + 2 + VSCONST_REG_MATRIX_OFFSET]

    ; Scale transformed point by weight
    mul     VSTMP_REG_NORMAL_ACCUM.xyz, VSTMP_REG_NORMAL_TMP.xyz, VSIN_REG_WEIGHTS.x

;------------------------------------------------------------------------------

    ; Get matrix index 2
    mov     a0.x, VSIN_REG_INDICES.y

    ; Transform position
    dp4		VSTMP_REG_POS_TMP.x, VSIN_REG_POS, c[a0.x + 0 + VSCONST_REG_MATRIX_OFFSET]
    dp4		VSTMP_REG_POS_TMP.y, VSIN_REG_POS, c[a0.x + 1 + VSCONST_REG_MATRIX_OFFSET]
    dp4		VSTMP_REG_POS_TMP.z, VSIN_REG_POS, c[a0.x + 2 + VSCONST_REG_MATRIX_OFFSET]

    ; Scale transformed point by weight and add to previous
    mad     VSTMP_REG_POS_ACCUM.xyz, VSTMP_REG_POS_TMP.xyz, VSIN_REG_WEIGHTS.y, VSTMP_REG_POS_ACCUM.xyz

    ; Transform normal
    dp3		VSTMP_REG_NORMAL_TMP.x, VSIN_REG_NORMAL, c[a0.x + 0 + VSCONST_REG_MATRIX_OFFSET]
    dp3		VSTMP_REG_NORMAL_TMP.y, VSIN_REG_NORMAL, c[a0.x + 1 + VSCONST_REG_MATRIX_OFFSET]
    dp3		VSTMP_REG_NORMAL_TMP.z, VSIN_REG_NORMAL, c[a0.x + 2 + VSCONST_REG_MATRIX_OFFSET]

    ; Scale transformed point by weight
    mad     VSTMP_REG_NORMAL_ACCUM.xyz, VSTMP_REG_NORMAL_TMP.xyz, VSIN_REG_WEIGHTS.y, VSTMP_REG_NORMAL_ACCUM.xyz

;------------------------------------------------------------------------------

    ; Copy w - should be 1 in VSIN_REG_POS by default, but can't go through blending stuff.
    mov		VSTMP_REG_POS_ACCUM.w, VSIN_REG_POS.w

;------------------------------------------------------------------------------
; Combined camera & projection matrix
;------------------------------------------------------------------------------

    dp4		oPos.x, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_X
    dp4		oPos.y, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_Y
    dp4		oPos.z, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_Z
    dp4     oPos.w,  VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_W

;------------------------------------------------------------------------------
; Single directional light + Ambient
;------------------------------------------------------------------------------

    ; Normalize
    dp3     VSTMP_REG_NORMAL_ACCUM.w, VSTMP_REG_NORMAL_ACCUM, VSTMP_REG_NORMAL_ACCUM
    rsq     VSTMP_REG_NORMAL_ACCUM.w, VSTMP_REG_NORMAL_ACCUM.w
    mul     VSTMP_REG_NORMAL_ACCUM.xyz, VSTMP_REG_NORMAL_ACCUM.xyz, VSTMP_REG_NORMAL_ACCUM.w

    ; DP normal & light dir clamp then scale by light color
    dp3     VSTMP_REG_NORMAL_ACCUM.w, VSTMP_REG_NORMAL_ACCUM, VSCONST_REG_LIGHT_DIR
    max     VSTMP_REG_NORMAL_ACCUM.w, VSCONST_REG_LIGHT_DIR.w, -VSTMP_REG_NORMAL_ACCUM.w

;------------------------------------------------------------------------------
;   Multiply diffuse light color by diffuse light coefficient & add ambient
;------------------------------------------------------------------------------

    mov     VSTMP_REG_COLOR_TMP, VSCONST_REG_AMBIENT

    mad     oD0.xyz, VSCONST_REG_LIGHT_COLOR.xyz, VSTMP_REG_NORMAL_ACCUM.w, VSTMP_REG_COLOR_TMP.xyz
    mov     oD0.w, VSCONST_REG_AMBIENT.w

;------------------------------------------------------------------------------
; Copy texture coordinates
;------------------------------------------------------------------------------

    mov     oT0, VSIN_REG_TEXCOORDS

;------------------------------------------------------------------------------
; And replicate texture coordinates to 2nd stage for decal texture
;------------------------------------------------------------------------------

    mov     oT1, VSIN_REG_TEXCOORDS
