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
*
*/

import { ThemeProvider } from "@/components/theme-provider";
import { BrowserRouter, Routes, Route } from "react-router-dom";
import { Dashboard } from "@/components/dashboard/dashboard";
import { HomePage } from "@/components/home-page/homePage";
import PBFTPage from "@/components/home-page/PBFTFullView"; // Import the new PBFT page
import AuthorsPage from "./components/Authors/AuthorPage";
import { Toaster } from "./components/ui/toaster";

function App() {
  return (
    <ThemeProvider defaultTheme="dark" storageKey="vite-ui-theme">
      <Toaster />
      <BrowserRouter>
        <Routes>
          <Route path="/" element={<HomePage />} />
          <Route path="/dashboard" element={<Dashboard />} />
          <Route path="/pbft" element={<PBFTPage />} /> {/* Add this */}
          <Route path="/authors" element={<AuthorsPage />} />
        </Routes>
      </BrowserRouter>
    </ThemeProvider>
  );
}

export default App;
