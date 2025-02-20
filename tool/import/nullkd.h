
/***************************************************************************
 *                                                                         *
 * Module  : nhsscheme.h                                                   *
 *                                                                         *
 * Purpose : .                                                             *
 *                                                                         *
 **************************************************************************/

#ifndef NULLKD_H
#define NULLKD_H


#ifdef    __cplusplus
extern              "C"
{
#endif                          /* __cplusplus */

extern void
_rtWorldImportGuideKDPush(RtWorldImportGuideKDTree *tree, 
                         _rtWorldImportGuideKDStack *stack);

extern void
_rtWorldImportGuideKDPop(_rtWorldImportGuideKDStack *stack);

#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* RNULLKD_H */
