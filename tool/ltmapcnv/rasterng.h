#ifndef RASTERNG_H
#define RASTERNG_H

extern void
_rtltmapUVRemaperNonGreedy(RwImage *dstImg, 
                            RwTexCoords *dstUV, 
                            RwImage *srcImg, 
                            RwTexCoords *srcUV, 
                            RwReal rejectionSize);

#endif /*RASTERNG_H*/
