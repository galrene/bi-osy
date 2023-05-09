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
 * [BLOCK_SIZE][PREVIOUS_BLOCK][NEXT_BLOCK][BLOCK_SIZE]
 * where last bit of size is allocated flag
 */
struct CBlock {
    size_t m_Size;
    CBlock * m_Prev;
    CBlock * m_Next;
    CBlock ()
    : m_Size ( 0 ), m_Prev ( nullptr ), m_Next ( nullptr ) {}
};

/**
 * Bidirectional for memory blocks.
 */
class CBiLL {
private:
    CBlock * m_Front = nullptr;
public:
    bool empty () { return m_Front == nullptr; }
    void pushFront ( CBlock * item ) {
        if ( ! m_Front ) {
            m_Front = item;
            return;
        }
        CBlock * prevFront = m_Front;
        prevFront->m_Prev = item;
        m_Front = item;
        m_Front->m_Next = prevFront;
    }
    bool pop ( CBlock * item ) {
        // the case of a single item in the LL
        if ( item == m_Front
             && ! m_Front->m_Next ) {
            m_Front = nullptr;
            return true;
        }
        CBlock * curr = m_Front;
        while ( curr ) {
            if ( curr == item ) {
                if ( curr->m_Prev )
                    curr->m_Prev->m_Next = curr->m_Next;
                if ( curr->m_Next )
                    curr->m_Next->m_Prev = curr->m_Prev;
                return true;
            }
            curr = curr->m_Next;
        }
        return false;
    }
};

class CHeap {
private:
    /* Linked lists of sizes 2^i */
    CBiLL * m_MemBlockLists[32] = {};
    uintptr_t * m_Begin;
    size_t m_Size;

    size_t m_AllocatedCnt;

public:
    CHeap ( uintptr_t * begin, size_t size )
    : m_Begin ( begin ), m_Size ( size ), m_AllocatedCnt ( 0 ) {}
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

