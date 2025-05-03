#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <tuple>
#include <cassert>
#include <thread>
#include <mutex>
#include <algorithm>
#include <set>


class Transaction{
public:
    Transaction(uint64_t proxy_id, uint64_t user_seq, uint64_t n) {
        proxy_id_ = proxy_id;
        user_seq_ = user_seq;
        local_ordering_indices_.resize(n+1);
    }
    uint64_t proxy_id_;
    uint64_t user_seq_;
    std::vector<uint64_t> local_ordering_indices_;
};


int distance(Transaction* txn1, Transaction* txn2) {
    int count = 0;
    assert(txn1 != nullptr);
    assert(txn2 != nullptr);
    for (int i=1; i<txn1->local_ordering_indices_.size(); i++) {
        if (txn1->local_ordering_indices_[i] < txn2->local_ordering_indices_[i]) {
            count++;
        } else {
            count--;
        }
    }
    return count;
}


void calculateDistancesChunk(std::vector<Transaction*>& transactions, 
                             std::map<std::pair<uint64_t, uint64_t>, int>& localDistances, 
                             int start, int end) {
    for (int i = start; i < end; i++) {
        int limit = ((i / 100) + 1) * 100;
        if (transactions[i] == nullptr) {
            std::cout << "i = " << i  << " start = " << start << std::endl;
            fflush(stdout);
            assert(false);
        }
        for (int j = i + 1; j < limit && j < transactions.size(); j++) {
            localDistances[std::make_pair(i, j)] = distance(transactions[i], transactions[j]);
        }
    }
}

void calculateDistances(std::vector<Transaction*>& transactions, 
                        std::map<std::pair<uint64_t, uint64_t>, int>& distances) {
    const int numThreads = std::thread::hardware_concurrency(); // Get number of cores
    std::vector<std::thread> threads;
    std::vector<std::map<std::pair<uint64_t, uint64_t>, int>> localDistances(numThreads);

    int chunkSize = transactions.size() / numThreads;
    
    for (int t = 0; t < numThreads; t++) {
        int start = t * chunkSize;
        int end = (t == numThreads - 1) ? transactions.size() : start + chunkSize;
        threads.emplace_back(calculateDistancesChunk, std::ref(transactions), std::ref(localDistances[t]), start, end);
    }
    
    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "Calculating Distance Done" << std::endl;
    // Merge results from all thread-local maps into the final distances map
    for (const auto& localMap : localDistances) {
        distances.insert(localMap.begin(), localMap.end());
    }
    std::cout << "Merging Distance Done" << std::endl;
}

int abs(int value) {
    if (value < 0) {
        value = -value;
    }
    return value;
}

std::vector<std::pair<int, double>> calculateRatio (std::map<std::pair<uint64_t, uint64_t>, int> distances, std::map<uint64_t, uint64_t>& idx_scc, int n) {
    std::vector<std::pair<int, double>> ratio;
    std::map<int, int> total;
    std::map<int, int> correct;
    for (int i=0; i<=n; i++) {
        total[i] = correct[i] = 0;
    }
    for (auto& txn_pair: distances) {
        int distance = txn_pair.second;
        uint64_t scc_1 = idx_scc[txn_pair.first.first];
        uint64_t scc_2 = idx_scc[txn_pair.first.second];
        total[abs(distance)] += 1;
        if (distance >= 0) {
            correct[distance] += 1;
        } else if (scc_1 == scc_2){
            correct[abs(distance)] += 1;
        } else if (distance == -n) {
            std::cout << "REV" << txn_pair.first.first << " " << txn_pair.first.second << "\n";
        }
    }
    int d;
    for(d=0; d<=n; d++) {
        if (total[d] > 0) {
            std::cout << "total[" << d << "] = " << total[d] << std::endl;
            int x = d;
            double y = 1.0*correct[d]/total[d];
            auto p = std::make_pair(x, y);
            ratio.push_back(p);
        }
    }
    return ratio;
}



double calculateScore(std::map<uint64_t, uint64_t>& positions, std::map<uint64_t, uint64_t>& round) {
    std::vector<uint64_t> values;
    std::map<uint64_t, std::set<uint64_t>> round_txns;

    for (auto& it: positions) {
        values.push_back(it.first);
        auto r = round[it.first];
        round_txns[r].insert(it.first);
    }

    uint64_t total = 0;
    uint64_t correct = 0;

    sort(values.begin(), values.end());

    for (int i=0; i < values.size(); i++) {
        uint64_t t1 = values[i];
        auto r = round[i];
        for (auto& t2: round_txns[r]) {
            if (t2 > t1) {
                total += 1;
                if (positions[t1] <= positions[t2]) {
                    correct += 1;
                }
            }
        }
    }

    return 1.0 * correct / total;
}

void readFinalOrdering(const std::string& filename, std::map<uint64_t, uint64_t>& positions, std::map<uint64_t, uint64_t>& round, uint64_t count = 0x7FFFFFF) {
    std::fstream file(filename);

    std::cout << "Reading " << filename << std::endl;

    if (!file) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        return;
    }

    uint64_t a, b, c, d;
    while (count--) {
        if (!(file >> a >> b >> c >> d)) {
            if (file.eof()) {
                std::cout << "Reached end of file." << std::endl;
            }   else {
                std::cerr << "Error: Invalid input encountered in file." << std::endl;
            }
            file.close();
            return;;  // Exit loop if input fails
        }
        positions[b] = d;
        round[b] = d / 100;
    }
}

// void readOrdering(const std::string& filename, std::vector<std::tuple<int, int, int>>& data, uint64_t count = 0x7FFFFFF) {
//     std::fstream file(filename);

//     std::cout << "Reading " << filename << std::endl;

//     if (!file) {
//         std::cerr << "Error: Unable to open file " << filename << std::endl;
//         return;
//     }

//     uint64_t a, b, c;
//     while (count--) {
//         if (!(file >> a >> b >> c)) {
//             if (file.eof()) {
//                 std::cout << "Reached end of file." << std::endl;
//             }   else {
//                 std::cerr << "Error: Invalid input encountered in file." << std::endl;
//             }
//             file.close();
//             return;;  // Exit loop if input fails
//         }
//         // std::cout << a << " " << b << " " << c << std::endl;
//         data.emplace_back(a, b, c);
//     }
// }



int main(int argc, char* argv[]) {
    // int n = std::stoi(argv[1]);
    // std::cout << "n=" << n << std::endl;
    std::map<std::pair<uint64_t, uint64_t>, Transaction*> proxy_to_transaction;
    std::vector<Transaction*> transactions;
    
    std::vector<std::tuple<int, int, int>> final_ordering; 
    std::string file_name = "final_ordering_1";
    std::map<uint64_t, uint64_t> positions;
    std::map<uint64_t, uint64_t> round;
    readFinalOrdering(file_name, positions, round, 10000);

    std::cout << "score: " << calculateScore(positions, round) << std::endl;

    return 0;
}