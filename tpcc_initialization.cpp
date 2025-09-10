#include <iostream>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <random>
#include <chrono>
#include <algorithm>

using namespace std;

// Function to generate a non-uniform random number (NURand)
int NURand(int A, int x, int y) {
    static int C = std::rand() % 1000; // C is a constant per execution
    return (((std::rand() % A) | (std::rand() % (y - x + 1) + x)) + C) % (y - x + 1) + x;
}

std::vector<int> generatePermutation(int n) {
    // Create a vector with integers from 1 to n
    std::vector<int> permutation;
    for (int i = 1; i <= n; ++i) {
        permutation.push_back(i);
    }

    // Use a random number generator
    std::random_device rd; // Obtain a random seed
    std::mt19937 gen(rd()); // Seed the generator

    // Perform Fisher-Yates shuffle
    for (int i = n - 1; i > 0; --i) {
        std::uniform_int_distribution<int> dis(0, i);
        int j = dis(gen); // Get a random index
        std::swap(permutation[i], permutation[j]);
    }

    return permutation;
}


std::string getCurrentTimestamp() {
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

// Function to generate a random string of digits with length between low and high
std::string generateRandomDigitString(int low, int high) {
    // Random number generator
    std::random_device rd;               // Seed generator
    std::mt19937 gen(rd());              // Mersenne Twister engine
    std::uniform_int_distribution<int> lengthDist(low, high); // Length between 10 and 20
    std::uniform_int_distribution<int> digitDist(0, 25);    // Digits between 0 and 9

    int length = lengthDist(gen); // Randomly choose a length between 10 and 20
    std::string randomString;

    // Generate random digits and build the string
    for (int i = 0; i < length; ++i) {
        randomString += ('a'-'\0') + digitDist(gen);
    }

    return randomString;
}

// Function to generate a random string of digits with length between low and high
std::string generateRandomNString(int low, int high) {
    // Random number generator
    std::random_device rd;               // Seed generator
    std::mt19937 gen(rd());              // Mersenne Twister engine
    std::uniform_int_distribution<int> lengthDist(low, high); // Length between 10 and 20
    std::uniform_int_distribution<int> digitDist(0, 9);    // Digits between 0 and 9

    int length = lengthDist(gen); // Randomly choose a length between 10 and 20
    std::string randomString;

    // Generate random digits and build the string
    for (int i = 0; i < length; ++i) {
        randomString += std::to_string(digitDist(gen));
    }

    return randomString;
}

bool IsBC() {
    // Random number generator
    std::random_device rd;               // Seed generator
    std::mt19937 gen(rd());              // Mersenne Twister engine
    std::uniform_int_distribution<int> digitDist(0, 9);    // Digits between 0 and 9
    return digitDist(gen) == 0;
}


// Function to generate a random string 
std::string generateRandomZipCode() {
    // Random number generator
    std::random_device rd;               // Seed generator
    std::mt19937 gen(rd());              // Mersenne Twister engine
    std::uniform_int_distribution<int> dis(0, 9999); // Random number in [0, 9999]

    int randomNumber = dis(gen);

    // Format as a 4-digit string with leading zeros if necessary
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << randomNumber << "11111";
    return oss.str();
}

// Function to generate a random double between [0, 0.2]
double generateRandomDouble(double low, double high) {

    // Generate a random integer and scale it to [0, 0.2]
    return low + static_cast<double>(std::rand()) / RAND_MAX * (high - low);
}

int generateRandomInt(int low, int high) {
    // Use a random number generator
    std::random_device rd; // Obtain a random seed
    std::mt19937 gen(rd()); // Seed the generator
    std::uniform_int_distribution<int> dis(low, high); // Define the range

    return dis(gen); // Generate a random integer between low and high
}

// Utility function to execute SQL commands
void executeSQL(sqlite3* db, const std::string& sql) {
    char* errorMessage = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errorMessage) != SQLITE_OK) {
        std::cerr << "SQL error: " << errorMessage << std::endl;
        sqlite3_free(errorMessage);
        throw std::runtime_error("Failed to execute SQL statement");
    }
}

