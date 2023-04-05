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

int companyCounter = 0;
atomic_int workerCounter = 0;

class CProblemPackWrapper {
private:
public:
    explicit CProblemPackWrapper ( AProblemPack pPack )
    : m_ProblemPack ( std::move ( pPack ) ), m_SolvedCnt ( 0 ) {}

    bool isFullySolved () { return m_SolvedCnt == m_ProblemPack->m_Problems.size(); }

    AProblemPack m_ProblemPack;

    atomic_size_t m_SolvedCnt;
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

class CSafeSolver{
private:
        AProgtestSolver m_Solver;
        vector<AProblemWrapper> m_Problems;
        mutex m_MtxSolver; // protects the global solver
public:
    explicit CSafeSolver ( AProgtestSolver s )
    : m_Solver (std::move( s )) {}

    size_t solve ();
    bool addProblem ( const AProblemWrapper& problem );
    bool hasFreeCapacity() { return m_Solver->hasFreeCapacity(); }
};

using ASafeSolver = shared_ptr<CSafeSolver>;

class CCompanyWrapper;
using ACompanyWrapper = shared_ptr<CCompanyWrapper>;

template < typename T_ >
class CSafeQueue {
private:
  deque<T_> m_Queue;
  mutex m_Mtx;                    // controls access to the shared queue
  condition_variable m_CVEmpty;   // protects from removing items from an empty queue

public:
    void push ( T_ & item ) {
      unique_lock<mutex> ul (m_Mtx);
      m_Queue.push_back(item);
      m_CVEmpty.notify_one();
    }
    auto front () {
        unique_lock<mutex> ul (m_Mtx);
        return m_Queue.front();
    }
    /**
    * Pop and return the item at the front.
    */
    auto pop () {
        unique_lock<mutex> ul (m_Mtx);
        m_CVEmpty.wait(ul, [ this ] () { return ( ! m_Queue.empty() ); } );
        T_ item  = m_Queue.front();
        m_Queue.pop_front();
        return item;
    }
    bool empty() {
        unique_lock<mutex> ul (m_Mtx);
        return m_Queue.empty();
    }
};

bool CSafeSolver::addProblem ( const AProblemWrapper & problem ) {
    lock_guard<mutex> l ( m_MtxSolver );
    m_Problems.push_back(problem);
    return m_Solver->addProblem(problem->m_Problem);
}

size_t CSafeSolver::solve () {
        printf("Calling solve\n");
        lock_guard<mutex> l ( m_MtxSolver );
        size_t solvedCnt = m_Solver->solve();
        for ( const auto & problem : m_Problems )
            problem->m_ParentProblemPack->m_SolvedCnt++;
        return solvedCnt;
}

class COptimizer {
public:
    COptimizer ()
    : m_Solver ( make_shared<CSafeSolver> ( createProgtestSolver() ) ), m_FinishedReceivingCompaniesCnt ( 0 ) {}

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
    void stashSolver();

    bool allCompaniesFinishedReceiving () { return m_FinishedReceivingCompaniesCnt == m_Companies.size(); }

    ASafeSolver m_Solver;
    CSafeQueue<ASafeSolver> m_FullSolvers;
    atomic_size_t m_FinishedReceivingCompaniesCnt;
private:
    vector<ACompanyWrapper> m_Companies;
    mutex m_MtxReturn;

    vector<thread> m_Workers;

    condition_variable m_CVWorker;
    mutex m_MtxWorkerNoWork;

};

class CCompanyWrapper {
public:
    explicit CCompanyWrapper ( ACompany company, int id )
            : m_Company ( std::move(company) ), m_CompanyID ( id ) {}

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

    void notify () { m_CVReturner.notify_one(); }

    void startCompany ( COptimizer & optimizer );

private:
    ACompany m_Company;
    int m_CompanyID;
    mutex m_MtxReturnerNoWork;
    condition_variable m_CVReturner;

