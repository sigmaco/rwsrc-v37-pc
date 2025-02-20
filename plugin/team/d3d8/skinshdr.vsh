vs.1.1

#include "skindefs.h" 

;------------------------------------------------------------------------------
; Bone space transforms
;------------------------------------------------------------------------------

    ; Bone 0 ------------------------------------------------------------------------------------
    mov     a0.x, VSIN_REG_INDICES.x
    mul     VSTMP_REG_SKIN_TRANS_0, VSIN_REG_WEIGHTS.x, c[a0.x + 0 + VSCONST_REG_MATRIX_OFFSET]
    mul     VSTMP_REG_SKIN_TRANS_1, VSIN_REG_WEIGHTS.x, c[a0.x + 1 + VSCONST_REG_MATRIX_OFFSET]
    mul     VSTMP_REG_SKIN_TRANS_2, VSIN_REG_WEIGHTS.x, c[a0.x + 2 + VSCONST_REG_MATRIX_OFFSET]

    ; Bone 1 ------------------------------------------------------------------------------------
    mov     a0.x, VSIN_REG_INDICES.y
    mad     VSTMP_REG_SKIN_TRANS_0, VSIN_REG_WEIGHTS.y, c[a0.x + 0 + VSCONST_REG_MATRIX_OFFSET], VSTMP_REG_SKIN_TRANS_0
    mad     VSTMP_REG_SKIN_TRANS_1, VSIN_REG_WEIGHTS.y, c[a0.x + 1 + VSCONST_REG_MATRIX_OFFSET], VSTMP_REG_SKIN_TRANS_1
    mad     VSTMP_REG_SKIN_TRANS_2, VSIN_REG_WEIGHTS.y, c[a0.x + 2 + VSCONST_REG_MATRIX_OFFSET], VSTMP_REG_SKIN_TRANS_2

    ; Skin position
    dp4     VSTMP_REG_POS_ACCUM.x, VSTMP_REG_SKIN_TRANS_0, VSIN_REG_POS
    dp4     VSTMP_REG_POS_ACCUM.y, VSTMP_REG_SKIN_TRANS_1, VSIN_REG_POS
    dp4     VSTMP_REG_POS_ACCUM.z, VSTMP_REG_SKIN_TRANS_2, VSIN_REG_POS

    ; Skin normal
    dp3     VSTMP_REG_NORMAL_ACCUM.x, VSTMP_REG_SKIN_TRANS_0, VSIN_REG_NORMAL
    dp3     VSTMP_REG_NORMAL_ACCUM.y, VSTMP_REG_SKIN_TRANS_1, VSIN_REG_NORMAL
    dp3     VSTMP_REG_NORMAL_ACCUM.z, VSTMP_REG_SKIN_TRANS_2, VSIN_REG_NORMAL

    ; Copy w - should be 1 in VSIN_REG_POS by default, but can't go through blending stuff.
    mov		VSTMP_REG_POS_ACCUM.w, VSIN_REG_POS.w

;------------------------------------------------------------------------------
; Combined camera & projection matrix
;------------------------------------------------------------------------------

    dp4		oPos.x, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_X
    dp4		oPos.y, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_Y
    dp4		oPos.z, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_Z
    dp4     r11.x,  VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_W
; Can't read from the oPos register so I need this extra copy for the fog
    mov     oPos.w, r11.x

;------------------------------------------------------------------------------
; Fogging - remove it if you don't use it ever to save a cycle
;------------------------------------------------------------------------------

    mov     oFog.x, r11.x

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
