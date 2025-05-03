#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <tuple>
#include <cassert>
#include <thread>
#include <mutex>
#include <algorithm>


class Transaction{
public:
    Transaction(uint64_t proxy_id, uint64_t user_seq, uint64_t n) {
        proxy_id_ = proxy_id;
        user_seq_ = user_seq;
        local_ordering_indices_.resize(n+1);
    }

    void calculate_average_idx_() {
        double total = 0;
        for (auto v: local_ordering_indices_) {
            total += v;
        }
        average_idx_ = total / local_ordering_indices_.size();
    }

    uint64_t proxy_id_;
    uint64_t user_seq_;
    std::vector<uint64_t> local_ordering_indices_;
    double average_idx_;
    uint64_t scc;
};


int distance(Transaction* txn1, Transaction* txn2) {
    int count = 0;
    assert(txn1 != nullptr);
    assert(txn2 != nullptr);
    for (int i=1; i<txn1->local_ordering_indices_.size(); i++) {
        if (txn1->local_ordering_indices_[i] < txn2->local_ordering_indices_[i]) {
            count++;
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
        auto user_seq1 = transactions[i]->user_seq_;
        auto user_seq2 = transactions[i]->user_seq_;
        for (int j = i + 1; j < limit && j < transactions.size(); j++) {
            localDistances[std::make_pair(user_seq1, user_seq2)] = 
                distance(transactions[i], transactions[j]);
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
        total[distance] += 1;
        if (scc_1 <= scc_2){
            correct[distance] += 1;
        } else {
            std::cout << "REV" << txn_pair.first.first << " " << txn_pair.first.second << "\n";
        }
    }
    int d;
    for(d=-n; d<=n; d++) {
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

void readFinalOrdering(const std::string& filename, std::vector<std::tuple<int, int, int>>& data, std::map<uint64_t, uint64_t>& idx_scc, uint64_t count = 0x7FFFFFF) {
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
        // std::cout << a << " " << b << " " << c << std::endl;
        data.emplace_back(a, b, c);
        idx_scc[b] = d;
    }
}

void readOrdering(const std::string& filename, std::vector<std::tuple<int, int, int>>& data, uint64_t count = 0x7FFFFFF) {
    std::fstream file(filename);

    std::cout << "Reading " << filename << std::endl;

    if (!file) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        return;
    }

    uint64_t a, b, c;
    while (count--) {
        if (!(file >> a >> b >> c)) {
            if (file.eof()) {
                std::cout << "Reached end of file." << std::endl;
            }   else {
                std::cerr << "Error: Invalid input encountered in file." << std::endl;
            }
            file.close();
            return;;  // Exit loop if input fails
        }
        // std::cout << a << " " << b << " " << c << std::endl;
        data.emplace_back(a, b, c);
    }
}


// void sortTransactions(std::vector<Transaction*>& transactions) {
//     std::sort(transactions.begin(), transactions.end(), [](const Transaction* a, const Transaction* b) {
//         return a->user_seq_ < b->user_seq_;
//     });
// }

void sortTransactions(std::vector<Transaction*>& transactions) {
    std::sort(transactions.begin(), transactions.end(), [](const Transaction* a, const Transaction* b) {
        return a->average_idx_ < b->average_idx_;
    });
}



int main(int argc, char* argv[]) {
    uint64_t batch_size_ = 15;
    int n = std::stoi(argv[1]);
    std::cout << "n=" << n << std::endl;
    std::map<std::pair<uint64_t, uint64_t>, Transaction*> proxy_to_transaction;
    std::vector<Transaction*> transactions;
    
    std::vector<std::tuple<int, int, int>> final_ordering; 
    std::string file_name = "final_ordering_1";
    std::map<uint64_t, uint64_t> idx_scc;
    readFinalOrdering(file_name, final_ordering, idx_scc, 10000);

    // Print the read data

    transactions.resize(final_ordering.size());

    for (const auto& [a, b, c] : final_ordering) {
        auto txn = new Transaction(a, b, n);
        proxy_to_transaction[std::make_pair<uint64_t, uint64_t>(a, b)] = txn;
        transactions[c] = txn;
    }

    for (int i=1; i<=n; i++) {
        std::vector<std::tuple<int, int, int>> local_ordering; 
        std::string filename = "local_ordering_" + std::to_string(i);
        readOrdering(filename, local_ordering);
        for (const auto& [a, b, c] : local_ordering) {
            auto txn = proxy_to_transaction[std::make_pair<uint64_t, uint64_t>(a, b)];
            if (txn != nullptr) {
                txn->local_ordering_indices_[i] = c;
            }
        }
    }

    for (auto & txn : transactions) {
        txn->calculate_average_idx_();
    }

    sortTransactions(transactions);

    std::map<std::pair<uint64_t, uint64_t>, int> distances;
    // calculateDistances(transactions, distances);

    for (uint64_t i = 10; i < transactions.size(); i++) {
        if (transactions[i]->user_seq_ % batch_size_ == 1) {
            for (uint64_t j = i-batch_size_/2; j <= i+batch_size_/2 && j < transactions.size(); j++) {
                if (j == i) {
                    continue;
                }
                distances[std::make_pair(transactions[i]->user_seq_, transactions[j]->user_seq_)] = 
                distance(transactions[i], transactions[j]);
            }
        }
    }

    // for (auto& d: distances) {
    //     std::cout << d.first.first << " " << d.first.second << " " << d.second << std::endl;
    // }

    auto ratio = calculateRatio(distances, idx_scc, n);

    std::cout << "Calculating Ratio Done.\n";

    for (auto p: ratio) {
        std::cout << p.second << std::endl;
    }

    return 0;
}