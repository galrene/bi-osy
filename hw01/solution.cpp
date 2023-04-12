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
#include <utility>
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

//atomic_int workerCounter = 0;

class CProblemPackWrapper {
private:
public:
    explicit CProblemPackWrapper ( AProblemPack pPack )
    : m_ProblemPack ( std::move ( pPack ) ) {}

    bool isSolved () {
        for ( const auto & problem : m_ProblemPack->m_Problems )
            if ( problem->m_MaxProfit == 0 )
                return false;
        return true;
    }
    AProblemPack m_ProblemPack;
};

using AProblemPackWrapper = shared_ptr<CProblemPackWrapper>;

class CProblemWrapper {
public:
    explicit CProblemWrapper ( AProblem prob, AProblemPackWrapper probPack )
    : m_Problem ( std::move ( prob ) ), m_ParentProblemPack ( std::move ( probPack ) ) {}
    AProblem m_Problem;
    AProblemPackWrapper m_ParentProblemPack;
};

using AProblemWrapper = shared_ptr<CProblemWrapper>;

class CSolverWrapper{
private:
        AProgtestSolver m_Solver;
        vector<AProblemWrapper> m_Problems;
public:
    explicit CSolverWrapper ( AProgtestSolver s )
    : m_Solver ( std::move ( s ) ) {}

    size_t solve () { return m_Solver->solve(); }
    bool addProblem ( const AProblemWrapper& problem ) {
        m_Problems.push_back ( problem );
        return m_Solver->addProblem ( problem->m_Problem );
    }
    bool hasFreeCapacity() {  return m_Solver->hasFreeCapacity(); }
};
using ASolverWrapper = shared_ptr<CSolverWrapper>;

class CCompanyWrapper;
using ACompanyWrapper = shared_ptr<CCompanyWrapper>;


class CSafePPackQueue {
private:
  deque<AProblemPackWrapper> m_Queue;
  mutex m_Mtx;                    // controls access to the shared queue
  condition_variable m_CVEmpty;   // protects from removing items from an empty queue

public:
    void push ( AProblemPackWrapper & item ) {
      unique_lock<mutex> ul ( m_Mtx );
      m_Queue.push_back ( item );
      m_CVEmpty.notify_all();
    }
    /**
    * Pop and return the item at the front.
    */
    auto pop () {
        unique_lock<mutex> ul ( m_Mtx );
        m_CVEmpty.wait ( ul, [ this ] {
            // nullptr at front means all packs that were ever received were already solved
            return ( ! m_Queue.empty() && m_Queue.front() == nullptr )
                   || ( ! m_Queue.empty() && m_Queue.front()->isSolved() );
        } );
        AProblemPackWrapper item  = m_Queue.front();
        m_Queue.pop_front();
        return item;
    }
    void notify() { unique_lock<mutex> ul ( m_Mtx ); m_CVEmpty.notify_all(); }
};

class COptimizer {
public:
    COptimizer ()
    : m_Solver ( make_shared<CSolverWrapper> ( createProgtestSolver() ) ), m_FinishedReceivingCompaniesCnt ( 0 ), m_KillWorkers ( false ) {}

    static bool usingProgtestSolver() { return true; }
    static void checkAlgorithm(AProblem problem) {}

    void start(int threadCount);

    void stop ();

    void addCompany ( const ACompany& company );

    void worker ();
    /**
    * Constructs a new empty solver.
    */
    bool getNewSolver ();
    /**
     * Enqueues a solver and notifies worker.
     */
    void stashSolver ();
    bool allCompaniesFinishedReceiving () { return m_FinishedReceivingCompaniesCnt.load() == m_Companies.size(); }
    void notifyCompanies();


    mutex m_MtxSolver;
    ASolverWrapper m_Solver;
    queue<ASolverWrapper> m_FullSolvers;
    condition_variable m_CVWorker;
    atomic_size_t m_FinishedReceivingCompaniesCnt;
    vector<thread> m_Workers;
    bool m_KillWorkers; // worker kill flag when all companies finished receiving problems
private:
    vector<ACompanyWrapper> m_Companies;
};

class CCompanyWrapper {
public:
    explicit CCompanyWrapper ( ACompany company )
            : m_Company ( std::move(company) ) {}

    thread m_ThrReceive;
    /**
    * Receive unsolved problem packs.
    */
    void receiver ( COptimizer & optimizer  );

    thread m_ThrReturn;
    /**
    * Return solved problem packs.
    */
    void returner ();

    void notifyQueue () { m_ProblemPacks.notify(); }

    void startCompany ( COptimizer & optimizer );

private:
    ACompany m_Company;
    CSafePPackQueue m_ProblemPacks;
};
/**
 * Notifies returner thread of each company under it's lock.
 */
void COptimizer::notifyCompanies () {
    for ( const auto & company : m_Companies )
        company->notifyQueue();
}

bool COptimizer::getNewSolver () {
    m_Solver = make_shared<CSolverWrapper> ( createProgtestSolver() );
    return m_Solver != nullptr;
}

