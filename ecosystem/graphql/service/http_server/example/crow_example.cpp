/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <random>
#include <string>

#include "crow.h"
//#include "crow_all.h"

int main() {
  crow::SimpleApp app; // define your crow application

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
