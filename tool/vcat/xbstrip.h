//-----------------------------------------------------------------------------
// File: XBStrip.h
//
// Desc: Tristrip routines (which convert a mesh into a list of optimized
//       triangle strips).
//
// Hist: 02.01.01 - New for March XDK release
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------




//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define OPTIMIZE_FOR_CACHE      0x00
#define OPTIMIZE_FOR_INDICES    0x01
#define OUTPUT_TRISTRIP         0x00
#define OUTPUT_TRILIST          0x02




//-----------------------------------------------------------------------------
// Name: Stripify()
// Desc: Main stripify routine. Stripifies a mesh and returns the number of 
//       strip indices contained in ppStripIndices.
// Note: Caller must make sure to call delete[] on the ppStripIndices array
//       when finished with it.
//-----------------------------------------------------------------------------
RwUInt32
Stripify( RwUInt32  dwNumTriangles,   // Number of triangles
          RwUInt16  *pTriangles,       // Ptr to triangle indices
          RwUInt32  *pdwNumIndices,    // Number of output indices
          RwUInt16  **ppStripIndices,   // Output indices
          RwUInt32  dwFlags = 0);    // Flags controlling optimizer.