void COptimizer::stashSolver () {
//    if ( m_Solver ) fprintf ( stderr, "Stashing solver filled: %ld\n", m_Solver->size() );
    m_FullSolvers.push ( m_Solver );
}

void COptimizer::start ( int threadCount ) {
    for ( const auto & company : m_Companies )
        company->startCompany( *this );

    for ( int i = 0; i < threadCount; i++ )
        m_Workers.emplace_back ( &COptimizer::worker, this );
}

void COptimizer::stop () {
//    fprintf ( stderr, "COptimizer::stop\n");
    for ( auto & worker : m_Workers )
        worker.join();
    for ( auto & company : m_Companies ) {
        company->m_ThrReceive.join();
        company->m_ThrReturn.join();
    }
}
void COptimizer::addCompany ( const ACompany& company ) {
    m_Companies.emplace_back ( make_shared<CCompanyWrapper> ( company ) );
}
void COptimizer::worker () {
//    atomic_int id = workerCounter++; // debug
//    fprintf ( stderr, "WORKER: Starting %d\n", id.load() );
    // if all companies won't get new problems and solver queue is empty, break

    while ( true ) {
        unique_lock<mutex> lk ( m_MtxSolver );
        m_CVWorker.wait ( lk, [ this ] { return ! m_FullSolvers.empty() || m_KillWorkers ; } );
        if ( m_FullSolvers.empty() && m_KillWorkers )
            break;
        auto s = m_FullSolvers.front(); m_FullSolvers.pop();
        lk.unlock();

        s->solve();
//        fprintf ( stderr, "WORKER: Notifying companies\n" );
        notifyCompanies();
    }

//    fprintf ( stderr, "WORKER: Stopping %d\n", id.load() );
}
void CCompanyWrapper::returner () {
//    fprintf ( stderr, "RETURNER: start%d\n", m_CompanyID );
    while ( auto pack = m_ProblemPacks.pop() ) {
//        fprintf ( stderr, "RETURNER: returning pack %d\n", m_CompanyID );
        m_Company->solvedPack ( pack->m_ProblemPack );
    }
//    fprintf ( stderr, "RETURNER: stop %d\n", m_CompanyID);
}
void CCompanyWrapper::receiver ( COptimizer & optimizer  ) {
//    fprintf ( stderr, "RECEIVER: start %d\n", m_CompanyID);
    while ( AProblemPack pPack = m_Company->waitForPack() ) {
        auto packWrapPtr = make_shared<CProblemPackWrapper> ( pPack );
        m_ProblemPacks.push ( packWrapPtr );

        unique_lock<mutex> lk ( optimizer.m_MtxSolver );
        for ( const auto & problem : pPack->m_Problems ) {
            optimizer.m_Solver->addProblem ( make_shared<CProblemWrapper>
                    ( CProblemWrapper ( problem, packWrapPtr ) ) );
            if ( ! optimizer.m_Solver->hasFreeCapacity() ) {
                optimizer.stashSolver();
                optimizer.getNewSolver();
            }
        }
        lk.unlock();
        // here, if all problems of a given problem pack have been successfully given to solvers
    }
    // here, if there are no more problems to be given for processing
    AProblemPackWrapper eofPack = nullptr;
    m_ProblemPacks.push ( eofPack );
    optimizer.m_FinishedReceivingCompaniesCnt++;
    // stash the last (not necessarily full) solver
    unique_lock<mutex> lk ( optimizer.m_MtxSolver );
    if ( optimizer.allCompaniesFinishedReceiving() ) {
        optimizer.stashSolver();
        optimizer.m_KillWorkers = true;
        optimizer.m_CVWorker.notify_all();
    }
//    fprintf ( stderr, "RECEIVER: stop %d\n", m_CompanyID );
}

void CCompanyWrapper::startCompany ( COptimizer & optimizer ) {
//    fprintf ( stderr, "Starting company %d\n", m_CompanyID );
    m_ThrReceive = thread ( &CCompanyWrapper::receiver, this, ref(optimizer) );
    m_ThrReturn = thread ( &CCompanyWrapper::returner, this );
}

#ifndef __PROGTEST__
int main() {
    int runs = 10000;
    int c = 10;
    int w = 6;

    for ( int i = 0; i < runs; i++ ) {
//        fprintf ( stderr, "=====================BEGIN===============================\n");
        fprintf ( stderr, "%d\n", i );
        COptimizer optimizer;
        vector<ACompanyTest> companies;
        for ( int j = 0; j < c; j++ ) {
            ACompanyTest company = std::make_shared<CCompanyTest>();
            companies.push_back(company);
        }
        for ( const auto & company : companies )
            optimizer.addCompany(company);
        optimizer.start(w);
        optimizer.stop();
        for ( const auto & company : companies )
            if (!company->allProcessed())
                throw std::logic_error("(some) problems were not correctly processed");
//        fprintf ( stderr, "=====================END======================================\n");
    }
    return 0;
}

#endif /* __PROGTEST__ */
