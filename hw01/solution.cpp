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
class CProblemPackWrapper {
private:
public:
    explicit CProblemPackWrapper ( AProblemPack pPack )
    : m_ProblemPack ( std::move ( pPack ) ) {}

    bool isFullySolved () { return m_SolvedCnt == m_ProblemPack->m_Problems.size(); }
    /**
     * Increment solved amount, check whether the pack isn't fully solved after this.
     */
    void solveOne ();

    AProblemPack m_ProblemPack;

    mutex m_MtxSolvedPackFlag;
    bool m_SolvedPack = false;
    atomic_size_t m_SolvedCnt = 0;
};

using AProblemPackWrapper = shared_ptr<CProblemPackWrapper>;

void CProblemPackWrapper::solveOne () {
    m_SolvedCnt++;
    if ( isFullySolved() ) {
        lock_guard<mutex> l ( m_MtxSolvedPackFlag );
        m_SolvedPack = true;
    }
}

class CProblemWrapper {
public:
    explicit CProblemWrapper ( AProblem prob, shared_ptr<CProblemPackWrapper> probPack )
    : m_Problem ( std::move ( prob ) ), m_ParentProblemPack ( std::move ( probPack ) ) {}
    AProblem m_Problem;
    shared_ptr<CProblemPackWrapper> m_ParentProblemPack;
};

class CSafeSolver{
private:
        AProgtestSolver m_Solver;
        vector<shared_ptr<CProblemWrapper>> m_Problems;
        mutex m_MtxSolver; // protects the global solver
public:
    explicit CSafeSolver ( AProgtestSolver s )
    : m_Solver (std::move( s )) {}

    size_t solve ();
    bool addProblem ( const shared_ptr<CProblemWrapper>& problem );
    bool hasFreeCapacity() { return m_Solver->hasFreeCapacity(); }
};

class CCompanyWrapper;
using ACompanyWrapper = shared_ptr<CCompanyWrapper>;

template < typename T_ >
class CSafeQueue {
private:
  deque<T_> m_Queue;
  mutex m_Mtx;        // controls access to the shared queue
  condition_variable m_CVEmpty;   // protects from removing items from an empty buffer

public:
    void push ( T_ & item ) {
      unique_lock<mutex> ul (m_Mtx);
      m_Queue.push_back(item);
      m_CVEmpty.notify_one();
    }
    void push ( T_ && item ) {
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
    // TODO: Do I need to lock here? -> I do, right?
    bool empty() {
        unique_lock<mutex> ul (m_Mtx);
        return m_Queue.empty();
    }
};

bool CSafeSolver::addProblem ( const shared_ptr<CProblemWrapper> & problem ) {
    lock_guard<mutex> l ( m_MtxSolver );
    m_Problems.push_back(problem);
    return m_Solver->addProblem(problem->m_Problem);
}

size_t CSafeSolver::solve () {
        lock_guard<mutex> l ( m_MtxSolver );
        size_t solvedCnt = m_Solver->solve();
        for ( const auto & problem : m_Problems )
            problem->m_ParentProblemPack->solveOne();
        return solvedCnt;
}

class COptimizer {
public:
    COptimizer ()
    : m_Solver ( make_shared<CSafeSolver> ( createProgtestSolver() ) ), m_FinishedReceivingCompaniesCnt ( 0 ) {}

    static bool usingProgtestSolver() { return true; }
    // dummy implementation if usingProgtestSolver() returns true
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

    bool allCompaniesFinishedSending () { return m_FinishedReceivingCompaniesCnt == m_Companies.size(); }

    shared_ptr<CSafeSolver> m_Solver;
    CSafeQueue<shared_ptr<CSafeSolver>> m_FullSolvers;
    atomic_size_t m_FinishedReceivingCompaniesCnt;
private:
    vector<shared_ptr<CCompanyWrapper>> m_Companies;
    mutex m_MtxReturn;

    vector<thread> m_Workers;

    condition_variable m_CVWorker;
    mutex m_MtxWorkerNoWork;

};

class CCompanyWrapper {
public:
    explicit CCompanyWrapper ( ACompany company )
            : m_Company ( std::move(company) ) {}

    thread m_ThrReceive;
    void receiver ( COptimizer & optimizer  );

    thread m_ThrReturn;
    void returner ();

    void notify () { m_CVReturner.notify_one(); }

    void startCompany ( COptimizer & optimizer );

private:
    ACompany m_Company;

    mutex m_MtxReturnerNoWork;
    condition_variable m_CVReturner;

    mutex m_MtxProblemPackQueue;
    CSafeQueue<shared_ptr<CProblemPackWrapper>> m_ProblemPacks;
    bool m_AllProblemsReceived = false; // have all the problems already been given to solvers for processing?
};


bool COptimizer::getNewSolver () {
    m_Solver = make_shared<CSafeSolver> ( createProgtestSolver() );
    return m_Solver != nullptr;
}

void COptimizer::stashSolver() {
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

}

void COptimizer::addCompany ( const ACompany& company ) {
    m_Companies.emplace_back (make_shared<CCompanyWrapper>(company) );
}
void COptimizer::worker () {
    while ( true ) {
        // TODO: if all companies are solved and solver queue is empty, break
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
}
/**
 * Return solved problem packs.
 */
void CCompanyWrapper::returner () {
    while ( true ) {
        unique_lock<mutex> lk ( m_MtxReturnerNoWork );
        m_CVReturner.wait ( lk, [ this ] {
            return ! m_ProblemPacks.empty () && m_ProblemPacks.front()->m_SolvedPack;
        } );
        lk.unlock();
        m_Company->solvedPack ( m_ProblemPacks.pop()->m_ProblemPack );
    }
}

/**
 * Receive unsolved problem packs.
 */
void CCompanyWrapper::receiver ( COptimizer & optimizer  ) {
    while ( AProblemPack pPack = m_Company->waitForPack() ) {
        /* TODO: SUS - check what happens here - creating shared_ptr<CPPackWrapper> directly from PPack pointer
            should call the CPPackWrapper constructor if correct. */
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
    if ( optimizer.allCompaniesFinishedSending() )
        optimizer.stashSolver();
}

void CCompanyWrapper::startCompany ( COptimizer & optimizer ) {
    m_ThrReceive = thread ( &CCompanyWrapper::receiver, this, ref(optimizer) );
    m_ThrReturn = thread ( &CCompanyWrapper::returner, this );
}

#ifndef __PROGTEST__

/**
 * TODO: How do I know that a company has all it's problems solved and therefore I can stop it's returner?
 */

int main() {
    COptimizer optimizer;
    ACompanyTest company = std::make_shared<CCompanyTest>();
    optimizer.addCompany(company);
    optimizer.start(4);
    optimizer.stop();
    if (!company->allProcessed())
        throw std::logic_error("(some) problems were not correctly processed");
    return 0;
}

#endif /* __PROGTEST__ */
