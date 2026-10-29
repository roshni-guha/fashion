package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os"
	"strconv"
	"sync"
	"time"

	"github.com/pion/ice/v4"
	"github.com/pion/webrtc/v4"
)

// Broker manages the WebRTC DataChannels for all connected viewers
type Broker struct {
	clients map[*webrtc.DataChannel]struct{}
	mutex   sync.RWMutex
}

func NewBroker() *Broker {
	return &Broker{
		clients: make(map[*webrtc.DataChannel]struct{}),
	}
}

func (b *Broker) AddClient(dc *webrtc.DataChannel) {
	b.mutex.Lock()
	defer b.mutex.Unlock()
	b.clients[dc] = struct{}{}
	fmt.Printf("[Broker] Client added. Total clients: %d\n", len(b.clients))
}

func (b *Broker) RemoveClient(dc *webrtc.DataChannel) {
	b.mutex.Lock()
	defer b.mutex.Unlock()
	delete(b.clients, dc)
	fmt.Printf("[Broker] Client disconnected. Total clients: %d\n", len(b.clients))
}

// Broadcast fans out the video frame payload to all active WebRTC clients
func (b *Broker) Broadcast(payload []byte) {
	b.mutex.RLock()
	defer b.mutex.RUnlock()

	for dc := range b.clients {
		if dc.ReadyState() == webrtc.DataChannelStateOpen {
			err := dc.SendText(string(payload))
			if err != nil {
				log.Println("Error sending to client:", err)
			}
		}
	}
}

// newWebRTCAPI configures how ICE candidates are advertised. Inside Docker the container IP
// isn't reachable from the host browser, so set WEBRTC_UDP_PORT (published as UDP) and
// WEBRTC_PUBLIC_IP (e.g. 127.0.0.1) to route all WebRTC traffic through one known port.
func newWebRTCAPI() *webrtc.API {
	var s webrtc.SettingEngine

	if portStr := os.Getenv("WEBRTC_UDP_PORT"); portStr != "" {
		port, err := strconv.Atoi(portStr)
		if err != nil {
			log.Fatalf("invalid WEBRTC_UDP_PORT %q: %v", portStr, err)
		}
		mux, err := ice.NewMultiUDPMuxFromPort(port)
		if err != nil {
			log.Fatalf("listen on UDP %d: %v", port, err)
		}
		s.SetICEUDPMux(mux)
		s.SetNetworkTypes([]webrtc.NetworkType{webrtc.NetworkTypeUDP4})
	}
	if ip := os.Getenv("WEBRTC_PUBLIC_IP"); ip != "" {
		// Advertise the public IP in place of the container's private one
		if err := s.SetICEAddressRewriteRules(webrtc.ICEAddressRewriteRule{
			External:        []string{ip},
			AsCandidateType: webrtc.ICECandidateTypeHost,
			Mode:            webrtc.ICEAddressRewriteReplace,
		}); err != nil {
			log.Fatalf("invalid WEBRTC_PUBLIC_IP %q: %v", ip, err)
		}
	}

	return webrtc.NewAPI(webrtc.WithSettingEngine(s))
}

func main() {
	broker := NewBroker()
	api := newWebRTCAPI()

	// 1. Simulate the Ingestion Engine (60 FPS Camera Feed)
	go func() {
		frameID := 0
		ticker := time.NewTicker(time.Second / 60) // 60 FPS
		defer ticker.Stop()

		for range ticker.C {
			frameID++
			// In production, this payload is the H.264/VP8 byte array from C++
			payload := []byte(fmt.Sprintf(`{"frame_id": %d, "timestamp": %d, "bytes": 1024}`, frameID, time.Now().UnixMilli()))
			broker.Broadcast(payload)
		}
	}()

	// 2. WebRTC Signaling Endpoint
	http.HandleFunc("/sdp", func(w http.ResponseWriter, r *http.Request) {
		// CORS headers for local testing
		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type")
		if r.Method == http.MethodOptions {
			return
		}
		if r.Method != http.MethodPost {
			http.Error(w, "POST an SDP offer", http.StatusMethodNotAllowed)
			return
		}

		var offer webrtc.SessionDescription
		if err := json.NewDecoder(r.Body).Decode(&offer); err != nil {
			http.Error(w, err.Error(), http.StatusBadRequest)
			return
		}

		// Initialize WebRTC Peer Connection
		peerConnection, err := api.NewPeerConnection(webrtc.Configuration{
			ICEServers: []webrtc.ICEServer{{URLs: []string{"stun:stun.l.google.com:19302"}}},
		})
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}

		// Tear down the connection on any signaling error so sockets and goroutines aren't leaked
		ok := false
		defer func() {
			if !ok {
				peerConnection.Close()
			}
		}()

		// Release resources once the viewer goes away (tab closed, network dropped)
		peerConnection.OnConnectionStateChange(func(s webrtc.PeerConnectionState) {
			if s == webrtc.PeerConnectionStateFailed {
				peerConnection.Close()
			}
		})

		// Listen for the DataChannel created by the browser
		peerConnection.OnDataChannel(func(d *webrtc.DataChannel) {
			d.OnOpen(func() {
				broker.AddClient(d)
			})
			d.OnClose(func() {
				broker.RemoveClient(d)
			})
		})

		// Set the remote description (the browser's offer)
		if err = peerConnection.SetRemoteDescription(offer); err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}

		// Create answer
		answer, err := peerConnection.CreateAnswer(nil)
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		if err = peerConnection.SetLocalDescription(answer); err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}

		// Wait for ICE gathering to complete before sending the answer back
		gatherComplete := webrtc.GatheringCompletePromise(peerConnection)
		select {
		case <-gatherComplete:
		case <-r.Context().Done():
			return // browser gave up waiting
		}

		ok = true
		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(peerConnection.LocalDescription())
	})

	// Serve the frontend UI
	http.Handle("/", http.FileServer(http.Dir(".")))

	fmt.Println("Live Fashion Week Broker running on http://localhost:8090")
	log.Fatal(http.ListenAndServe(":8090", nil))
}
