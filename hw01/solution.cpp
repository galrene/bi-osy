#ifndef __PROGTEST__

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <climits>
#include <cfloat>
#include <cassert>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <string>
#include <vector>
#include <array>
#include <iterator>
#include <set>
#include <list>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <stack>
#include <deque>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <stdexcept>
#include <condition_variable>
#include <pthread.h>
#include <semaphore.h>
#include "progtest_solver.h"
#include "sample_tester.h"

using namespace std;
#endif /* __PROGTEST__ */


using namespace std;
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
class COptimizer {
public:
    static bool usingProgtestSolver(void) {
        return true;
    }

    static void checkAlgorithm(AProblem problem) {
        // dummy implementation if usingProgtestSolver() returns true
    }

    void start(int threadCount);

    void stop(void);

    void addCompany(ACompany company);

private:
    queue<ACompany> m_Companies;
    mutex m_MtxComp;

    queue<AProblemPack> m_PackReceived;
    mutex m_MtxReceived;
    queue<AProblemPack> m_PackReturn;
    mutex m_MtxReturn;

    vector<thread> m_Workers;
    mutex m_MtxWorkerNoWork;
    condition_variable m_CVWorker;

    shared_ptr<CProgtestSolver> m_Solver;

    void createWorker(void);
    void worker ( void );
};

void COptimizer::worker ( void ) {
    unique_lock lk (m_MtxWorkerNoWork);
    m_CVWorker.wait( lk, [ this ] { return ! m_PackReceived.empty(); } );
}

void COptimizer::createWorker ( void ) {
    m_Workers.emplace_back ( &COptimizer::worker, this );
}

void COptimizer::start ( int threadCount ) {
    createCommThreads();
    for ( int i = 0; i < threadCount; i++ )
        createWorker();
}

void COptimizer::stop ( void ) {

}

void COptimizer::addCompany(ACompany company) {
    lock_guard<mutex> lg ( m_MtxComp ); // unnecessary?
    m_Companies.push( company );
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef __PROGTEST__

int main(void) {
    COptimizer optimizer;
    ACompanyTest company = std::make_shared<CCompanyTest>();
    optimizer.addCompany(company);
    optimizer.start(4);
    optimizer.stop();
    if (!company->allProcessed())
        throw std::logic_error("(some) problems were not correctly processsed");
    return 0;
}

#endif /* __PROGTEST__ */
