'use client';
import { useEffect, useRef } from 'react';
import OpenSeadragon from 'openseadragon';

export default function ArchivalViewer() {
  const viewerRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (!viewerRef.current) return;

    const viewer = OpenSeadragon({
      element: viewerRef.current,
      prefixUrl: '//openseadragon.github.io/openseadragon/images/',
      tileSources: {
        Image: {
          xmlns: 'http://schemas.microsoft.com/deepzoom/2008',
          Url: 'http://localhost:8080/tiles/',
          Format: 'png',
          Overlap: '1',
          TileSize: '256',
          Size: {
            // From ingest/dataset/fashion_item.dzi (House of Worth ball gown, Met 156027)
            Width: '2956',
            Height: '4000'
          }
        }
      }
    });

    return () => viewer.destroy();
  }, []);

  return (
    <div className="w-full h-[600px] border border-gray-700 rounded-lg overflow-hidden bg-black shadow-2xl">
      <div ref={viewerRef} className="w-full h-full" />
    </div>
  );
}