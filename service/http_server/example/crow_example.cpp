/*
 * Copyright (c) 2019-2022 ExpoLab, UC Davis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

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