    mutex m_MtxProblemPackQueue;
    CSafeQueue<AProblemPackWrapper> m_ProblemPacks;
    bool m_AllProblemsReceived = false; // have all the problems already been given to solvers for processing?
};

bool COptimizer::getNewSolver () {
    m_Solver = make_shared<CSafeSolver> ( createProgtestSolver() );
    return m_Solver != nullptr;
}

void COptimizer::stashSolver() {
    printf("Stashing solver\n");
    m_FullSolvers.push ( m_Solver );
    m_CVWorker.notify_one();
}

void COptimizer::start ( int threadCount ) {
    for ( const auto & company : m_Companies )
        company->startCompany( *this );

    for ( int i = 0; i < threadCount; i++ )
        m_Workers.emplace_back ( &COptimizer::worker, this );
}

void COptimizer::stop () {
    printf("COptimizer::stop\n");
    for ( auto & company : m_Companies ) {
        company->m_ThrReceive.join();
        company->m_ThrReturn.join();
    }
    for ( auto & worker : m_Workers )
        worker.join();
}

void COptimizer::addCompany ( const ACompany& company ) {
    m_Companies.emplace_back (make_shared<CCompanyWrapper>(company, companyCounter++) );
}
void COptimizer::worker () {
    atomic_int id = workerCounter++;
    printf("Starting worker %d\n", id.load());

    // if all companies won't get new problems and solver queue is empty, break
    while ( ! ( allCompaniesFinishedReceiving() && m_FullSolvers.empty() ) ) {
        unique_lock<mutex> loko (m_MtxWorkerNoWork);
        m_CVWorker.wait( loko, [ this ] { return ! m_FullSolvers.empty(); } );
        loko.unlock();

        auto s = m_FullSolvers.pop();
        s->solve();

        loko.lock();
        for ( const auto & company : m_Companies )
            company->notify();
        loko.unlock();
    }
    printf("Stopping worker %d\n", id.load() );
}
void CCompanyWrapper::returner () {
    printf("Starting returner %d\n", m_CompanyID );
    // TODO: Deadlocks here I guess
    while ( ! ( m_AllProblemsReceived && m_ProblemPacks.empty() ) ) {
        unique_lock<mutex> lk ( m_MtxReturnerNoWork );
        m_CVReturner.wait ( lk, [ this ] {
            return ! m_ProblemPacks.empty () && m_ProblemPacks.front()->isFullySolved();
        } );
        lk.unlock();
        m_Company->solvedPack ( m_ProblemPacks.pop()->m_ProblemPack );
    }
    printf("Stopping returner %d\n", m_CompanyID);
    printf("==============================================================\n");
}
void CCompanyWrapper::receiver ( COptimizer & optimizer  ) {
    printf("Starting receiver %d\n", m_CompanyID);
    while ( AProblemPack pPack = m_Company->waitForPack() ) {
        printf("Pack size: &ld\n", pPack->m_Problems.size() );
        auto packWrapPtr = make_shared<CProblemPackWrapper> ( pPack );
        m_ProblemPacks.push ( packWrapPtr );
        for ( const auto & problem : pPack->m_Problems ) {
            optimizer.m_Solver->addProblem ( make_shared<CProblemWrapper>
                    ( CProblemWrapper ( problem, packWrapPtr ) ) );
            if ( ! optimizer.m_Solver->hasFreeCapacity() ) {
                optimizer.stashSolver();
                optimizer.getNewSolver();
            }
        }
        // here, if all problems of a given problem pack have been successfully given to solvers
    }
    // here, if there are no more problems to be given for processing
    m_AllProblemsReceived = true;
    optimizer.m_FinishedReceivingCompaniesCnt++;
    if ( optimizer.allCompaniesFinishedReceiving() )
        optimizer.stashSolver();
    printf("Stopping receiver %d\n", m_CompanyID );
}

void CCompanyWrapper::startCompany ( COptimizer & optimizer ) {
    printf("Starting company %d\n", m_CompanyID );
    m_ThrReceive = thread ( &CCompanyWrapper::receiver, this, ref(optimizer) );
    m_ThrReturn = thread ( &CCompanyWrapper::returner, this );
}

#ifndef __PROGTEST__

int main() {
    COptimizer optimizer;
    ACompanyTest company = std::make_shared<CCompanyTest>();
    optimizer.addCompany(company);
//    optimizer.start(4);
    optimizer.start(1);
    optimizer.stop();
    if (!company->allProcessed())
        throw std::logic_error("(some) problems were not correctly processed");
    return 0;
}

#endif /* __PROGTEST__ */
