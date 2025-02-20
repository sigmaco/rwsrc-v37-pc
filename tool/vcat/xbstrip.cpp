//-----------------------------------------------------------------------------
// File: XBStrip.cpp
//
// Desc: Tristrip routines (which convert a mesh into a list of optimized
//       triangle strips).
//
// Hist: 02.01.01 - New for March XDK release
//       06.10.01 - Revised algorithm for better cache performance
//       02.14.01 - Fixed some memory leaks
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdarg.h>
#include <algorithm> // This is the STL header for the sort template
#include <assert.h>

#include "rwcore.h"

#include "xbstrip.h"




// Cache size to optimize for.  The actual cache size is 24, but it is in 
// general not good to use the oldest 6 entries.
#define CACHE_SIZE 18


// Estimated length of the vertex shader to use when comparing the cost of
// different stripifications.
#define SHADER_CYCLES 20




//-----------------------------------------------------------------------------
// structs
//-----------------------------------------------------------------------------
typedef RwUInt16 (*TRIANGLELIST)[3];


struct TRIANGLEINFO
{
    int neighbortri[3];     // Triangle sharing edge (i,i+1), or -1
    int neighboredge[3];    // Edge (j,j+1) of neighboring triangle.
};




//-----------------------------------------------------------------------------
// Name: struct CStrip
// Desc: A single triangle strip. After a mesh is stripified, it typically is 
//       composed of several of these.
//-----------------------------------------------------------------------------
struct CStrip
{
    RwBool  m_bIsStripCW;

    RwUInt32 m_dwNumTriangles;
    int*  m_pTriangles;

    RwUInt32 m_dwNumIndices;
    RwUInt16* m_pIndices;

    RwUInt32 m_dwNumNeighbors;

    CStrip( RwUInt32 num_tris, RwUInt32 num_indices ) 
    {
        m_dwNumTriangles = num_tris;
        m_pTriangles = new int[ num_tris ]; 

        m_dwNumIndices = num_indices;
        m_pIndices = new RwUInt16[ num_indices ];
    }

    ~CStrip()
    {
        delete[] m_pTriangles;
        delete[] m_pIndices;
    }
};




//-----------------------------------------------------------------------------
// Name: struct CSubStrip
// Desc: A structure that specifies part of a strip in a striplist.
//-----------------------------------------------------------------------------
struct CSubStrip
{
    int m_iStrip;   // Index into striplist.
    int m_iStart;   // Starting triangle index
    int m_iEnd;     // Ending triangle index.
};




//-----------------------------------------------------------------------------
// Name: struct CStripList
// Desc: A list of triangle strips.
//-----------------------------------------------------------------------------
struct CStripList
{
    CStrip** m_pStrips;
    RwUInt32    m_dwNumStrips;

    CStrip** begin() { return (m_dwNumStrips) ? &m_pStrips[0] : NULL; }

    void RemoveStripFromList( CStrip** pStrip )
    {
        for( RwUInt32 i=0; i<m_dwNumStrips; i++ )
        {
            if( &m_pStrips[i] == pStrip )
            {
                delete m_pStrips[i];
                m_dwNumStrips--;

                while( i < m_dwNumStrips )
                {
                    m_pStrips[i] = m_pStrips[i+1];
                    i++;
                }
                break;
            }
        }
    }
    
    void RemoveFirst()
    {
        RemoveStripFromList( &m_pStrips[0] );
    }

    void AddStripToList( CStrip* pStrip )
    {
        m_pStrips[m_dwNumStrips++] = pStrip;
    }

    CStripList(RwUInt32 dwMaxSize)
    {
        m_pStrips     = new CStrip*[dwMaxSize];
        m_dwNumStrips = 0L;
    }

    ~CStripList()
    {
        for( RwUInt32 i=0; i<m_dwNumStrips; i++ )
        {
            delete m_pStrips[i];
        }

        delete[] m_pStrips;
    }
};




//-----------------------------------------------------------------------------
// Class used to vertices for locality of access.
//-----------------------------------------------------------------------------
struct SortEntry
{
    int iFirstUsed;
    int iOrigIndex;

    // Define the < operator, which is needed for the STL sort() routine.
    RwBool operator<( const SortEntry& rhs ) { return iFirstUsed < rhs.iFirstUsed; }
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: vertex cache class
//-----------------------------------------------------------------------------
class CVertCache
{
public:
    CVertCache()    { Reset(); }
    ~CVertCache()   {};

    // Reset cache
    void Reset()
    {
        m_iCachePtr = 0;
        m_cachehits = 0;
        memset( m_rgCache, 0xff, sizeof(m_rgCache) );
    }

    // Add vertindex to cache
    int Add(int strip, int vertindex);

    // Check if a vert is in the cache.
    RwBool IsCached(int vertindex)
    {
        for( int iCache = 0; iCache < CACHE_SIZE; iCache++ )
        {
            if( vertindex == m_rgCache[iCache] )
            {
                // Item is in the cache
                return TRUE;
            }
        }

        return FALSE;
    }

    // Check if a vert uses one of the last two cached verts.
    RwBool IsOld(int vertindex)
    {
        if( vertindex == m_rgCache[m_iCachePtr] )
        {
            // Item is in the cache
            return TRUE;
        }

        return FALSE;
    }

    int NumCacheHits() const
    {
        return m_cachehits;
    }

private:
    int  m_cachehits;                // current # of cache hits
    RwUInt16 m_rgCache[CACHE_SIZE];      // vertex cache
    int  m_rgCacheStrip[CACHE_SIZE]; // strip # which added or re-used vert
    int  m_iCachePtr;                // fifo ptr
    bool m_bReUsed[CACHE_SIZE];      // true if vert was re-used.
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: main stripper class
//-----------------------------------------------------------------------------
class CStripper
{
public:
    int           m_dwNumTris;   // # tris
    TRIANGLELIST  m_pTriangles;  // trilist

    TRIANGLEINFO* m_pTriInfo;    // tri edge, neighbor info
    int*          m_pUsed;       // tri used flag

