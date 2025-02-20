#ifndef RASTERG_H
#define RASTERG_H

extern void
_rtltmapUVRemaperGreedy(RwImage *dstImg, 
                            RwTexCoords *dstUV, 
                            RwImage *srcImg, 
                            RwTexCoords *srcUV, 
                            RwReal rejectionSize);

#endif /*RASTERG_H*/
