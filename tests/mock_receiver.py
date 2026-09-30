"""Opt-in LAN fixture for device tests; never proxies real receiver data.

Run on an unused port 8080 with --bind set to your test computer's LAN IP.
Point the ESP station setting at that IP temporarily, then restore it afterwards.
Write live, slow, frozen, malformed or unavailable to --mode-file to change responses.
Temperature intentionally unavailable (port 80 is not opened).
"""
import argparse
import json
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--bind', required=True)
parser.add_argument('--mode-file', required=True, type=Path)
args = parser.parse_args()
started = time.time()
frozen = started

class Handler(BaseHTTPRequestHandler):
    def log_message(self, *args):
        pass

    def do_GET(self):
        mode = args.mode_file.read_text().strip()
        if mode == 'unavailable':
            self.send_error(503)
            return
        if self.path == '/data/aircraft.json':
            payload = {'now': frozen if mode == 'frozen' else time.time(),
                       'messages': 0, 'aircraft': []}
        elif self.path == '/data/stats.json':
            payload = {'now': time.time(), 'total': {'local': {'accepted': [0, 0]}}}
        else:
            self.send_error(404)
            return
        if mode == 'slow':
            payload['padding'] = 'x' * 48000
        data = b'{"now":' if mode == 'malformed' else json.dumps(payload).encode()
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(data)))
        self.end_headers()
        try:
            if mode == 'slow':
                for at in range(0, len(data), 4096):
                    self.wfile.write(data[at:at+4096])
                    self.wfile.flush()
                    time.sleep(0.2)
            else:
                self.wfile.write(data)
        except (BrokenPipeError, ConnectionResetError):
            pass

print('Fixture listening on the requested LAN interface, port 8080', flush=True)
ThreadingHTTPServer((args.bind, 8080), Handler).serve_forever()
