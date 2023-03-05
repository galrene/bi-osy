#include <iostream>
#include <thread>
#include <vector>
#include <cmath>
#include <chrono>

using namespace std;

void calcSum ( long from, long to, long & ret ) {
    fprintf ( stderr, "I'm working here yo\n" );
    long res = 0;
    for ( ; from < to; from++ )
        res += ( sqrt ( from + 1 ) + from )
               /
               sqrt ( pow ( from, 2 ) +  from + 1 );
    ret = res;
}


int main ( int argc, char * argv[] ) {
    if ( argc < 3 || *argv[1] - '0' < 0 || *argv[2] - '0' < 0 ) {
        cout << "Expected ./main <m> <threadCount>" << endl;
        return 1;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    long sumMembers = stoi(argv[1]);
    long threadCount = stoi(argv[2]);

    vector<thread> thr;
    vector<long> results;
    for ( int i = 0; i < threadCount; i ++ )
        results.push_back ( 0 );
    long cntPerThread = sumMembers/threadCount;

    for ( int i = 0; i < threadCount; i ++ )
        thr.emplace_back ( calcSum, i * cntPerThread, ( ( i + 1 ) * cntPerThread ) - 1, ref(results[i]) );
    for ( int i = 0; i < threadCount; i ++ )
        thr[i].join();

    // merge results
    long long finalResult = 0;
    for ( const auto & res : results )
        finalResult += res;

    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
    std::cout << "Elapsed time: " << elapsed_time << " seconds" << std::endl;

    cout << finalResult << " is the final result" << endl;

    return 0;
}