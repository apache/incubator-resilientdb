export default function PlaceholderPage() {
  return (
    <div className="flex flex-col justify-center items-center flex-1 h-screen">


      {/* Main Content for Placeholder Page */}
        <div className="text-center">
          <h2 className="text-4xl font-bold mb-4">Welcome to the Placeholder Page!</h2>
          <p className="text-lg text-gray-400">
            Placeholder for future content.
          </p>
        </div>

      {/* Footer for Placeholder Page (Optional) */}
      <footer className=" p-4 text-center text-sm text-gray-500 sticky bottom-0 z-10">
        <p>Placeholder Page Footer</p>
      </footer>
    </div>
  );
} 