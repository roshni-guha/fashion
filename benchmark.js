import http from 'k6/http';
import { check, sleep } from 'k6';

export const options = {
  stages: [
    { duration: '10s', target: 50 },  // Ramp up to 50 concurrent users
    { duration: '30s', target: 200 }, // Spike to 200 concurrent users
    { duration: '10s', target: 0 },   // Ramp down
  ],
};

export default function () {
  // Simulate users requesting different Deep-Zoom tiles
  const zoomLevel = 12;
  const x = Math.floor(Math.random() * 10);
  const y = Math.floor(Math.random() * 10);
  
  const res = http.get(`http://localhost:8080/tiles/${zoomLevel}/${x}_${y}.png`);
  
  check(res, {
    'is status 200': (r) => r.status === 200,
    'is cache hit': (r) => r.headers['X-Cache'] === 'HIT',
  });
  
  sleep(0.1); // Small delay between requests per user
}