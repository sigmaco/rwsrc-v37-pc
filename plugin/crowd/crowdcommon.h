/*
 *  crowdcommon.h
 *
 *  Common interface across all platforms.
 */

#ifndef CROWDCOMMON_H
#define CROWDCOMMON_H

#include "rwcore.h"
#include "rpplugin.h"
#include "rpdbgerr.h"
#include "rpcrowd.h"

/******************************************************************************
 *  Types
 */
typedef struct CrowdEngineData CrowdEngineData;
struct CrowdEngineData
{
    RxPipeline *pipe;
};

struct RpCrowd
{
    RpCrowdParam        param;
    RwUInt32            currTexture;
    RwUInt32            currAnim;
    RwReal              currAnimTime;
};

/******************************************************************************
 *  Global Variables 
 */
extern RwModuleInfo _rpCrowdModule;
extern RwInt32      _rpCrowdEngineDataOffset;
extern RwUInt32     _rpCrowdDeviceDataSize;

/******************************************************************************
 *  Macros
 */
#define CROWDENGINEGETDATA(engineInstance)  \
    ((CrowdEngineData *)                    \
     ((RwUInt8 *)engineInstance + _rpCrowdEngineDataOffset))
#define CROWDENGINEGETCONSTDATA(engineInstance) \
    ((const CrowdEngineData *)                  \
     ((const RwUInt8 *)engineInstance + _rpCrowdEngineDataOffset))

#define CROWDGETDEVICEDATA(_crowd) \
    ((void *)((RwUInt8 *)(_crowd) + sizeof(RpCrowd)))
#define CROWDGETCONSTDEVICEDATA(_crowd) \
    ((const void *)((const RwUInt8 *)(_crowd) + sizeof(RpCrowd)))

/******************************************************************************
 *  Functions
 */
extern RwBool       _rpCrowdDeviceOpen(void);
extern RwBool       _rpCrowdDeviceClose(void);
extern RpCrowd *    _rpCrowdDeviceInitCrowd(RpCrowd *crowd);
extern RwBool       _rpCrowdDeviceDeInitCrowd(RpCrowd *crowd);
extern RpCrowd *    _rpCrowdDeviceRenderCrowd(RpCrowd *crowd);

#endif /* CROWDCOMMON_H */

