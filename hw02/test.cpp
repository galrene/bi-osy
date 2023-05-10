#ifndef __PROGTEST__
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <cmath>
using namespace std;
#endif /* __PROGTEST__ */

#define ALLOC_MEMORY_RANGE 32

/**
 * Bidirectional LL for memory blocks.
 */
class CBiLL {
private:
    uintptr_t * m_Front;
public:
    bool empty () { return m_Front == nullptr; }
    CBiLL ()
    : m_Front ( nullptr ) {}

    void pushFront ( uintptr_t * item ) {
        if ( ! m_Front ) {
            m_Front = item;
            return;
        }
        uintptr_t * prevFront = m_Front;
        prevFront[1] = (uintptr_t) item;
        m_Front = item;
        m_Front[2] = (uintptr_t) prevFront;
    }

    uintptr_t * front () { return m_Front; }

    uintptr_t * popFront () { auto front = m_Front; pop (m_Front); return front; }

    void pop ( uintptr_t * item ) {
        auto prev = (uintptr_t *) item[1];
        auto next = (uintptr_t *) item[2];
        // the case of a single item in the LL - set front to nullptr
        if ( item == m_Front )
            m_Front = next;
        if ( prev )
            prev[2] = (uintptr_t) next;
        if ( next )
            next[1] = (uintptr_t) prev;
    }
};
/**
 * Free block in memory consists of:
 * [BLOCK_SIZE][PREVIOUS_BLOCK_PTR][NEXT_BLOCK_PTR][BLOCK_SIZE]
 * where last bit of size is allocated flag
 */
class CHeap {
private:
    /* Linked lists of sizes 2^i */
    CBiLL m_MemBlocks[ALLOC_MEMORY_RANGE] = {};
    uintptr_t * m_Begin;
    int m_Size;
    int m_AllocatedCnt; // number of allocated blocks
    /**
     * Split a given memory block into blocks of sizes in powers of 2
     * @param size size of memory block to split
     */
    void splitMemSpace ( int size ) {
        uintptr_t * currPos = m_Begin;
        for ( size_t i = 0; size > 0; i++ ) {
            if ( size & 1 ) {
                createBlock (currPos, i );
                int blockSize =  1 << (i-3); // in uintptr_ts, not bytes (therefore divided by 8)
                currPos += blockSize;
            }
            size >>= 1;
        }
    }
    /**
     * Create memory block at address of 2^i size and push it into a corresponding linked list.
     * @param address where to create the block
     * @param i 2's power in the size of the block to create
     */
    void createBlock ( uintptr_t * address, size_t i ) {
        size_t size = 1 << i;
        address[0] = size;
        address[1] = 0;
        address[2] = 0;
        address[ size / sizeof(uintptr_t)  - 1] = size; // uintptr_t * address = 8B, size is in bytes
        m_MemBlocks[i].pushFront ( address );
    }

public:
    CHeap () = default;
    CHeap ( uintptr_t * begin, int size )
    : m_Begin ( begin ), m_Size ( size ), m_AllocatedCnt ( 0 ) {}

    void init () {
        splitMemSpace (m_Size );
    }
    int done () { return m_AllocatedCnt; }

    /**
     * Splits block at given index until one with required size is created.
     * Returns pointer to the resulting block of required size.
     * @param requiredSizePow 2's power (index) of the size of the required block size
     * @param blockToSplitPow 2's power (index) of the size of the given block
     */
    uintptr_t * splitBlock ( size_t blockToSplitPow, size_t requiredSizePow ) {
        uintptr_t * blockToSplit = nullptr;
        /*      currBlockSize   != requiredBlockSize */
        while ( blockToSplitPow != requiredSizePow ) {
            blockToSplit = m_MemBlocks[blockToSplitPow].popFront();

            size_t newBlockIndex = blockToSplitPow - 1;
            size_t newBlockSize = 1 << newBlockIndex;

            createBlock ( blockToSplit, newBlockIndex );
            createBlock ( blockToSplit + (newBlockSize / sizeof (uintptr_t)), newBlockIndex );
            blockToSplitPow--;
        }
        return blockToSplit;
    }
    /**
     * Sets block at index as allocated in memory.
     * @param block where to allocate block
     * @param blockIndex 2's power of the block size
     * @return address to block of allocated memory
     */
    uintptr_t * allocBlock ( uintptr_t * block, size_t blockIndex ) {
        size_t blockSize = 1 << blockIndex;
        block[0] |= 1; // mark block as allocated
        block[ blockSize / sizeof(uintptr_t) - 1] |= 1;
        m_AllocatedCnt++;
        return block + 1;
    }

