#include <random>
#include <string>

#include "crow.h"
//#include "crow_all.h"

int main() {
  crow::SimpleApp app;  // define your crow application

  // define your endpoint at the root directory
  CROW_ROUTE(app, "/")
  ([]() { return "Welcome to the home page"; });

  CROW_ROUTE(app, "/random")
  ([]() { return std::to_string(std::rand() % 100); });

  CROW_ROUTE(app, "/add/<int>/<int>")
  ([](int a, int b) { return std::to_string(a + b); });

  CROW_ROUTE(app, "/add")
  ([]() { return "No parameters supplied"; });

  // set the port, set the app to run on multiple threads, and run the app
  app.port(8000).multithreaded().run();
}