// Function to create all necessary tables
void createTables(sqlite3* db) {
    std::cout << "Creating tables..." << std::endl;

    // Warehouse table
    executeSQL(db, R"(
        CREATE TABLE IF NOT EXISTS WAREHOUSE (
            W_ID INTEGER PRIMARY KEY,
            W_NAME TEXT,
            W_STREET_1 TEXT,
            W_STREET_2 TEXT,
            W_CITY TEXT,
            W_STATE TEXT,
            W_ZIP TEXT,
            W_TAX REAL,
            W_YTD REAL
        );
    )");

    // District table
    executeSQL(db, R"(
        CREATE TABLE IF NOT EXISTS DISTRICT (
            D_ID INTEGER,
            D_W_ID INTEGER,
            D_NAME TEXT,
            D_STREET_1 TEXT,
            D_STREET_2 TEXT,
            D_CITY TEXT,
            D_STATE TEXT,
            D_ZIP TEXT,
            D_TAX REAL,
            D_YTD REAL,
            D_NEXT_O_ID INTEGER,
            PRIMARY KEY (D_W_ID, D_ID),
            FOREIGN KEY (D_W_ID) REFERENCES WAREHOUSE(W_ID)
        );
    )");

    // Customer table
    executeSQL(db, R"(
        CREATE TABLE IF NOT EXISTS CUSTOMER (
            C_ID INTEGER,
            C_D_ID INTEGER,
            C_W_ID INTEGER,
            C_FIRST TEXT,
            C_MIDDLE TEXT,
            C_LAST TEXT,
            C_STREET_1 TEXT,
            C_STREET_2 TEXT,
            C_CITY TEXT,
            C_STATE TEXT,
            C_ZIP TEXT,
            C_PHONE TEXT,
            C_SINCE TEXT,
            C_CREDIT TEXT,
            C_CREDIT_LIM REAL,
            C_DISCOUNT REAL,
            C_BALANCE REAL,
            C_YTD_PAYMENT REAL,
            C_PAYMENT_CNT INTEGER,
            C_DELIVERY_CNT INTEGER,
            C_DATA TEXT,
            PRIMARY KEY (C_W_ID, C_D_ID, C_ID),
            FOREIGN KEY (C_W_ID, C_D_ID) REFERENCES DISTRICT(D_W_ID, D_ID)
        );
    )");

    // Order table
    executeSQL(db, R"(
        CREATE TABLE IF NOT EXISTS ORDERS (
            O_ID INTEGER,
            O_D_ID INTEGER,
            O_W_ID INTEGER,
            O_C_ID INTEGER,
            O_ENTRY_D TEXT,
            O_CARRIER_ID INTEGER,
            O_OL_CNT INTEGER,
            O_ALL_LOCAL INTEGER,
            PRIMARY KEY (O_W_ID, O_D_ID, O_ID),
            FOREIGN KEY (O_W_ID, O_D_ID, O_C_ID) REFERENCES CUSTOMER(C_W_ID, C_D_ID, C_ID)
        );
    )");

    // NewOrder table
    executeSQL(db, R"(
        CREATE TABLE IF NOT EXISTS NEW_ORDER (
            NO_O_ID INT,                             
            NO_D_ID INT,                            
            NO_W_ID INT,                            
            PRIMARY KEY (NO_W_ID, NO_D_ID, NO_O_ID), 
            FOREIGN KEY (NO_W_ID, NO_D_ID, NO_O_ID) REFERENCES ORDERS (O_W_ID, O_D_ID, O_ID) 
        );
    )");


    // orderline table
    executeSQL(db, R"(
        CREATE TABLE IF NOT EXISTS ORDER_LINE (
            OL_O_ID INTEGER,
            OL_D_ID INTEGER,
            OL_W_ID INTEGER,
            OL_NUMBER INTEGER,
            OL_I_ID INTEGER,
            OL_SUPPLY_W_ID INTEGER,
            OL_DELIVERY_D DATETIME,
            OL_QUANTITY NUMERIC(2),
            OL_AMOUNT NUMERIC(6, 2),
            OL_DIST_INFO CHAR(24),
            PRIMARY KEY (OL_W_ID, OL_D_ID, OL_O_ID, OL_NUMBER),
            FOREIGN KEY (OL_W_ID, OL_D_ID, OL_O_ID) REFERENCES ORDERS (O_W_ID, O_D_ID, O_ID),
            FOREIGN KEY (OL_SUPPLY_W_ID, OL_I_ID) REFERENCES STOCK (S_W_ID, S_I_ID)
        );
    )");

    // Stock table
    executeSQL(db, R"(
        CREATE TABLE IF NOT EXISTS STOCK (
            S_I_ID INTEGER,
            S_W_ID INTEGER,
            S_QUANTITY INTEGER,
            S_DIST_01 TEXT,
            S_DIST_02 TEXT,
            S_DIST_03 TEXT,
            S_DIST_04 TEXT,
            S_DIST_05 TEXT,
            S_DIST_06 TEXT,
            S_DIST_07 TEXT,
            S_DIST_08 TEXT,
            S_DIST_09 TEXT,
            S_DIST_10 TEXT,
            S_YTD INTEGER,
            S_ORDER_CNT INTEGER,
            S_REMOTE_CNT INTEGER,
            S_DATA TEXT,
            PRIMARY KEY (S_W_ID, S_I_ID),
            FOREIGN KEY (S_W_ID) REFERENCES WAREHOUSE(W_ID)
        );
    )");

    // Stock table
    executeSQL(db, R"(
        CREATE TABLE IF NOT EXISTS ITEM (
            I_ID INT,                             
            I_IM_ID INT,                   
            I_NAME VARCHAR(24),              
            I_PRICE DECIMAL(5, 2),          
            I_DATA VARCHAR(50),                  
            PRIMARY KEY (I_ID)             
        );
    )");

    // History table
    executeSQL(db, R"(
        CREATE TABLE IF NOT EXISTS HISTORY (
            H_C_ID INT NOT NULL,
            H_C_D_ID INT NOT NULL,
            H_C_W_ID INT NOT NULL,
            H_D_ID INT NOT NULL,
            H_W_ID INT NOT NULL,
            H_DATE DATETIME NOT NULL,
            H_AMOUNT NUMERIC(6, 2) NOT NULL,
            H_DATA VARCHAR(24),
            FOREIGN KEY (H_C_W_ID, H_C_D_ID, H_C_ID) REFERENCES CUSTOMER (C_W_ID, C_D_ID, C_ID),
            FOREIGN KEY (H_W_ID, H_D_ID) REFERENCES DISTRICT (D_W_ID, D_ID)
        );
    )");


    std::cout << "Tables created successfully." << std::endl;
}

