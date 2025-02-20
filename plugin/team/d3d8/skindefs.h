#ifndef SKIN_DEFS_H
#define SKIN_DEFS_H

/* For use in the vertex shader descriptor */
#define VSD_REG_POS                     0
#define VSD_REG_WEIGHTS                 1
#define VSD_REG_INDICES                 2
#define VSD_REG_NORMAL                  3
#define VSD_REG_COLOR                   4
#define VSD_REG_TEXCOORDS               5
#define VSD_REG_TEXCOORDS2              6


/* Input register - For use in the vertex shader code */
#define VSIN_REG_POS                    v0
#define VSIN_REG_WEIGHTS                v1
#define VSIN_REG_INDICES                v2
#define VSIN_REG_NORMAL                 v3
#define VSIN_REG_COLOR                  v4
#define VSIN_REG_TEXCOORDS              v5
#define VSIN_REG_TEXCOORDS2             v6


/* Temporary register - For use in the vertex shader code */
#define VSTMP_REG_POS_TMP               r0
#define VSTMP_REG_POS_ACCUM             r1
#define VSTMP_REG_NORMAL_TMP            r2
#define VSTMP_REG_NORMAL_ACCUM          r3
#define VSTMP_REG_SKIN_TRANS_0          r6
#define VSTMP_REG_SKIN_TRANS_1          r7
#define VSTMP_REG_SKIN_TRANS_2          r8
#define VSTMP_REG_COLOR_TMP             r10

/* Vertex shader defines */

/* always need a transform so start him on top */
#define VSCONST_REG_TRANSFORM_OFFSET    0
#define VSCONST_REG_TRANSFORM_SIZE      4

/* optionally need an ambient light */
#define VSCONST_REG_AMBIENT_OFFSET      4
#define VSCONST_REG_AMBIENT_SIZE        1

/* optionally need some directional lights */
#define VSCONST_REG_DIR_LIGHT_OFFSET    5
#define VSCONST_REG_DIR_LIGHT_SIZE      2

/* Envmap texture matrix transform */
#define VSCONST_REG_ENV_OFFSET          7
#define VSCONST_REG_ENV_SIZE            2

/* grow bone matrices upwards from bottom in vertex shader constants */
#define VSCONST_REG_MATRIX_OFFSET       9

/* Constant register - For use in the vertex shader code */
#define VSCONST_REG_TRANSFORM_X         c[0 + VSCONST_REG_TRANSFORM_OFFSET]
#define VSCONST_REG_TRANSFORM_Y         c[1 + VSCONST_REG_TRANSFORM_OFFSET]
#define VSCONST_REG_TRANSFORM_Z         c[2 + VSCONST_REG_TRANSFORM_OFFSET]
#define VSCONST_REG_TRANSFORM_W         c[3 + VSCONST_REG_TRANSFORM_OFFSET]

#define VSCONST_REG_AMBIENT             c[VSCONST_REG_AMBIENT_OFFSET]

#define VSCONST_REG_LIGHT_DIR           c[VSCONST_REG_DIR_LIGHT_OFFSET]
#define VSCONST_REG_LIGHT_COLOR         c[1 + VSCONST_REG_DIR_LIGHT_OFFSET]

#endif
