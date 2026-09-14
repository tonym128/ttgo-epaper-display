#!/usr/bin/env python3
import http.server
import socketserver
import webbrowser
import os
import sys

PORT = 8000
WEB_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "web")
os.chdir(WEB_DIR)

class LocalStudioHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # Prevent caching for live editing
        self.send_header('Cache-Control', 'no-cache, no-store, must-revalidate')
        self.send_header('Pragma', 'no-cache')
        self.send_header('Expires', '0')
        super().end_headers()

def main():
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("127.0.0.1", PORT), LocalStudioHandler) as httpd:
        url = f"http://localhost:{PORT}"
        print("=" * 60)
        print("  Smart E-Paper Badge Web Bluetooth Studio")
        print(f"  Serving at: {url}")
        print("  Press Ctrl+C to stop the server")
        print("=" * 60)
        
        # Try to open in user browser
        try:
            webbrowser.open(url)
        except Exception:
            pass
        
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nServer stopped.")

if __name__ == "__main__":
    main()
