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
class CProblemPackWrapper {
private:
    AProblemPack m_ProblemPack;
public:
};


class CCompanyWrapper {
public:
    explicit CCompanyWrapper ( ACompany company )
    : m_Company ( company ) {}

    thread m_ThrReceive;
    void receiver (void );

    thread m_ThrReturn;
    void returner ( void );

    void startCompany ( void );

private:
    ACompany m_Company;

    condition_variable m_CVReturner;
    mutex m_MtxReturnerNoWork;

    queue<CProblemPackWrapper> m_ProblemPacks;
};

void CCompanyWrapper::returner ( void ) {
    unique_lock<mutex> lk ( m_MtxReturnerNoWork );
    m_CVReturner.wait( lk, [ this ] { return ! m_PackReceived.empty(); } );
}

void CCompanyWrapper::receiver ( void ) {
    while ( true ) {
        AProblemPack problem = m_Company->waitForPack();
        if (!problem)
            break;
        // iterate through problem in problem pack and fill the solver with them
        m_ProblemPacks.push(problem);
    }
}

void CCompanyWrapper::startCompany ( void ) {
    m_ThrReceive = thread ( &CCompanyWrapper::receiver, this );
    m_ThrReturn = thread ( &CCompanyWrapper::returner, this );
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

    void stop ( void );

    void addCompany ( ACompany company );

    void worker ( void );
private:
    vector<shared_ptr<CCompanyWrapper>> m_Companies;

    queue<AProblemPack> m_PackReturn;
    mutex m_MtxReturn;

    shared_ptr<CProgtestSolver> m_Solver;

    vector<thread> m_Workers;

    queue<AProgtestSolver> m_FullSolvers;

    condition_variable m_CVWorker;
    mutex m_MtxWorkerNoWork;

};

void COptimizer::start ( int threadCount ) {
    for ( const auto & company : m_Companies )
        company->startCompany();

    for ( int i = 0; i < threadCount; i++ )
        m_Workers.emplace_back ( &COptimizer::worker, this );
}

void COptimizer::stop ( void ) {

}

void COptimizer::addCompany ( ACompany company ) {
    m_Companies.emplace_back (make_shared<CCompanyWrapper>(company) );
}

void COptimizer::worker ( void ) {
    unique_lock<mutex> lk (m_MtxWorkerNoWork);
    m_CVWorker.wait( lk, [ this ] { return ! m_FullSolvers.empty(); } );

    // !! lock
    AProgtestSolver s = m_FullSolvers.front(); m_FullSolvers.pop();
    // !! unlock
    s->solve();

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
