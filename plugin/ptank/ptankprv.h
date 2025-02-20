/**********************************************************************
 *
 * File :     ptankprv.h
 *
 * Abstract : PTank plugin private header
 *
 **********************************************************************
 *
 * This file is a product of Criterion Software Ltd.
 *
 * This file is provided as is with no warranties of any kind and is
 * provided without any obligation on Criterion Software Ltd. or
 * Canon Inc. to assist in its use or modification.
 *
 * Criterion Software Ltd. will not, under any
 * circumstances, be liable for any lost revenue or other damages arising
 * from the use of this file.
 *
 * Copyright (c) 1998 Criterion Software Ltd.
 * All Rights Reserved.
 *
 * RenderWare is a trademark of Canon Inc.
 *
 ************************************************************************/

#ifndef PTANKPRV_H
#define PTANKPRV_H


/*--- Include files ---*/
#include "rwcore.h"


/* Warning messages */
#define NOPOSMSG \
(RWSTRING("No position specifed for PTank, rpPTANKDFLAGPOSITION added"))

#define POSANDMATMSG \
(RWSTRING("rpPTANKDFLAGPOSITION and rpPTANKDFLAGMATRIX submited. \
rpPTANKDFLAGPOSITION is ignored"))

#define SIZANDMATMSG \
(RWSTRING("rpPTANKDFLAGSIZE and rpPTANKDFLAGMATRIX submited. \
rpPTANKDFLAGSIZE is ignored"))

#define ROTANDMATMSG \
(RWSTRING("rpPTANKDFLAGROTATE and rpPTANKDFLAGMATRIX submited. \
rpPTANKDFLAGROTATE is ignored"))

#define LOCKABORTMSG \
(RWSTRING("Lock was aborted as data piece type is not supported"))


#define MATCONSCONFLICT \
(RWSTRING("rpPTANKDFLAGMATRIX and rpPTANKDFLAGCNSMATRIX conflict"))

#define ROTCONSCONFLICT \
(RWSTRING("rpPTANKDFLAG2DROTATE and rpPTANKDFLAGCNS2DROTATE conflict"))

#define VCOCONSCONFLICT \
(RWSTRING("rpPTANKDFLAGVTXCOLOR and rpPTANKDFLAGCNSVTXCOLOR conflict"))

#define V2XCONSCONFLICT \
(RWSTRING("rpPTANKDFLAGVTX2TEXCOORDS and rpPTANKDFLAGCNSVTX2TEXCOORDS conflict"))

#define V4XCONSCONFLICT \
(RWSTRING("rpPTANKDFLAGVTX4TEXCOORDS and rpPTANKDFLAGCNSVTX4TEXCOORDS conflict"))


/*--- Plugin API Functions ---*/

#ifdef    __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/* Are normally in the platform files */
extern void        *
PTankOpen(void *object,
                    RwInt32 __RWUNUSED__ offset,
                    RwInt32 __RWUNUSED__ size);
extern void        *
PTankClose(void *object,
                    RwInt32 __RWUNUSED__ offset,
                    RwInt32 __RWUNUSED__ size);


#ifdef    __cplusplus
}
#endif                          /* __cplusplus */

#endif /* PTANKPRV_H */

