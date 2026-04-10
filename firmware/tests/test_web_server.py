"""
Verify the www and ws server.

    pytest tests/test_web_server.py --host 192.168.88.117
"""

import time
import urllib.request
import urllib.error

WEB_MAX_OPEN_SOCKETS = 2

def test_index_page(host):
    """GET / four times in succession — checks for resource leaks."""
    url = f"http://{host}/"
    for i in range(4):
        resp = urllib.request.urlopen(url, timeout=3)
        body = resp.read()
        assert resp.status == 200, f"Expected 200, got {resp.status}"
        assert len(body) > 4096, f"Body too small: {len(body)} bytes (expected >4kB)"


def test_scpi_via_http(host):
    """POST /api/scpi with a valid command returns 200 OK."""
    url = f"http://{host}/api/scpi"
    for i in range(4):
      req = urllib.request.Request(url, data=b"OUTP1:STAT OFF\n", method="POST")
      resp = urllib.request.urlopen(req, timeout=2)
      body = resp.read().decode()
      assert resp.status == 200
      assert "OK" in body


def test_scpi_invalid_command(host):
    """POST /api/scpi with garbage returns 400."""
    url = f"http://{host}/api/scpi"
    req = urllib.request.Request(url, data=b"TOTALLY:BOGUS\n", method="POST")
    try:
        urllib.request.urlopen(req, timeout=2)
        assert False, "Expected 400 error"
    except urllib.error.HTTPError as e:
        assert e.code == 400


def test_404_missing_file(host):
    """GET for a non-existent path returns 404."""
    url = f"http://{host}/no_such_file.xyz"
    try:
        urllib.request.urlopen(url, timeout=2)
        assert False, "Expected 404"
    except urllib.error.HTTPError as e:
        assert e.code == 404


def test_websocket_connect(host):
    """Open a WebSocket, verify upgrade succeeds, then close cleanly."""
    import socket
    import hashlib
    import base64

    ws_key = base64.b64encode(b"test-key-12345!!").decode()
    request = (
        f"GET /ws HTTP/1.1\r\n"
        f"Host: {host}\r\n"
        f"Upgrade: websocket\r\n"
        f"Connection: Upgrade\r\n"
        f"Sec-WebSocket-Key: {ws_key}\r\n"
        f"Sec-WebSocket-Version: 13\r\n"
        f"\r\n"
    )

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(3)
    try:
        s.connect((host, 80))
        s.sendall(request.encode())
        resp = s.recv(1024).decode("latin-1")
        assert "101" in resp, f"Expected 101 Switching Protocols, got: {resp[:80]}"
        assert "Upgrade" in resp
    finally:
        s.close()
    time.sleep(0.3)


def test_websocket_max_clients(host):
    """Open WEB_MAX_OPEN_SOCKETS WebSocket connections; verify they all upgrade.
    Then verify an extra HTTP request still works (LRU purge) or is rejected."""
    import socket

    ws_key = "dGVzdC1rZXktMTIzNDUhIQ=="  # base64("test-key-12345!!")
    request = (
        f"GET /ws HTTP/1.1\r\n"
        f"Host: {host}\r\n"
        f"Upgrade: websocket\r\n"
        f"Connection: Upgrade\r\n"
        f"Sec-WebSocket-Key: {ws_key}\r\n"
        f"Sec-WebSocket-Version: 13\r\n"
        f"\r\n"
    )

    held = []
    try:
        for i in range(WEB_MAX_OPEN_SOCKETS):
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(3)
            s.connect((host, 80))
            s.sendall(request.encode())
            resp = s.recv(1024).decode("latin-1")
            assert "101" in resp, f"WS connection {i} failed: {resp[:80]}"
            held.append(s)

        time.sleep(0.3)

        # With all sockets occupied, an HTTP request should still work
        # because lru_purge_enable=true will close the oldest socket
        try:
            resp = urllib.request.urlopen(f"http://{host}/", timeout=3)
            body = resp.read()
            assert resp.status == 200
        except (urllib.error.URLError, ConnectionError):
            pass  # acceptable — server may refuse when fully loaded
    finally:
        for s in held:
            s.close()
        time.sleep(0.5)
