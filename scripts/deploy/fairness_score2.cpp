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

void sortTransactions(std::vector<Transaction*>& transactions) {
    std::sort(transactions.begin(), transactions.end(), [](const Transaction* a, const Transaction* b) {
        return a->average_idx_ < b->average_idx_;
    });
}

double calculateScore(std::vector<Transaction*> transactions, uint64_t block_size) {
    uint64_t total = 0; 
    uint64_t correct = 0; 
    for (int i=0; i<transactions.size(); i++) {
        std::cout << "user_seq: " << transactions[i]->user_seq_ 
                << " average_idx: " << transactions[i]->average_idx_
                << " scc: " << transactions[i]->scc << std::endl;
        for (int j=i+1; j<=i+block_size && j<transactions.size(); j++) {
            total += 1;
            if (transactions[i]->scc <= transactions[j]->scc) {
                correct += 1;
            }
        }
    }

    return 1.0 * correct / total;
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
        idx_scc[c] = d;
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



int main(int argc, char* argv[]) {
    uint64_t block_size = 15;
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
        txn->scc = idx_scc[c];
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

    auto score = calculateScore(transactions, block_size);

    std::cout << "score = " << score << std::endl;

    return 0;
}