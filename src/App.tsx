import { ThemeProvider } from "@/components/theme-provider";
import { BrowserRouter, Routes, Route } from "react-router-dom";
import { Dashboard } from "@/components/dashboard/dashboard";
import { HomePage } from "@/components/home-page/homePage";
import PBFTPage from "@/components/home-page/PBFT"; // Import the new PBFT page

function App() {
  return (
    <ThemeProvider defaultTheme="dark" storageKey="vite-ui-theme">
      <BrowserRouter>
        <Routes>
          <Route path="/" element={<HomePage />} />
          <Route path="/dashboard" element={<Dashboard />} />
          <Route path="/pbft" element={<PBFTPage />} /> {/* Add this */}
        </Routes>
      </BrowserRouter>
    </ThemeProvider>
  );
}

export default App;
