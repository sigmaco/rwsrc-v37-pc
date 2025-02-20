/*===========================================================================*
 *-                                                                         -*
 *-  Module  :   dmorphapi.h                                                -*
 *-                                                                         -*
 *-  Purpose :   Contains a list of files to parse to build documentation.  -*
 *-                                                                         -*
 *===========================================================================*/

/* RWPUBLIC */
#ifndef RPDMORPH_H
#define RPDMORPH_H

#include <rwcore.h>
#include <rpworld.h>

#if ((defined(GCN_DRVMODEL_H) || \
      defined(XBOX_DRVMODEL_H)))
#include "dmorphplatformapi.h"
#endif

#include "dmorph.h"
#include "dmphanim.h"
#include "dmphintp.h"
#include "dmphgeom.h"

#endif /* RPDMORPH_H */
/* RWPUBLICEND */
