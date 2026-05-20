from http.server import HTTPServer, BaseHTTPRequestHandler
import subprocess, json, os, sys

PROJ = os.path.dirname(os.path.abspath(__file__))
MMU  = os.path.join(PROJ, 'mmu')

class Handler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        print(f'  {self.address_string()} {fmt % args}')

    def send_cors(self):
        self.send_header('Access-Control-Allow-Origin',  '*')
        self.send_header('Access-Control-Allow-Methods', 'POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')

    def do_OPTIONS(self):
        self.send_response(204)
        self.send_cors()
        self.end_headers()

    def do_POST(self):
        if self.path != '/run':
            self.send_response(404); self.end_headers(); return

        body   = json.loads(self.rfile.read(int(self.headers['Content-Length'])))
        inp    = body.get('input',  'in1')
        frames = int(body.get('frames', 16))
        algo   = body.get('algo',   'f')

        input_file = os.path.join(PROJ, 'lab3_assign', inp)
        rfile      = os.path.join(PROJ, 'lab3_assign', 'rfile')

        if not os.path.isfile(MMU):
            result = {'ok': False, 'stdout': '', 'stderr': './mmu binary not found. Run: make'}
        elif not os.path.isfile(input_file):
            result = {'ok': False, 'stdout': '', 'stderr': f'Input file not found: {input_file}'}
        else:
            try:
                r = subprocess.run(
                    [MMU, f'-f{frames}', f'-a{algo}', '-oOPFS', input_file, rfile],
                    capture_output=True, text=True, timeout=15, cwd=PROJ
                )
                result = {'ok': r.returncode == 0, 'stdout': r.stdout, 'stderr': r.stderr}
            except subprocess.TimeoutExpired:
                result = {'ok': False, 'stdout': '', 'stderr': 'mmu timed out'}
            except Exception as e:
                result = {'ok': False, 'stdout': '', 'stderr': str(e)}

        payload = json.dumps(result).encode()
        self.send_response(200)
        self.send_header('Content-Type',   'application/json')
        self.send_header('Content-Length', str(len(payload)))
        self.send_cors()
        self.end_headers()
        self.wfile.write(payload)

PORT = 8765
print(f'MMU server listening on http://localhost:{PORT}')
print(f'Project dir : {PROJ}')
print(f'Binary      : {MMU} {"(found)" if os.path.isfile(MMU) else "(NOT FOUND — run: make)"}')
print()
try:
    HTTPServer(('localhost', PORT), Handler).serve_forever()
except KeyboardInterrupt:
    print('\nServer stopped.')
    sys.exit(0)
