#pragma once

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <random>
#include <algorithm>
#include <atomic>
#include "platform/proto/resdb.pb.h"

namespace resdb {
namespace common  {

class TpccTransactionGenerator {
    
    public:
        TpccTransactionGenerator ();
        int fillTpccTransaction(Request* request);
    
    private:
        int fillNewOrderTransaction (resdb::NewOrderTransaction* txn);
        int fillPaymentTransaction(resdb::PaymentTransaction* txn);
        int fillOrderStatusTransaction(resdb::OrderStatusTransaction* txn);
        int fillDeliveryTransaction(resdb::DeliveryTransaction* txn);
        int fillStockLevelTransaction(resdb::StockLevelTransaction* txn);

        // Function to generate a non-uniform random number (NURand)
        inline int NURand(int A, int x, int y) {
            static std::uniform_int_distribution<> disC(0, 999); // Range for C
            static int C = disC(gen); // C is a constant per execution

            std::uniform_int_distribution<> disA(0, A - 1);       // Range for A
            std::uniform_int_distribution<> disXY(x, y);         // Range for x to y
            return (((disA(gen) | disXY(gen)) + C) % (y - x + 1)) + x;
        }

        // Function to generate a random double between [low, high]
        inline double generateRandomDouble(double low, double high) {
            // Generate a random integer and scale it to [low, hight]
            static std::uniform_int_distribution<> disDouble(0, 10000); // Range for C
            return low + static_cast<double>(disDouble(gen)) / 10000 * (high - low);
        }

        // Function to generate a random int between [low, high]
        inline int generateRandomInt(int low, int high) {
            std::uniform_int_distribution<int> dis(low, high); // Define the range
            return dis(gen); // Generate a random integer between low and high
        }

        inline std::string getCurrentTimestamp() {
            // Get the current time as a time_point
            auto now = std::chrono::system_clock::now();

            // Convert to time_t to format the time
            std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);

            // Convert to tm structure
            std::tm now_tm = *std::localtime(&now_time_t);

            // Format the time as a string
            std::ostringstream oss;
            oss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
            return oss.str();
        }

        // Function to generate C_LAST based on a number between 0 and 999
        inline std::string generateCustomerLastName(int number) {
            if (number >= 1000) {
                number  = NURand(255, 0, 999);
            }

            // List of syllables
            const std::vector<std::string> syllables = {
                "BAR", "OUGHT", "ABLE", "PRI", "PRES", "ESE", 
                "ANTI", "CALLY", "ATION", "EING"
            };

            // Extract the digits
            int hundreds = number / 100;
            int tens = (number / 10) % 10;
            int ones = number % 10;

            // Construct the last name
            std::string lastName = syllables[hundreds] + syllables[tens] + syllables[ones];

            return lastName;
        }

        // Use a random number generator
        std::random_device rd; // Obtain a random seed
        std::mt19937 gen; // Seed the generator
};



} // common
}  // namespace resdb

