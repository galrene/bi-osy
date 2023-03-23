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
class CCompanyWrapper {
public:
    CCompanyWrapper ( int threadCount );

    void worker ( void );
    void receiver ( void );
    void returner ( void );

private:
    shared_ptr<CCompany> m_Company;
    vector<thread> m_Workers;
    thread m_ThrReceive;

    thread m_ThrReturn;

    condition_variable m_CVWorker;
    mutex m_MtxWorkerNoWork;

    mutex m_MtxReturnerNoWork;

};

void CCompanyWrapper::worker ( void ) {
    unique_lock<mutex> lk (m_MtxWorkerNoWork);
    m_CVWorker.wait( lk, [ this ] { return ! m_PackReceived.empty(); } );
}

void CCompanyWrapper::returner ( void ) {
    unique_lock<mutex> lk ( m_MtxReturnerNoWork );
    m_Receiver.wait( lk, [ this ] { return ! m_PackReceived.empty(); } );
}

void CCompanyWrapper::receiver ( void ) {

}

CCompanyWrapper::CCompanyWrapper ( int threadCount ) {
    for ( int i = 0; i < threadCount; i++ )
        m_Workers.emplace_back ( &CCompanyWrapper::worker, this );
}

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

    queue<AProblemPack> m_PackReturn;
    mutex m_MtxReturn;

    shared_ptr<CProgtestSolver> m_Solver;

};



void COptimizer::start ( int threadCount ) {
}

void COptimizer::stop ( void ) {
}

void COptimizer::addCompany(ACompany company) {
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
