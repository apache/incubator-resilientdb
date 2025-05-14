export default function PlaceholderPage() {
  return (
    <div className="flex flex-col flex-1 h-full">
      {/* Header for Placeholder Page - Rendered by the page itself */}
      <header className=" shadow-md p-4 sticky top-0 z-10">
        <h1 className="text-2xl font-semibold text-center bg-gradient-to-r from-blue-400 to-purple-500 text-transparent bg-clip-text">
          Placeholder Page
        </h1>
      </header>

      {/* Main Content for Placeholder Page */}
      <main className="flex-grow flex items-center justify-center p-6  bg-opacity-50">
        <div className="text-center">
          <h2 className="text-4xl font-bold mb-4">Welcome to the Placeholder Page!</h2>
          <p className="text-lg text-gray-400">
            This page is correctly using the shared layout. Its content is rendered within the main content area defined in <code>RootLayout.tsx</code>.
          </p>
        </div>
      </main>

      {/* Footer for Placeholder Page (Optional) */}
      <footer className=" p-4 text-center text-sm text-gray-500 sticky bottom-0 z-10">
        <p>Placeholder Page Footer</p>
      </footer>
    </div>
  );
} 