// Function to initialize warehouse data
void initializeWarehouses(sqlite3* db, int warehouseCount) {
    std::cout << "Initializing warehouse data..." << std::endl;
    for (int w = 1; w <= warehouseCount; ++w) {
        std::string w_name = generateRandomDigitString(6,10);
        std::string w_st_1 = generateRandomDigitString(10,20);
        std::string w_st_2 = generateRandomDigitString(10,20);
        std::string w_city = generateRandomDigitString(10,20);
        std::string w_state = generateRandomDigitString(2,2);
        std::string w_zip = generateRandomZipCode();
        double w_tax = generateRandomDouble(0, 0.2);
        std::string sql = "INSERT INTO WAREHOUSE (W_ID, W_NAME, W_STREET_1, W_STREET_2, W_CITY, W_STATE, W_ZIP, W_TAX, W_YTD) "
                          "VALUES (" +
                          std::to_string(w) + ", '" + w_name + "', '" + w_st_1 + "', '" + w_st_2 + "', '" + w_city + "', '" + w_state + "', '" + w_zip + "', " +
                          std::to_string(w_tax) + ", 300000.0);";

        // std::cout << sql << std::endl;
        executeSQL(db, sql);
    }
    std::cout << "Warehouse data initialized." << std::endl;
}

// Function to initialize district data
void initializeDistricts(sqlite3* db, int warehouseCount, int districtCount) {
    std::cout << "Initializing district data..." << std::endl;
    for (int w = 1; w <= warehouseCount; ++w) {
        for (int d = 1; d <= districtCount; ++d) {
            std::string d_name = generateRandomDigitString(6,10);
            std::string d_st_1 = generateRandomDigitString(10,20);
            std::string d_st_2 = generateRandomDigitString(10,20);
            std::string d_city = generateRandomDigitString(10,20);
            std::string d_state = generateRandomDigitString(2,2);
            std::string d_zip = generateRandomZipCode();
            double d_tax = generateRandomDouble(0, 0.2);
            std::string sql = "INSERT INTO DISTRICT (D_ID, D_W_ID, D_NAME, D_STREET_1, D_STREET_2, D_CITY, D_STATE, D_ZIP, D_TAX, D_YTD, D_NEXT_O_ID) "
                          "VALUES (" +
                          std::to_string(d) + ", " + std::to_string(w) + ", '" + d_name + "', '" + d_st_1 + "', '" + d_st_2 + "', '" + d_city + "', '" + d_state + "', '" + d_zip + "', " +
                          std::to_string(d_tax) + ", 300000.0, 3001);";
            executeSQL(db, sql);
            // std::cout << sql << std::endl;
        }
    }
    std::cout << "District data initialized." << std::endl;
}

