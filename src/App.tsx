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
