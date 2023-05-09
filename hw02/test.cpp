#ifndef __PROGTEST__
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <cmath>
using namespace std;
#endif /* __PROGTEST__ */


/**
 * A memory block.
 *
 * Free block in memory consists of: 4 * uintptr_8 = 32B
 * [BLOCK_SIZE][PREVIOUS_BLOCK_PTR][NEXT_BLOCK_PTR][BLOCK_SIZE]
 * where last bit of size is allocated flag
 */


/**
 * Bidirectional for memory blocks.
 */
class CBiLL {
private:
    uintptr_t * m_Front = nullptr;
public:
    bool empty () { return m_Front == nullptr; }

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
    void pop ( uintptr_t * item ) {
        auto prev = (uintptr_t *) item[1];
        auto next = (uintptr_t *) item[2];
        // the case of a single item in the LL
        if ( item == m_Front )
            m_Front = next;
        if ( prev )
            prev[2] = (uintptr_t) next;
        if ( next )
            next[1] = (uintptr_t) prev;
    }
};

class CHeap {
private:
    /* Linked lists of sizes 2^i */
    CBiLL * m_MemBlocks[32] = {};
    uintptr_t * m_Begin;
    int m_Size;
    size_t m_AllocatedCnt; // number of allocated blocks
    /**
     * Split a number into powers of two.
     */
    void split ( int n ) {
        uintptr_t * currPos = m_Begin;
        for ( size_t i = 0; n > 0; i++ ) {
            if ( n & 1 ) {
                createBlock (currPos, i );
                currPos += 1 << i;
            }
            n >>= 1;
        }
    }
    void createBlock ( uintptr_t * address, size_t i ) {
        address[0] = 1 << i;
        address[1] = 0;
        address[2] = 0;
        address[ ( (1 << i) / 8 ) - 1] = (1 << i); // uintptr_t = 8B, size is in Bytes
        m_MemBlocks[i]->pushFront ( address );
    }

public:
    CHeap ( uintptr_t * begin, int size )
    : m_Begin ( begin ), m_Size ( size ), m_AllocatedCnt ( 0 ) {}

    void init () {
        // TODO: check size and begin ptr validity
    }
};

void   HeapInit    ( void * memPool, int memSize ) {
  /* todo */
}
void * HeapAlloc   ( int    size ) {
  /* todo */
}
bool   HeapFree    ( void * blk ) {
  /* todo */
}
void   HeapDone    ( int  * pendingBlk ) {
  /* todo */
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

