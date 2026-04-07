"""
Verify the embedded HTTP server serves the UI.

    pytest tests/test_web_server.py --host 192.168.88.117
"""

import time
import urllib.request


def test_index_page(host):
    """GET / must return 200, respond within 2 s, and deliver >4 kB."""
    url = f"http://{host}/"
    start = time.monotonic()
    resp = urllib.request.urlopen(url, timeout=2)
    elapsed = time.monotonic() - start
    body = resp.read()

    assert resp.status == 200, f"Expected 200, got {resp.status}"
    assert elapsed < 2.0, f"Response took {elapsed:.2f}s (limit 2s)"
    assert len(body) > 4096, f"Body too small: {len(body)} bytes (expected >4kB)"
