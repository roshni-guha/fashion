import ArchivalViewer from '@/app/components/ArchivalViewer';

export default function Home() {
  return (
    <main className="min-h-screen bg-neutral-900 text-white p-8">
      <header className="mb-10">
        <h1 className="text-3xl font-bold tracking-tight">Dual-Engine Fashion Intelligence</h1>
        <p className="text-gray-400 mt-2">C++ Consistent Hash Ring (Storage) & Go WebRTC (Live Stream)</p>
      </header>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-8">
        
        {/* Engine 1: C++ Lossless Archival Vault */}
        <section className="flex flex-col gap-4">
          <div className="flex justify-between items-center">
            <h2 className="text-xl font-semibold">1. Archival Vault (Deep-Zoom)</h2>
            <span className="bg-blue-900 text-blue-300 text-xs px-2 py-1 rounded">C++20 Edge Router :8080</span>
          </div>
          <ArchivalViewer />
        </section>

        {/* Engine 2: Go WebRTC Fanout */}
        <section className="flex flex-col gap-4">
          <div className="flex justify-between items-center">
            <h2 className="text-xl font-semibold">2. Live Runway Telemetry</h2>
            <span className="bg-green-900 text-green-300 text-xs px-2 py-1 rounded">Go WebRTC Broker :8090</span>
          </div>
          {/* Embed the Go server's raw UI via iframe for immediate integration */}
          <div className="w-full h-[600px] border border-gray-700 rounded-lg overflow-hidden shadow-2xl bg-black">
            <iframe src="http://localhost:8090" className="w-full h-full border-none" />
          </div>
        </section>
        
      </div>
    </main>
  );
}