    // ctors/dtors
    CStripper( int dwNumtris, TRIANGLELIST pTriangles );
    ~CStripper();

    // initialize tri info
    void InitTriangleInfo( int tri, int vert );

    // get maximum length strip from tri/vert
    int CreateStrip( int tri, int vert, int maxlen, int *pswaps, 
                     RwBool bLookAhead, RwBool bNonSequential, RwBool bStartCW, 
                     int* pTris );

    // Turn a list of triangles into indices.
    int CreateIndices( RwBool bStartCW, int iNumTriangles, int* pTriangles, 
                       RwUInt16* pStripVerts );

    // stripify entire mesh
    void BuildStrips( CStripList* pStripList, int maxlen, RwBool bLookAhead, 
                      RwBool bNonSequential, RwBool bSwapOrientation );

    // blast strip indices to ppstripindices
    int CreateManyStrips( CStripList *pstriplist, RwUInt16 **ppstripindices );
    int CreateLongStrip( CStripList *pstriplist, RwUInt16 **ppstripindices );
    int CreateCachedStrip( CStripList *pstriplist, RwUInt16 **ppstripindices );

    // find the best cached strip.
    CStrip** FindBestCachedStrip( CStripList* pStripList, const CVertCache &VertexCache );

    int GetNeighborCount( int tri )
    {
        int count = 0;
        for( int vert = 0; vert < 3; vert++ )
        {
            int neighbortri = m_pTriInfo[tri].neighbortri[vert];
            count += (neighbortri != -1) && !m_pUsed[neighbortri];
        }
        return count;
    }
};




//-----------------------------------------------------------------------------
// Name: CreateStrip()
// Desc: Get maximum length of strip starting at tri/vert
//-----------------------------------------------------------------------------
int CStripper::CreateStrip( int tri, int vert, int maxlen, int *pswaps,
                            RwBool bLookAhead, RwBool bNonSequential, 
                            RwBool bStartCW, int* pTris )
{
    *pswaps = 0;

    // this guy has already been used?
    if(m_pUsed[tri])
        return 0;

    // mark tri as used
    m_pUsed[tri] = 1;

    int swaps = 0;

    // add first tri
    int iNumTris = 0;
    pTris[iNumTris++] = tri;

    bStartCW = !bStartCW;

    // get next tri information
    int edge     = (bStartCW ? vert + 2 : vert + 1) % 3;
    int nexttri  = m_pTriInfo[tri].neighbortri[edge];
    int nextvert = m_pTriInfo[tri].neighboredge[edge];

    // start building the strip until we run out of room or indices
    for(int stripcount = 3; stripcount < maxlen; stripcount++)
    {
        // dead end?
        if(nexttri == -1 || m_pUsed[nexttri])
            break;

        // move to next tri
        tri  = nexttri;
        vert = nextvert;

        // toggle orientation
        bStartCW = !bStartCW;

        // find the next natural edge
        int edge = (bStartCW ? vert + 2 : vert + 1) % 3;
        nexttri  = m_pTriInfo[tri].neighbortri[edge];
        nextvert = m_pTriInfo[tri].neighboredge[edge];

        RwBool bSwap = FALSE;
        if(nexttri == -1 || m_pUsed[nexttri])
        {
            // if the next tri is a dead end - try swapping orientation
            bSwap = TRUE;
        }
        else if( bLookAhead )
        {
            // try a swap and see who our new neighbor would be
            int edgeswap     = (bStartCW ? vert + 1 : vert + 2) % 3;
            int nexttriswap  = m_pTriInfo[tri].neighbortri[edgeswap];
            int nextvertswap = m_pTriInfo[tri].neighboredge[edgeswap];

            if( nexttriswap != -1 && !m_pUsed[nexttriswap] )
            {
                // if the swap neighbor has a lower count, change directions
                if( GetNeighborCount(nexttriswap) < GetNeighborCount(nexttri) )
                {
                    bSwap = TRUE;
                }
                else if( GetNeighborCount(nexttriswap) == GetNeighborCount(nexttri) )
                {
                    // if they have the same number of neighbors - check their neighbors
                    edgeswap    = (bStartCW ? nextvertswap + 2 : nextvertswap + 1) % 3;
                    nexttriswap = m_pTriInfo[nexttriswap].neighbortri[edgeswap];

                    int edge1    = (bStartCW ? nextvert + 1 : nextvert + 2) % 3;
                    int nexttri1 = m_pTriInfo[nexttri].neighbortri[edge1];

                    if( nexttri1 == -1 || m_pUsed[nexttri1] )
                    {
                        // natural winding order leads us to a dead end so turn
                        bSwap = TRUE;
                    }
                    else if( nexttriswap != -1 && !m_pUsed[nexttriswap] )
                    {
                        // check neighbor counts on both directions and swap if it's better
                        if( GetNeighborCount(nexttriswap) < GetNeighborCount(nexttri1) )
                            bSwap = TRUE;
                    }
                }
            }
        }

        if( bSwap && bNonSequential )
        {
            // we've been told to change directions so make sure we actually can
            // and then add the swap vertex
            int edgeswap = (bStartCW ? vert + 1 : vert + 2) % 3;
            nexttri      = m_pTriInfo[tri].neighbortri[edgeswap];
            nextvert     = m_pTriInfo[tri].neighboredge[edgeswap];

            if( nexttri != -1 && !m_pUsed[nexttri] )
            {
                stripcount++;
                swaps++;
                bStartCW = !bStartCW;
            }
        }

        pTris[iNumTris++] = tri;

        // mark triangle as used
        m_pUsed[tri] = 1;
    }

    // clear the used flags
    for( int j = 0; j < iNumTris; j++ )
        m_pUsed[pTris[j]] = 0;

    // return swap count and striplen
    *pswaps = swaps;

    return iNumTris;
}




//-----------------------------------------------------------------------------
// Name: CStripper::CreateIndices()
// Desc: Make strip indices from triangle list.
//-----------------------------------------------------------------------------
int CStripper::CreateIndices( RwBool bStartCW, int iNumTriangles, int* pTriangles, RwUInt16* pStripVerts )
{
    int stripcount = 0;
    RwBool bCW = bStartCW;
    int in_edge = -1;

    for( int i = 0; i < iNumTriangles; i++ )
    {
        int out_edge;

        int tri = pTriangles[i];

        // get next tri information
        if( i < iNumTriangles-1 )
        {
            int nexttri = pTriangles[i+1];

            for( out_edge = 0; out_edge < 3; out_edge++ )
                if( m_pTriInfo[tri].neighbortri[out_edge] == nexttri )
                    break;
        }
        else
        {
            out_edge = (bCW ? (in_edge + 1) : (in_edge + 2)) % 3;
        }

        if( i == 0 )
        {
            if( bCW )
            {
                pStripVerts[0] = m_pTriangles[tri][(out_edge + 2) % 3];
                pStripVerts[1] = m_pTriangles[tri][(out_edge + 0) % 3];
                pStripVerts[2] = m_pTriangles[tri][(out_edge + 1) % 3];
            }
            else
            {
                pStripVerts[0] = m_pTriangles[tri][(out_edge + 2) % 3];
                pStripVerts[1] = m_pTriangles[tri][(out_edge + 1) % 3];
                pStripVerts[2] = m_pTriangles[tri][(out_edge + 0) % 3];
            }

            stripcount = 3;
        }
        else
        {
            if( out_edge == (bCW ? (in_edge + 1) : (in_edge + 2)) % 3 )
            {
                // In order.
                pStripVerts[stripcount++] = m_pTriangles[tri][(in_edge + 2) % 3];
            }
            else
            {
                // Swap.
                if( bCW )
                {
                    pStripVerts[stripcount++] = m_pTriangles[tri][(in_edge + 0) % 3];
                    pStripVerts[stripcount++] = m_pTriangles[tri][(in_edge + 2) % 3];
                }
                else
                {
                    pStripVerts[stripcount++] = m_pTriangles[tri][(in_edge + 1) % 3];
                    pStripVerts[stripcount++] = m_pTriangles[tri][(in_edge + 2) % 3];
                }

                bCW = !bCW;
            }
        }

        in_edge = m_pTriInfo[tri].neighboredge[out_edge];
        bCW = !bCW;
    }

    return stripcount;
}




//-----------------------------------------------------------------------------
// Name: FindBestStrip()
// Desc: Given a striplist and current cache state, pick the best next strip
//-----------------------------------------------------------------------------
CStrip** FindBestStrip( CStripList* pStripList,
                              const CVertCache &VertexCache )
{
    if( 0 == pStripList->m_dwNumStrips ) 
        return NULL;

    CStrip** ppBestStrip       = pStripList->begin();
    RwUInt32    dwBestStripLen    = (*ppBestStrip)->m_dwNumIndices;
    RwBool     bStartCW          = (*ppBestStrip)->m_bIsStripCW;
    RwBool     bBestStripFlipped = FALSE;
    int      MaxCacheHits      = -1;

    // Go through all the other strips looking for the best caching
    for( RwUInt32 i = 0; i < pStripList->m_dwNumStrips; i++ )
    {
        CStrip* pStrip        = pStripList->m_pStrips[i];
        RwUInt32   dwStripLen    = pStrip->m_dwNumIndices;
        RwBool    bStripFlipped = FALSE;

        // Check cache if this strip is the same type as us (ie: cw/odd)
        if( ( pStrip->m_bIsStripCW == bStartCW) && ( (dwBestStripLen & 0x1) == (dwStripLen & 0x1) ) )
        {
            // Copy current state of cache
            CVertCache NewVertexCache = VertexCache;

            // Figure out what this guy would do to our cache
            for( RwUInt32 ivert = 0; ivert < dwStripLen; ivert++ )
                NewVertexCache.Add( 2, pStrip->m_pIndices[ivert] );

            // For even length strips - see if we can get better cache hits
            // if we reverse the vertex cache contents
            if( !(dwStripLen & 0x1) )
            {
                // Create a copy of the vertex cache, with all vertices flipped
                CVertCache FlippedVertexCache = VertexCache;
                for( int ivert = pStrip->m_dwNumIndices-1; ivert >= 0; ivert-- )
                    FlippedVertexCache.Add( 2, pStrip->m_pIndices[ivert] );

                // Accept the flipped cache if it gives us more cahce hits
                if( FlippedVertexCache.NumCacheHits() > NewVertexCache.NumCacheHits() )
                {
                    NewVertexCache = FlippedVertexCache;
                    bStripFlipped  = TRUE;
                }
            }

            // Record the best number of cache hits to date
            int NumCacheHits = NewVertexCache.NumCacheHits() - VertexCache.NumCacheHits();
            
            if( NumCacheHits > MaxCacheHits )
            {
                MaxCacheHits      = NumCacheHits;
                ppBestStrip       = &pStripList->m_pStrips[i];
                dwBestStripLen    = dwStripLen;//? added by mikey
                bBestStripFlipped = bStripFlipped;
            }
        }
    }

    if( bBestStripFlipped )
    {
        CStrip* pStrip = *ppBestStrip;
        int first = 0;
        int last  = pStrip->m_dwNumIndices - 1;

        while( first < last )
        {
            // Swap vertex indices
            RwUInt16 temp                 = pStrip->m_pIndices[first];
            pStrip->m_pIndices[first] = pStrip->m_pIndices[last];
            pStrip->m_pIndices[last]  = temp;
            first++;
            last--;
        }
    }

    // Make sure we keep the list in order and always pull off
    // the first dude.
    if( ppBestStrip != pStripList->begin() )
    {
        // Swap strips
        CStrip* temp           = (*ppBestStrip);
        (*ppBestStrip)         = (*pStripList->begin());
        (*pStripList->begin()) = temp;
    }

    return pStripList->begin();
}




//-----------------------------------------------------------------------------
// Name: CStripper::FindBestCachedStrip()
// Desc: Given a striplist and current cache state, pick the best next strip
//-----------------------------------------------------------------------------
CStrip** CStripper::FindBestCachedStrip( CStripList* pStripList,
                                         const CVertCache &VertexCache )
{
    if( 0 == pStripList->m_dwNumStrips ) 
        return NULL;

    CStrip** ppBestStrip       = pStripList->begin();

    RwBool     bBestStripFlipped = FALSE;

    int      MaxCacheHits      = -1;

    RwUInt32    dwBestNeighborCount = (*ppBestStrip)->m_dwNumNeighbors;

    // Go through all the other strips looking for the best caching
    for( RwUInt32 i = 0; i < pStripList->m_dwNumStrips; i++ )
    {
        CStrip* pStrip        = pStripList->m_pStrips[i];
        RwUInt32   dwStripLen    = pStrip->m_dwNumIndices;
        RwBool    bStripFlipped = FALSE;

        // Copy current state of cache
        CVertCache NewVertexCache = VertexCache;

        // Figure out what this guy would do to our cache
        for( RwUInt32 ivert = 0; ivert < dwStripLen; ivert++ )
            NewVertexCache.Add( 2, pStrip->m_pIndices[ivert] );

        // See if we can get better cache hits if we reverse the order we draw
        // the strip in.
        {
            // Create a copy of the vertex cache, with all vertices flipped
            CVertCache FlippedVertexCache = VertexCache;
            for( int ivert = pStrip->m_dwNumIndices-1; ivert >= 0; ivert-- )
                FlippedVertexCache.Add( 2, pStrip->m_pIndices[ivert] );

            // Accept the flipped cache if it gives us more cahce hits
            if( FlippedVertexCache.NumCacheHits() > NewVertexCache.NumCacheHits() )
            {
                NewVertexCache = FlippedVertexCache;
                bStripFlipped  = TRUE;
            }
        }

        // Record the best number of cache hits to date
        int NumCacheHits = NewVertexCache.NumCacheHits() - VertexCache.NumCacheHits();
        
        if( NumCacheHits > MaxCacheHits )
        {
            MaxCacheHits        = NumCacheHits;
            ppBestStrip         = &pStripList->m_pStrips[i];
            bBestStripFlipped   = bStripFlipped;
            dwBestNeighborCount = pStripList->m_pStrips[i]->m_dwNumNeighbors;
        }
        else if ( NumCacheHits == MaxCacheHits && 
                  pStripList->m_pStrips[i]->m_dwNumNeighbors < dwBestNeighborCount )
        {
            ppBestStrip         = &pStripList->m_pStrips[i];
            bBestStripFlipped   = bStripFlipped;
            dwBestNeighborCount = pStripList->m_pStrips[i]->m_dwNumNeighbors;
        }
    }

    if( bBestStripFlipped )
    {
        CStrip* pStrip = *ppBestStrip;
        int first = 0;
        int last  = pStrip->m_dwNumIndices - 1;

        while( first < last )
        {
            // Swap vertex indices
            RwUInt16 temp                 = pStrip->m_pIndices[first];
            pStrip->m_pIndices[first] = pStrip->m_pIndices[last];
            pStrip->m_pIndices[last]  = temp;
            first++;
            last--;
        }

        // We must also reverse the starting winding for odd length strips.
        if( (pStrip->m_dwNumIndices & 0x1) )
        {
            pStrip->m_bIsStripCW = !pStrip->m_bIsStripCW;
        }
    }

    // Make sure we keep the list in order and always pull off
    // the first dude.
    if( ppBestStrip != pStripList->begin() )
    {
        // Swap strips
        CStrip* temp           = (*ppBestStrip);
        (*ppBestStrip)         = (*pStripList->begin());
        (*pStripList->begin()) = temp;
    }

    return pStripList->begin();
}




//-----------------------------------------------------------------------------
// Name: CStripper::CreateManyStrips()
// Desc: Don't merge the strips - just blast em into the stripbuffer one by one
//       (useful for debugging)
//-----------------------------------------------------------------------------
int CStripper::CreateManyStrips( CStripList* pStripList, RwUInt16** ppStripIndices )
{
    // Count the number of indices. Allow room for each of the strips size
    // plus the final 0
    RwUInt32 dwIndexCount = pStripList->m_dwNumStrips + 1;

    // We're storing the strips in [size1 i1 i2 i3][size2 i4 i5 i6][0] format
    for( RwUInt32 i = 0; i < pStripList->m_dwNumStrips; i++ )
    {
        // Add striplength plus potential degenerate to swap ccw --> cw
        dwIndexCount += pStripList->m_pStrips[i]->m_dwNumIndices + 1;
    }

    // Alloc the space for all this stuff
    RwUInt16*      pStripIndices     = new RwUInt16[dwIndexCount];
    RwUInt32      dwNumStripIndices = 0;
    CVertCache VertexCache;

    // Loop through all strips
    CStrip** ppStrip = pStripList->begin();

    while( pStripList->m_dwNumStrips > 0 )
    {
        CStrip* pStrip = *ppStrip;

        if( !pStrip->m_bIsStripCW )
        {
            // add an extra index if it's ccw
            pStripIndices[dwNumStripIndices++] = (RwUInt16)pStrip->m_dwNumIndices + 1;
            pStripIndices[dwNumStripIndices++] = pStrip->m_pIndices[0];
        }
        else
        {
            // add the strip length
            pStripIndices[dwNumStripIndices++] = (RwUInt16)pStrip->m_dwNumIndices;
        }

        // add all the strip indices
        for( RwUInt32 i = 0; i < pStrip->m_dwNumIndices; i++)
        {
            pStripIndices[dwNumStripIndices++] = pStrip->m_pIndices[i];
            VertexCache.Add( 1, pStrip->m_pIndices[i] );
        }

        // free this guy and pop him off the list
        pStripList->RemoveFirst();

        // Get the next best strip
        ppStrip = FindBestStrip( pStripList, VertexCache );
    }

    // add terminating zero
    pStripIndices[dwNumStripIndices++] = 0;
    (*ppStripIndices) = pStripIndices;

    return dwNumStripIndices;
}




//-----------------------------------------------------------------------------
// Name: CStripper::CreateLongStrip()
// Desc: Merge striplist into one big uberlist with (hopefully) optimal caching
//-----------------------------------------------------------------------------
int CStripper::CreateLongStrip( CStripList* pStripList, RwUInt16** ppwStripIndices )
{
    // Allow room for one strip length plus a possible 3 extra indices per
    // concatenated strip list plus the final 0
    int dwIndexCount = (pStripList->m_dwNumStrips * 3) + 2;

    // We're storing the strips in [size1 i1 i2 i3][size2 i4 i5 i6][0] format
    for( RwUInt32 i=0; i < pStripList->m_dwNumStrips; i ++ )
    {
        dwIndexCount += pStripList->m_pStrips[i]->m_dwNumIndices;
    }

    // Alloc the space for all this stuff
    RwUInt16*      pStripIndices     = new RwUInt16[dwIndexCount];
    int        dwNumStripIndices = 0;
    CVertCache VertexCache;

    // Add first strip
    CStrip** ppStrip = pStripList->begin();
    CStrip*  pStrip = *ppStrip;

    // Note: first strip should be cw

    for( RwUInt32 ivert = 0; ivert < pStrip->m_dwNumIndices; ivert++ )
    {
        pStripIndices[dwNumStripIndices++] = pStrip->m_pIndices[ivert];
        VertexCache.Add( 1, pStrip->m_pIndices[ivert] );
    }

    // Kill first dude
    pStripList->RemoveStripFromList( ppStrip );

    // Add all the others
    while( pStripList->m_dwNumStrips )
    {
        ppStrip = FindBestStrip( pStripList, VertexCache );
        CStrip* pStrip = *ppStrip;
        RwUInt16 wLastVertex  = pStripIndices[dwNumStripIndices - 1];
        RwUInt16 wFirstVertex = pStrip->m_pIndices[0];

        if( wFirstVertex != wLastVertex )
        {
            // Add degenerate from last strip
            pStripIndices[dwNumStripIndices++] = wLastVertex;

            // Add degenerate from our strip
            pStripIndices[dwNumStripIndices++] = wFirstVertex;
        }

        // If we're not orientated correctly, we need to add a degenerate
        if( pStrip->m_bIsStripCW != !(dwNumStripIndices & 0x1) )
        {
            // This shouldn't happen - we're currently trying very hard
            // to keep everything oriented correctly.
            pStripIndices[dwNumStripIndices++] = wFirstVertex;
        }

        // Add these verts
        for( RwUInt32 ivert = 0; ivert < pStrip->m_dwNumIndices; ivert++ )
        {
            pStripIndices[dwNumStripIndices++] = pStrip->m_pIndices[ivert];
            VertexCache.Add( 1, pStrip->m_pIndices[ivert] );
        }

        // Free these guys
        pStripList->RemoveStripFromList( ppStrip );
    }

    (*ppwStripIndices) = pStripIndices;
    return dwNumStripIndices;
}




//-----------------------------------------------------------------------------
// Name: CStripper::CreateCachedStrip()
// Desc: Merge striplist into one big uberlist with (hopefully) optimal caching
//-----------------------------------------------------------------------------
int CStripper::CreateCachedStrip( CStripList* pStripList, RwUInt16** ppwStripIndices )
{
    RwUInt32 i;
    RwUInt16 pTempVerts[CACHE_SIZE*4];

    // Split up the strips into cache friendly pieces.
    CStripList* pNewList = new CStripList(m_dwNumTris);

    while( pStripList->m_dwNumStrips )
    {
        CStrip** ppStrip = pStripList->begin();
        CStrip* pStrip = *ppStrip;

        int start = 0;
        int ssize = pStrip->m_dwNumTriangles;

        do
        {
            int dsize = ssize;

            if (dsize > CACHE_SIZE)
                dsize = CACHE_SIZE;

            int j = pNewList->m_dwNumStrips++;

            // Create temp triaingle list/index list.
            int num_indices = CreateIndices( pStrip->m_bIsStripCW, dsize, 
                                             pStrip->m_pTriangles + start, 
                                             pTempVerts );
    
            // Make new strip.
            pNewList->m_pStrips[j] = new CStrip( dsize, num_indices );

            pNewList->m_pStrips[j]->m_bIsStripCW = pStrip->m_bIsStripCW;

            // Copy triangles.
            memcpy( pNewList->m_pStrips[j]->m_pTriangles, 
                    pStrip->m_pTriangles + start, dsize * sizeof(int) );

            // Copy indices.
            memcpy( pNewList->m_pStrips[j]->m_pIndices, 
                    pTempVerts, num_indices * sizeof(RwUInt16) );

            start += dsize;
            ssize -= dsize;
        }
        while (ssize > 0);

        pStripList->RemoveStripFromList( ppStrip );
    }

    // Count the number of adjacent triangles to each strip.
    // an edge of the mesh.
    for( i = 0; i < pNewList->m_dwNumStrips; i++ )
    {
        CStrip* pStrip = pNewList->m_pStrips[i];

        RwUInt32 count = 0;

        for( RwUInt32 j = 0; j < pStrip->m_dwNumTriangles; j++ )
        {
            // Count the number of neighbors.
            for( int vert = 0; vert < 3; vert++ )
            {
                if (m_pTriInfo[pStrip->m_pTriangles[j]].neighbortri[vert] != -1)
                    count++;
            }
        }

        pStrip->m_dwNumNeighbors = count;
    }

    // Should we remove/ignore very small strips?

    // Allow room for one strip length plus a possible 3 extra indices per
    // concatenated strip list plus the final 0
    int dwIndexCount = (pNewList->m_dwNumStrips * 3) + 2;

    // We're storing the strips in [size1 i1 i2 i3][size2 i4 i5 i6][0] format
    for( i = 0; i < pNewList->m_dwNumStrips; i++ )
    {
        dwIndexCount += pNewList->m_pStrips[i]->m_dwNumIndices;
    }

    // Alloc the space for all this stuff
    RwUInt16*      pStripIndices     = new RwUInt16[dwIndexCount];
    RwUInt32      dwNumStripIndices = 0;
    CVertCache VertexCache;

    // Add the strips.
    while( pNewList->m_dwNumStrips )
    {
        CStrip** ppStrip = FindBestCachedStrip( pNewList, VertexCache );
        CStrip* pStrip = *ppStrip;

        RwUInt16 wFirstVertex = pStrip->m_pIndices[0];

        RwUInt32 ivert = 0;

        if (dwNumStripIndices > 0)
        {
            RwUInt16 wLastVertex = pStripIndices[dwNumStripIndices - 1];
        
            assert( dwNumStripIndices > 2 );

            if( wLastVertex == pStrip->m_pIndices[1] &&
                pStripIndices[dwNumStripIndices - 2] == wFirstVertex &&
                pStrip->m_bIsStripCW == !(dwNumStripIndices & 0x1) )
            {
                // We are re-stitching strips together, so skip the first two
                // verts of this strip.
                ivert = 2;
            }
            else if( wFirstVertex != wLastVertex )
            {
                // Add degenerate from last strip
                pStripIndices[dwNumStripIndices++] = wLastVertex;

                // Add degenerate from our strip
                pStripIndices[dwNumStripIndices++] = wFirstVertex;
            }
        }

        // If we're not orientated correctly, we need to add a degenerate
        if( pStrip->m_bIsStripCW != !(dwNumStripIndices & 0x1) )
        {
            pStripIndices[dwNumStripIndices++] = wFirstVertex;
        }

        // Add these verts and update cache.
        while( ivert < pStrip->m_dwNumIndices )
        {
            pStripIndices[dwNumStripIndices] = pStrip->m_pIndices[ivert];
            VertexCache.Add( 1, pStrip->m_pIndices[ivert] );
            dwNumStripIndices++;
            ivert++;
        }

        // Free these guys
        pNewList->RemoveStripFromList( ppStrip );
    }

    delete pNewList;

    (*ppwStripIndices) = pStripIndices;
    return dwNumStripIndices;
}




//-----------------------------------------------------------------------------
// Name: CStripper::BuildStrips()
// Desc: Build a (hopefully) optimal set of strips from a trilist
//-----------------------------------------------------------------------------
void CStripper::BuildStrips( CStripList* pStripList, int maxlen, RwBool bLookAhead, 
                             RwBool bNonSequential, RwBool bSwapOrientation )
{
    // temp indices storage
    const int cNumTmpVerts = 1024;
    RwUInt16 pStripVerts[cNumTmpVerts + 1];
    int pStripTris[cNumTmpVerts + 1];

    // clear all the used flags for the tris
    memset( m_pUsed, 0, m_dwNumTris * sizeof(m_pUsed[0]) );

    RwBool bStartCW = TRUE;

    while( TRUE )
    {
        int   besttri;
        int   bestvert;
        float bestratio = 2.0f;
        int   bestneighborcount = RwInt32MAXVAL;
	int   tri;	//. vs2005 error fix. by nonstopdj.
	
        for( tri = 0; tri < m_dwNumTris; tri++)
        {
            // if used the continue
            if(m_pUsed[tri])
                continue;

            // get the neighbor count
            int curneighborcount = GetNeighborCount(tri);

            // push all the singletons to the very end
            if( !curneighborcount )
                curneighborcount = 4;

            // if this guy has more neighbors than the current best - bail
            if( curneighborcount > bestneighborcount )
                continue;

            // try starting the strip with each of this tris verts
            for( int vert = 0; vert < 3; vert++ )
            {
                int swaps;
                int num_tris = CreateStrip( tri, vert, maxlen, &swaps, bLookAhead, 
                                            bNonSequential, bStartCW, pStripTris );

                int len = 2 + num_tris + swaps;
                float ratio = (len == 3) ? 1.0f : (float)swaps / len;

                // check if this ratio is better than what we've already got for
                // this neighborcount
                if( (curneighborcount < bestneighborcount) ||
                    (curneighborcount == bestneighborcount && ratio < bestratio) )
                {
                    bestneighborcount = curneighborcount;
                    besttri = tri;
                    bestvert = vert;
                    bestratio = ratio;
                }

            }
        }

        // no strips found?
        if( bestneighborcount == RwInt32MAXVAL )
            break;

        // recreate this strip
        int swaps;
        int num_tris = CreateStrip( besttri, bestvert, maxlen, &swaps, bLookAhead, 
                                    bNonSequential, bStartCW, pStripTris );

        // Mark the tris on the best strip as used
        for( tri = 0; tri < num_tris; tri++ )
            m_pUsed[pStripTris[tri]] = 1;

        // Make the indices from the triangle verts.
        int num_indices = CreateIndices( bStartCW, num_tris, pStripTris, pStripVerts );

        // Create a new CStrip and stuff in the list.
        CStrip* pStrip = new CStrip( num_tris, num_indices );

        pStrip->m_bIsStripCW = bStartCW;

        for( int j = 0; j < num_tris; j++ )
            pStrip->m_pTriangles[j] = pStripTris[j];

        for( int k = 0; k < num_indices; k++ )
            pStrip->m_pIndices[k] = pStripVerts[k];

        // Store the CStrip
        pStripList->AddStripToList( pStrip );

        if( bSwapOrientation )
        {
            // if strip was odd - swap orientation
            if( (num_indices & 0x1) )
              bStartCW = !bStartCW;
        }
    }
}




//-----------------------------------------------------------------------------
// Name: CStripper::InitTriangleInfo()
// Desc: Initialize triangle information (edges, #neighbors, etc.)
//-----------------------------------------------------------------------------
void CStripper::InitTriangleInfo(int tri, int vert)
{
    RwUInt16* ptriverts = &m_pTriangles[tri + 1][0];
    int   vert1     =  m_pTriangles[tri][(vert + 1) % 3];
    int   vert2     =  m_pTriangles[tri][vert];

    for( int itri = tri + 1; itri < m_dwNumTris; itri++, ptriverts += 3 )
    {
        if( m_pUsed[itri] != 0x7 )
        {
            for( int ivert = 0; ivert < 3; ivert++ )
            {
                if( ( ptriverts[ivert] == vert1) &&
                    ( ptriverts[(ivert + 1) % 3] == vert2 ) )
                {
                    // add the triangle info
                    m_pTriInfo[tri].neighbortri[vert]  = itri;
                    m_pTriInfo[tri].neighboredge[vert] = ivert;
                    m_pUsed[tri] |= (1 << vert);

                    m_pTriInfo[itri].neighbortri[ivert]  = tri;
                    m_pTriInfo[itri].neighboredge[ivert] = vert;
                    m_pUsed[itri] |= (1 << ivert);
                    return;
                }
            }
        }
    }
}




//-----------------------------------------------------------------------------
// Name: CStripper()
// Desc: CStripper ctor
//-----------------------------------------------------------------------------
CStripper::CStripper( int dwNumTris, TRIANGLELIST pTriangles )
{
    // store trilist info
    m_dwNumTris  = dwNumTris;
    m_pTriangles = pTriangles;
    m_pUsed      = new int[dwNumTris];
    m_pTriInfo   = new TRIANGLEINFO[dwNumTris];
    int itri; //. vs2005 error fix by nonstopdj.

    // init triinfo
    for( itri = 0; itri < dwNumTris; itri++ )
    {
        m_pTriInfo[itri].neighbortri[0] = -1;
        m_pTriInfo[itri].neighbortri[1] = -1;
        m_pTriInfo[itri].neighbortri[2] = -1;
    }

    // Clear the used flag
    memset( m_pUsed, 0, m_dwNumTris * sizeof(m_pUsed[0]) );

    // Go through all the triangles and find edges, neighbor counts
    for( itri = 0; itri < dwNumTris; itri++ )
    {
        for( int ivert = 0; ivert < 3; ivert++ )
        {
            if( !(m_pUsed[itri] & (1 << ivert)) )
                InitTriangleInfo( itri, ivert );
        }
    }

    // Clear the used flags from InitTriangleInfo
    memset( m_pUsed, 0, m_dwNumTris * sizeof(m_pUsed[0]) );
}




//-----------------------------------------------------------------------------
// Name: ~CStripper
// Desc: CStripper dtor
//-----------------------------------------------------------------------------
CStripper::~CStripper()
{
    // free stuff
    delete[] m_pUsed;
    m_pUsed = NULL;

    delete[] m_pTriInfo;
    m_pTriInfo = NULL;
}




//-----------------------------------------------------------------------------
// Name: CVertCache::Add()
// Desc: Add an index to the cache - returns true if it was added, false otherwise
//-----------------------------------------------------------------------------
int CVertCache::Add( int strip, int vertindex )
{
    // Find index in cache
    for( int iCache = 0; iCache < CACHE_SIZE; iCache++ )
    {
        if( vertindex == m_rgCache[iCache] )
        {
            // If it's in the cache and it's from a different strip
            // change the strip to the new one and count the cache hit
            if( strip != m_rgCacheStrip[iCache] )
            {
                m_cachehits++;
                m_rgCacheStrip[iCache] = strip;
                m_bReUsed[iCache] = true;
            }

            // Item is already in the cache, so no need to add it
            return 0;
        }
    }

    int retval = 1;

    // If we are push one of the verts add by our strip out of the cache, return two.
    if ( m_rgCache[m_iCachePtr] != -1 && m_rgCacheStrip[m_iCachePtr] == strip && 
         !m_bReUsed[m_iCachePtr] )
        retval = 2;

    // Not in cache, add vert and strip
    m_rgCache[m_iCachePtr]      = (RwUInt16)vertindex;
    m_rgCacheStrip[m_iCachePtr] = strip;
    m_bReUsed[m_iCachePtr]      = false;
    m_iCachePtr                 = (m_iCachePtr + 1) % CACHE_SIZE;
    
    return retval;
}




//-----------------------------------------------------------------------------
// Name: CountCacheMisses()
// Desc: Count the number of cache misses for a given strip.
//-----------------------------------------------------------------------------
RwUInt32 CountCacheMisses( RwUInt32 dwIndexCount, RwUInt16 *pStripIndices )
{
    CVertCache VertexCache;

    RwUInt32 dwMisses = 0;

    for ( RwUInt32 i = 0; i < dwIndexCount; i++ )
        dwMisses += (VertexCache.Add( 1, pStripIndices[i] ) != 0);

    return dwMisses;
}




//-----------------------------------------------------------------------------
// Name: TriStripToTriList()
// Desc: Convert a tri-strip to a tri-list.
//-----------------------------------------------------------------------------
RwUInt32 TriStripToTriList( RwUInt32 dwNumStripIndices, const RwUInt16 *pStripIndices, 
                         RwUInt16 *pTriangleIndices )
{
    RwUInt32 dwNumTriangleIndices = 0;
        
    // Unstrip the indices.
    RwUInt16 ind0 = 0;
    RwUInt16 ind1 = pStripIndices[0];
    RwUInt16 ind2 = pStripIndices[1];

    for( RwUInt32 src = 2; src < dwNumStripIndices; src++ )
    {
        ind0 = ind1;
        ind1 = ind2;
        ind2 = pStripIndices[src];
    
        // Check for null-triangles.    
        if( ind0 != ind1 && ind1 != ind2 && ind2 != ind0 )
        {
            if( src & 1 )
            {
                pTriangleIndices[dwNumTriangleIndices] = ind1;
                dwNumTriangleIndices++;

                pTriangleIndices[dwNumTriangleIndices] = ind0;
                dwNumTriangleIndices++;

                // always put the new index last
                pTriangleIndices[dwNumTriangleIndices] = ind2;
                dwNumTriangleIndices++;
            }
            else
            {
                pTriangleIndices[dwNumTriangleIndices] = ind0;
                dwNumTriangleIndices++;

                pTriangleIndices[dwNumTriangleIndices] = ind1;
                dwNumTriangleIndices++;

                // always put the new index last
                pTriangleIndices[dwNumTriangleIndices] = ind2;
                dwNumTriangleIndices++;
            }
        }
    }

    return dwNumTriangleIndices;
}




//-----------------------------------------------------------------------------
// Name: Stripify()
// Desc: Stripify routine
//-----------------------------------------------------------------------------
RwUInt32 Stripify( RwUInt32 dwNumTriangles, RwUInt16* pTriangles, 
                RwUInt32* pdwNumIndices, RwUInt16** ppStripIndices, RwUInt32 dwFlags )
{
    if( 0 == dwNumTriangles || NULL == pTriangles )
        return 0;

    *ppStripIndices = 0;

    // The stipper, and storage for it's best results
    CStripper   stripper( dwNumTriangles, (TRIANGLELIST)pTriangles );

    RwUInt32       dwBestCost = 0xffffffff;
    RwUInt32       dwBestIndexCount;
    RwUInt16*       pTempStripIndices;

    // Map of various args to try stripifying mesh with
    struct ARGMAP
    {
        RwUInt32   dwMaxLength;    // Maximum length of strips
        RwBool    bLookAhead;     // Whether to use sgi greedy lookahead
        RwBool    bNonSequential; // Take non-sequential exits to lengthen strips.
    };
    
    ARGMAP argmap[] =
    {
        { 1024,  TRUE, TRUE  },
        { 1024,  FALSE, TRUE },
        { 1024,  FALSE, FALSE },
    };
    const int dwNumArgMaps = sizeof(argmap)/sizeof(ARGMAP);

    // Build strips with the various maxlength and lookahead arguments, and
    // pick the one with the least result index count.
    for( int map = 0; map < dwNumArgMaps; map++ )
    {
        // Build the strip with the various maxlength and lookahead arguments
        CStripList* pStripList = new CStripList(dwNumTriangles);

        stripper.BuildStrips( pStripList, argmap[map].dwMaxLength, 
                              argmap[map].bLookAhead, argmap[map].bNonSequential,
                              (dwFlags & OPTIMIZE_FOR_INDICES) != 0 );

        RwUInt32 dwIndexCount;
        RwUInt32 dwCost;

        // Build strip (freeing the strip list).
        if( dwFlags & OPTIMIZE_FOR_INDICES )
        {
            dwIndexCount = stripper.CreateLongStrip( pStripList, &pTempStripIndices );

            // Cost is just the number of indices.
            dwCost = dwIndexCount;
        }
        else
        {
            dwIndexCount = stripper.CreateCachedStrip( pStripList, &pTempStripIndices );

            // Count number of cache misses.
            RwUInt32 dwMisses = CountCacheMisses( dwIndexCount, pTempStripIndices );

            if( dwFlags & OUTPUT_TRILIST )
            {
                // Nulls don't matter for tri-lists.
                dwCost = dwMisses;
            }
            else
            {
                // Cost is the (shader length) / 2 + (# null tris) * 2
                dwCost = dwMisses * (SHADER_CYCLES/2) + 
                         (dwIndexCount - (dwNumTriangles + 2)) * 2;
            }
        }

        if ( dwCost < dwBestCost )
        {
            // Free the old best list
            if( *ppStripIndices )
                delete[] *ppStripIndices;

            // store the new best list
            *ppStripIndices  = pTempStripIndices;
            dwBestCost       = dwCost;
            dwBestIndexCount = dwIndexCount;
        }
        else
        {
            delete[] pTempStripIndices;
        }

        delete pStripList;
    }

    if( dwFlags & OUTPUT_TRILIST )
    {
        // Convert to triangle list.
        RwUInt16* pTempIndices = new RwUInt16[dwNumTriangles*3];

        dwBestIndexCount = TriStripToTriList( dwBestIndexCount, *ppStripIndices, 
                                              pTempIndices );

        assert( dwBestIndexCount <= dwNumTriangles*3 );

        delete[] *ppStripIndices;
        *ppStripIndices = pTempIndices;
    }

    if( pdwNumIndices )
        (*pdwNumIndices) = dwBestIndexCount;
    
    return dwBestIndexCount;
}




//-----------------------------------------------------------------------------
// Name: ComputeVertexPermutation()
// Desc: Reorder the vertices
//-----------------------------------------------------------------------------
void ComputeVertexPermutation( RwUInt32 dwNumStripIndices, RwUInt16* pStripIndices,
                               RwUInt32 dwNumVertices, RwUInt16** ppVertexPermutation )
{
    // Sort verts to maximize locality.
    SortEntry* pSortTable = new SortEntry[dwNumVertices];
    
    RwUInt32 i;

    // Fill in original index.
    for( i = 0; i < dwNumVertices; i++ )
    {
        pSortTable[i].iOrigIndex = i;
        pSortTable[i].iFirstUsed = -1;
    }

    // Fill in first used flag.
    for( i = 0; i < dwNumStripIndices; i++ )
    {
        int index = pStripIndices[i];

        if( pSortTable[index].iFirstUsed == -1 )
            pSortTable[index].iFirstUsed = i;
    }

    // Sort the table, using the STL sort() routine.
    std::sort( pSortTable, pSortTable + dwNumVertices );

    // Copy re-mapped to orignal vertex permutaion into output array.
    (*ppVertexPermutation) = new RwUInt16[dwNumVertices];

    for( i = 0; i < dwNumVertices; i++ )
    {
        (*ppVertexPermutation)[i] = (RwUInt16)(pSortTable[i].iOrigIndex);
    }

    // Build original to re-mapped permutation.
    RwUInt16* pInversePermutation = new RwUInt16[dwNumVertices];

    for( i = 0; i < dwNumVertices; i++ )
    {
        pInversePermutation[pSortTable[i].iOrigIndex] = (RwUInt16)i;
    }

    // We need to remap indices as well.
    for( i = 0; i < dwNumStripIndices; i++ )
    {
        pStripIndices[i] = pInversePermutation[ pStripIndices[i] ];
    }

    delete[] pSortTable;
    delete[] pInversePermutation;
}