    uintptr_t * alloc ( size_t size ) {
        size_t sizeWithHeader = size + (2 * sizeof (size_t)); // requested memory + header info
        int neededBlockIndex = ceil (log2(sizeWithHeader ) ); // exact power of 2 or the next biggest

        // memory block of the needed size exists
        if ( ! m_MemBlocks[neededBlockIndex].empty() )
            return allocBlock ( m_MemBlocks[neededBlockIndex].popFront(), neededBlockIndex );

        // find next biggest block
        for ( size_t i = neededBlockIndex; i < ALLOC_MEMORY_RANGE; i++ ) {
            if ( ! m_MemBlocks[i].empty() )
                return allocBlock ( splitBlock ( i, neededBlockIndex ), neededBlockIndex );
        }
        return nullptr;
    }
};

CHeap heap;

void   HeapInit    ( void * memPool, int memSize ) {
  heap = CHeap ( ( uintptr_t * ) memPool, memSize );
  heap.init();
}
void * HeapAlloc   ( int    size ) {
  return (void *) heap.alloc(size);
}
bool   HeapFree    ( void * blk ) {
  /* todo */
}
void   HeapDone    ( int  * pendingBlk ) {
    *pendingBlk = heap.done();
}

#ifndef __PROGTEST__
int main ( void )
{
  uint8_t       * p0, *p1, *p2, *p3, *p4;
  int             pendingBlk;
  static uint8_t  memPool[3 * 1048576];

  HeapInit ( memPool, 2097152 );
  assert ( ( p0 = (uint8_t*) HeapAlloc ( 512000 ) ) != NULL );
  memset ( p0, 0, 512000 );
  assert ( ( p1 = (uint8_t*) HeapAlloc ( 511000 ) ) != NULL );
  memset ( p1, 0, 511000 );
  assert ( ( p2 = (uint8_t*) HeapAlloc ( 26000 ) ) != NULL );
  memset ( p2, 0, 26000 );
  HeapDone ( &pendingBlk );
  assert ( pendingBlk == 3 );


  HeapInit ( memPool, 2097152 );
  assert ( ( p0 = (uint8_t*) HeapAlloc ( 1000000 ) ) != NULL );
  memset ( p0, 0, 1000000 );
  assert ( ( p1 = (uint8_t*) HeapAlloc ( 250000 ) ) != NULL );
  memset ( p1, 0, 250000 );
  assert ( ( p2 = (uint8_t*) HeapAlloc ( 250000 ) ) != NULL );
  memset ( p2, 0, 250000 );
  assert ( ( p3 = (uint8_t*) HeapAlloc ( 250000 ) ) != NULL );
  memset ( p3, 0, 250000 );
  assert ( ( p4 = (uint8_t*) HeapAlloc ( 50000 ) ) != NULL );
  memset ( p4, 0, 50000 );
  assert ( HeapFree ( p2 ) );
  assert ( HeapFree ( p4 ) );
  assert ( HeapFree ( p3 ) );
  assert ( HeapFree ( p1 ) );
  assert ( ( p1 = (uint8_t*) HeapAlloc ( 500000 ) ) != NULL );
  memset ( p1, 0, 500000 );
  assert ( HeapFree ( p0 ) );
  assert ( HeapFree ( p1 ) );
  HeapDone ( &pendingBlk );
  assert ( pendingBlk == 0 );


  HeapInit ( memPool, 2359296 );
  assert ( ( p0 = (uint8_t*) HeapAlloc ( 1000000 ) ) != NULL );
  memset ( p0, 0, 1000000 );
  assert ( ( p1 = (uint8_t*) HeapAlloc ( 500000 ) ) != NULL );
  memset ( p1, 0, 500000 );
  assert ( ( p2 = (uint8_t*) HeapAlloc ( 500000 ) ) != NULL );
  memset ( p2, 0, 500000 );
  assert ( ( p3 = (uint8_t*) HeapAlloc ( 500000 ) ) == NULL );
  assert ( HeapFree ( p2 ) );
  assert ( ( p2 = (uint8_t*) HeapAlloc ( 300000 ) ) != NULL );
  memset ( p2, 0, 300000 );
  assert ( HeapFree ( p0 ) );
  assert ( HeapFree ( p1 ) );
  HeapDone ( &pendingBlk );
  assert ( pendingBlk == 1 );


  HeapInit ( memPool, 2359296 );
  assert ( ( p0 = (uint8_t*) HeapAlloc ( 1000000 ) ) != NULL );
  memset ( p0, 0, 1000000 );
  assert ( ! HeapFree ( p0 + 1000 ) );
  HeapDone ( &pendingBlk );
  assert ( pendingBlk == 1 );


  return 0;
}
#endif /* __PROGTEST__ */