// Function to generate C_LAST based on a number between 0 and 999
std::string generateCustomerLastName(int number) {
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


// Function to initialize customer data
void initializeCustomers(sqlite3* db, int warehouseCount, int districtCount, int customersPerDistrict) {
    std::cout << "Initializing customer data..." << std::endl;
    for (int w = 1; w <= warehouseCount; ++w) {
        for (int d = 1; d <= districtCount; ++d) {
            for (int c = 1; c <= customersPerDistrict; ++c) {
                std::cout << d << " " << c << std::endl;
                int c_id = c;
                std::string c_last_name = generateCustomerLastName(c);
                std::string c_fisrt_name = generateRandomDigitString(8,16);
                std::string c_st_1 = generateRandomDigitString(10,20);
                std::string c_st_2 = generateRandomDigitString(10,20);
                std::string c_city = generateRandomDigitString(10,20);
                std::string c_state = generateRandomDigitString(2,2);
                std::string c_phone = generateRandomNString(16, 16);
                std::string c_zip = generateRandomZipCode();
                std::string c_credit = IsBC() ? "BC" : "GC";
                double c_discount = generateRandomDouble(0, 0.5);
                std::string c_data = generateRandomDigitString(300, 500);
                std::string date_time = getCurrentTimestamp();

                std::string sql = "INSERT INTO CUSTOMER (C_ID, C_D_ID, C_W_ID, C_FIRST, C_MIDDLE, C_LAST, C_STREET_1, C_STREET_2, C_CITY, C_STATE, C_ZIP, C_PHONE, C_SINCE, C_CREDIT, C_CREDIT_LIM, C_DISCOUNT, C_BALANCE, C_YTD_PAYMENT, C_PAYMENT_CNT, C_DELIVERY_CNT, C_DATA) "
                                  "VALUES (" +
                                  std::to_string(c_id) + ", " +
                                  std::to_string(d) + ", " +
                                  std::to_string(w) + ", '" + c_fisrt_name + "', 'OE', '" + c_last_name + "', '" + c_st_1 + "', '" + c_st_2 + "', '" + c_city + "', '" + c_state + "', '" + c_zip + "', '" + c_phone + "', '" + date_time + "', '" + c_credit + "', 50000.00, " + std::to_string(c_discount) + ", -10.0, 10.0, 1, 0, '" + c_data + "');";
                // std::cout << sql << std::endl;
                executeSQL(db, sql);
                
                
                std::string h_data = generateRandomDigitString(12,24);
                std::string sql2 = "INSERT INTO HISTORY (H_C_ID, H_C_D_ID, H_C_W_ID, H_D_ID, H_W_ID, H_DATE, H_AMOUNT, H_DATA) "
                                  "VALUES (" + std::to_string(c_id) + ", " + std::to_string(d) + ", " + std::to_string(w) + ", " + std::to_string(d) + ", " + std::to_string(w) + ", '" + date_time + "', 10.0 , '" + h_data + "');";
                // std::cout << sql2 << std::endl;
                executeSQL(db, sql2);
            }
        }
    }
    std::cout << "Customer data initialized." << std::endl;
}

// Function to initialize order, order-line, neworder data
void initializeOrders(sqlite3* db, int warehouseCount, int districtCount, int customersPerDistrict) {
    std::cout << "Initializing order data..." << std::endl;
    for (int w = 1; w <= warehouseCount; ++w) {
        for (int d = 1; d <= districtCount; ++d) {
            std::vector<int> permutation = generatePermutation(3000);
            for (int c = 1; c <= customersPerDistrict; ++c) {
                std::cout << d << " " << c << std::endl;
                int o_c_id = permutation[c-1];
                int o_ol_cnt = generateRandomInt(5, 15);
                std::string o_entry_d = getCurrentTimestamp();
                std::string o_carrier_id = "null";
                if (c < 2101) {
                    o_carrier_id = std::to_string(generateRandomInt(1, 10));
                }
                int o_all_local = 1;

                std::string order_sql = "INSERT INTO ORDERS (O_ID, O_C_ID, O_D_ID, O_W_ID, O_ENTRY_D, O_CARRIER_ID, O_OL_CNT, O_ALL_LOCAL) "
                                  "VALUES (" +
                                  std::to_string(c) + ", " + std::to_string(o_c_id) + ", " +
                                  std::to_string(d) + ", " +
                                  std::to_string(w) + ", '" + o_entry_d + "', " + o_carrier_id + ", " + std::to_string(o_ol_cnt) + ", " + std::to_string(o_all_local) + ");";
                // std::cout << order_sql << std::endl;
                executeSQL(db, order_sql);
                // 
                
                for (int ol = 1; ol <= o_ol_cnt; ++ol) {
                    std::string ol_delivery_d = getCurrentTimestamp();
                    double ol_amount = 0;
                    int ol_i_id = generateRandomInt(1, 100000);
                    std::string ol_dist_info = generateRandomDigitString(24, 24);
                    if (c >= 2101) {
                        ol_amount = generateRandomDouble(0.01, 9999.99);
                        ol_delivery_d = "null";
                    }
                    std::string ol_sql = "INSERT INTO ORDER_LINE (OL_O_ID, OL_D_ID, OL_W_ID, OL_NUMBER, OL_I_ID, OL_SUPPLY_W_ID, OL_DELIVERY_D, OL_QUANTITY, OL_AMOUNT, OL_DIST_INFO) "
                                  "VALUES (" +
                                  std::to_string(c) + ", " + std::to_string(d) + ", " +
                                  std::to_string(w) + ", " + std::to_string(ol) + ", " + std::to_string(ol_i_id) + ", " + std::to_string(w) 
                                  + ", '" + ol_delivery_d + "', 5, " + std::to_string(ol_amount) + ", '" + ol_dist_info + "');";
                    // std::cout << ol_sql << std::endl;
                    executeSQL(db, ol_sql);
                }   
            }

            for (int no = 2101; no <= 3000; ++no) {
                    std::string no_sql = "INSERT INTO NEW_ORDER (NO_O_ID, NO_D_ID, NO_W_ID) "
                                  "VALUES (" +
                                  std::to_string(no) + ", " + std::to_string(d) + ", " +
                                  std::to_string(w) + ");";
                    // std::cout << no_sql << std::endl;
                    executeSQL(db, no_sql);
            }
        }
    }
    std::cout << "Order data initialized." << std::endl;
}


// Function to initialize stock data
void initializeStock(sqlite3* db, int warehouseCount, int itemCount) {
    std::cout << "Initializing stock data..." << std::endl;
    for (int w = 1; w <= warehouseCount; ++w) {
        for (int i = 1; i <= itemCount; ++i) {
            if (i % 1000 == 1) {
                std::cout << i << std::endl;
            }
            double s_quantity = generateRandomDouble(10, 100); 
            std::string s_dist[11];
            for (int k = 1; k <= 10; k++) {
                s_dist[k] = generateRandomDigitString(24, 24);
            }
            std::string s_data = generateRandomDigitString(26, 50);
            if (generateRandomInt(0, 9) == 1) {
                int pos = generateRandomInt(0, s_data.length() - 8);
                s_data.replace(pos, 8, "ORIGINAL");
            }
            std::string sql = "INSERT INTO STOCK (S_I_ID, S_W_ID, S_QUANTITY, S_DIST_01, S_DIST_02, S_DIST_03, S_DIST_04, S_DIST_05, S_DIST_06, S_DIST_07, S_DIST_08, S_DIST_09, S_DIST_10, S_YTD, S_ORDER_CNT, S_REMOTE_CNT, S_DATA) "
                              "VALUES (" +
                              std::to_string(i) + ", " +
                              std::to_string(w) + ",  " +
                              std::to_string(s_quantity) + ", '" + s_dist[1] + "', '" + s_dist[2] + "', '" + s_dist[3] + "', '" + s_dist[4] + "', '" + s_dist[5] + "', '" + s_dist[6] + "', '" + s_dist[7] + "', '" + s_dist[8] + "', '" + s_dist[9] + "', '" + s_dist[10] + "', 0, 0, 0, '" + s_data + "');"; 
            executeSQL(db, sql);
            // std::cout << sql << std::endl;
        }
    }
    std::cout << "Stock data initialized." << std::endl;
}


// Function to initialize stock data
void initializeItems(sqlite3* db, int warehouseCount, int itemCount) {
    std::cout << "Initializing item data..." << std::endl;
    for (int w = 1; w <= warehouseCount; ++w) {
        for (int i = 1; i <= itemCount; ++i) {
            if (i % 1000 == 0) {
                std::cout << i << std::endl;
            }
            
            int i_im_id = NURand(255, 0, 10000); 
            std::string i_name = generateRandomDigitString(14, 24);
            double i_price = generateRandomDouble(1.0, 100.0);
            std::string i_data = generateRandomDigitString(26, 50);
            if (std::rand() % 127 % 10 == 1) {
                int pos = std::rand() % (i_data.length() - 8);
                i_data.replace(pos, 8, "ORIGINAL");
            }
            std::string sql = "INSERT INTO ITEM (I_ID, I_IM_ID, I_NAME, I_PRICE, I_DATA) "
                              "VALUES (" +
                              std::to_string(i) + ", " +
                              std::to_string(i_im_id) + ", '" + i_name + "', " + std::to_string(i_price) + ", '"  + i_data + "');"; 
            executeSQL(db, sql);
            // std::cout << sql << std::endl;
        }
    }
    std::cout << "Item data initialized." << std::endl;
}

int main() {

    // Initialize random seed
    std::srand(std::time(nullptr)); // Seed with current time

    sqlite3* db;
    const char* dbName = "tpcc_10x_small.db";

    // Open the database
    if (sqlite3_open(dbName, &db)) {
        std::cerr << "Error opening database: " << sqlite3_errmsg(db) << std::endl;
        return -1;
    }

    try {
        // Create tables
        createTables(db);

        // Initialize data
        int warehouseCount = 1;
        int districtCount = 10;
        int customersPerDistrict = 300;
        int itemCount = 10000;

        initializeWarehouses(db, warehouseCount);
        initializeItems(db, warehouseCount, itemCount);
        initializeDistricts(db, warehouseCount, districtCount);
        initializeCustomers(db, warehouseCount, districtCount, customersPerDistrict);
        initializeOrders(db, warehouseCount, districtCount, customersPerDistrict);
        initializeStock(db, warehouseCount, itemCount);
        

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    // Close the database
    sqlite3_close(db);
    return 0;
}


