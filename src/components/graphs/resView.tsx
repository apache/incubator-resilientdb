import { Cuboid } from "lucide-react";
import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from "../ui/card";

export function ResView() {
  return (
    <Card className="w-4/5 max-w-8xl mx-auto bg-gradient-to-br from-slate-900 to-slate-800 text-white shadow-xl">
      <CardHeader className="border-b border-slate-700">
        <div className="flex flex-col sm:flex-row items-center justify-between gap-4">
          <div className="flex items-center gap-2">
            <Cuboid className="w-6 h-6 text-blue-400" />
            <CardTitle className="text-2xl font-bold">Protocol View</CardTitle>
          </div>
        </div>
        <CardDescription className="mt-4 text-slate-300">
          Get Information on ResDB's internal blockchain data
        </CardDescription>
      </CardHeader>
      <CardContent className="p-0">
        <div className="w-full h-screen overflow-auto">
          <iframe
            src="http://64.23.201.175:3000/pages/visualizer"
            width="100%"
            height="100%"
            style={{ border: "none" }}
            title="Visualizer"
          ></iframe>
        </div>
      </CardContent>
    </Card>
  );
}
