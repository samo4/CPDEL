"""
    pytest tests/ --host 192.168.88.117
"""

import socket
import time

RECV_TIMEOUT = 2.0
MAX_CLIENTS = 4


def _connect(host: str, port: int) -> socket.socket:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(RECV_TIMEOUT)
    s.connect((host, port))
    return s


# ---------------------------------------------------------------------------
# test_idn
# ---------------------------------------------------------------------------

def test_idn(scpi_addr):
    """*IDN? must return a 4-field SCPI identity string terminated with CRLF."""
    host, port = scpi_addr
    with _connect(host, port) as s:
        s.sendall(b"*IDN?\n")
        data = s.recv(256).decode("ascii")

    assert data.endswith("\r\n"), f"Response must end with CRLF, got: {data!r}"
    parts = data.strip().split(",")
    assert len(parts) == 4, f"Expected 4 comma-separated IDN fields, got: {data!r}"
    assert parts[0] == "CPDEL",  f"Unexpected manufacturer field: {parts[0]!r}"
    assert parts[1] == "SOUSIM", f"Unexpected model field: {parts[1]!r}"
    assert parts[2] == "0",      f"Unexpected serial field: {parts[2]!r}"
    assert parts[3],             "Firmware version field must not be empty"


# ---------------------------------------------------------------------------
# test_client_limit
# ---------------------------------------------------------------------------

def test_client_limit(scpi_addr):
    """The server must close the (MAX_CLIENTS + 1)-th connection immediately."""
    host, port = scpi_addr
    held = []
    try:
        for _ in range(MAX_CLIENTS):
            held.append(_connect(host, port))

        # Give tasks a moment to be scheduled so slots are truly consumed
        time.sleep(0.1)

        extra = _connect(host, port)
        try:
            data = extra.recv(256)
            assert data == b"", (
                f"Expected EOF on connection #{MAX_CLIENTS + 1}, got: {data!r}"
            )
        finally:
            extra.close()
    finally:
        for s in held:
            s.close()
