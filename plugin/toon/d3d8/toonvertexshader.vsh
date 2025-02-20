vs.1.1

; c0-4 projection matrix
; c5-7 world-view matrix

; get position in camera space
dp4 r0.x, v0, c4
dp4 r0.y, v0, c5
dp4 r0.z, v0, c6

; get normal in camera space
dp3 r1.x, v1, c4
dp3 r1.y, v1, c5

; get 2D normal length
mul r1.w, r1.x, r1.x
mad r1.w, r1.y, r1.y, r1.w
rsq r1.w, r1.w

; calculate ink transform
mad r0.w, r0.z, v3.x, v3.y
mul r0.w, r0.w, r0.z

; apply ink transform to 2D normal length
mul r1.w, r1.w, r0.w

; translate position in camera space
mad r0.xy, r1.xy, r1.w, r0.xy

; output transformed vertex
mov r0.w, v0.w
m4x4 oPos, r0, c0

mov oD0, v2
