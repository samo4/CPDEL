import socket
import time
import pytest

def pytest_addoption(parser):
    parser.addoption("--host", default="127.0.0.1", help="SCPI server host (default: 127.0.0.1)")
    parser.addoption("--port", default=5025, type=int, help="SCPI server port (default: 5025)")


@pytest.fixture(scope="session")
def host(request):
    return request.config.getoption("--host")


@pytest.fixture(scope="session")
def scpi_addr(request):
    host = request.config.getoption("--host")
    port = request.config.getoption("--port")
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(2.0)
    try:
        s.connect((host, port))
        s.close()
    except OSError:
        pytest.skip(f"SCPI server not reachable at {host}:{port}")
    return host, port

@pytest.fixture(scope="session", autouse=True)
def check_idn(scpi_addr):
    """Verify IDN before any test runs. Aborts the session immediately on failure."""
    host, port = scpi_addr
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(2.0)
    data = ""
    try:
        s.connect((host, port))
        s.sendall(b"*IDN?\n")
        # Accumulate until CRLF — recv() may return before the terminator arrives
        while "\r\n" not in data:
            chunk = s.recv(256).decode("ascii")
            if not chunk:
                break
            data += chunk
    except OSError as e:
        pytest.exit(f"IDN check failed: {e}", returncode=1)
    finally:
        s.close()

    print(f"\nIDN response: {data!r}")  # always visible with pytest -s, helps debugging

    if not data.endswith("\r\n"):
        hex_repr = ' '.join(f'{ord(c):02x}' for c in data)
        pytest.exit(f"IDN response missing CRLF: {data!r} (hex: {hex_repr}) (len: {len(data)})", returncode=1)
    parts = data.strip().split(",")
    if len(parts) != 4 or parts[1] != "CPDEL" or parts[2] != "0":
        pytest.exit(f"IDN response malformed: {data!r}", returncode=1)
    if not parts[3].split("/")[0]:  # version must be non-empty
        pytest.exit(f"IDN firmware version empty: {data!r}", returncode=1)


@pytest.fixture(scope="session", autouse=True)
def settle_server_after_tests():
    """Give server time to process socket teardown after pytest finishes."""
    yield
    time.sleep(0.5)
