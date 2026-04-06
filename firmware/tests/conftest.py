import socket
import pytest

def pytest_addoption(parser):
    parser.addoption("--host", default="127.0.0.1", help="SCPI server host (default: 127.0.0.1)")
    parser.addoption("--port", default=5025, type=int, help="SCPI server port (default: 5025)")

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
