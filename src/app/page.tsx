
import Dashboard from "@/app/dashboard/page";
import Head from "next/head";

export default function Home() {
  return (
    <>
    <Head>
      <title> MemView </title> 
      <meta name = "Memview" content = "dashboard"/>
      <meta name = "viewport" content = "width=device-width, initial-scale=1"/>
    </Head>
    <main>
      <Dashboard />
    </main>
    </>
  );
}
