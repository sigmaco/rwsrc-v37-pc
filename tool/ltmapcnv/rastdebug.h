#ifndef LMRERASTER_H
#define LMRERASTER_H

extern void
_rtltmapUVRemaperNonGreedy(RwImage *dstImg, 
                            RwTexCoords *dstUV, 
                            RwImage *srcImg, 
                            RwTexCoords *srcUV, 
                            RwReal rejectionSize);

#endif /*LMRERASTER_H*/
