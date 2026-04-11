"""
    pytest tests/ --host 192.168.88.117
"""

import socket
import time
import pytest

RECV_TIMEOUT = 2.0
MAX_CLIENTS = 2

def _connect(host: str, port: int, retries: int = 3) -> socket.socket:
    for attempt in range(retries):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(RECV_TIMEOUT)
        try:
            s.connect((host, port))
            return s
        except OSError:
            s.close()
            if attempt == retries - 1:
                raise
            time.sleep(0.2)

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
        time.sleep(0.3)  # let server process disconnects before next test

def _recv_lines(sock: socket.socket, count: int, timeout: float = 5.0) -> list[str]:
    """Read exactly `count` CRLF-terminated lines from sock within timeout."""
    buf = ""
    deadline = time.monotonic() + timeout
    lines = []
    while len(lines) < count:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            break
        sock.settimeout(remaining)
        try:
            chunk = sock.recv(256).decode("ascii")
        except TimeoutError:
            break
        if not chunk:
            break
        buf += chunk
        while "\r\n" in buf:
            line, buf = buf.split("\r\n", 1)
            lines.append(line)
            if len(lines) >= count:
                break
    return lines


def _assert_numeric(lines: list[str], label: str) -> None:
    for line in lines:
        val = float(line)  # raises ValueError if not a number
        assert val >= 0.0, f"Unexpected negative {label}: {val}"


def _run_cont_cycle(s: socket.socket, cmd_on: bytes, cmd_off: bytes, label: str) -> None:
    """Generic ON/OFF continuous measurement cycle: arm, collect 3 readings, disarm."""
    s.sendall(cmd_on)
    time.sleep(3.0)
    lines = _recv_lines(s, 3, 8.0)
    assert len(lines) == 3, f"Expected 3 continuous {label} readings, got {len(lines)}: {lines}"
    _assert_numeric(lines, label)

    s.sendall(cmd_off)
    # Drain any in-flight measurement that arrived before OFF was processed
    s.settimeout(0.5)
    try:
        s.recv(256)
    except TimeoutError:
        pass

    # Verify silence: no further measurements for 3 seconds
    stray = _recv_lines(s, 1, timeout=3.0)
    assert len(stray) == 0, f"Received unexpected {label} reading(s) after OFF: {stray}"


def test_volt_cont(scpi_addr):
    """MEAS:VOLT:CONT ON delivers readings; OFF stops them. Run twice on same connection."""
    host, port = scpi_addr
    with _connect(host, port) as s:
        for _ in range(2):
            _run_cont_cycle(s, b"MEAS:VOLT:CONT ON (@1)\n", b"MEAS:VOLT:CONT OFF (@1)\n", "voltage")
            time.sleep(0.1)


def test_curr_cont(scpi_addr):
    """MEAS:CURR:CONT ON delivers readings; OFF stops them. Run twice on same connection."""
    host, port = scpi_addr
    with _connect(host, port) as s:
        for _ in range(2):
            _run_cont_cycle(s, b"MEAS:CURR:CONT ON (@1)\n", b"MEAS:CURR:CONT OFF (@1)\n", "current")
            time.sleep(0.1)

def _query(s: socket.socket, cmd: bytes) -> str:
    """Send a query and return the single response line (stripped)."""
    s.sendall(cmd)
    lines = _recv_lines(s, 1, timeout=2.0)
    assert len(lines) == 1, f"Expected 1 response line for {cmd!r}, got {len(lines)}: {lines}"
    return lines[0]


def test_meas_volt(scpi_addr):
    """MEAS:VOLT? (@1) returns a single numeric voltage reading."""
    host, port = scpi_addr
    with _connect(host, port) as s:
        val = float(_query(s, b"MEAS:VOLT? (@1)\n"))
        assert val >= 0.0, f"Unexpected negative voltage: {val}"


def test_meas_curr(scpi_addr):
    """MEAS:CURR? (@1) returns a single numeric current reading."""
    host, port = scpi_addr
    with _connect(host, port) as s:
        val = float(_query(s, b"MEAS:CURR? (@1)\n"))
        assert val >= 0.0, f"Unexpected negative current: {val}"

@pytest.mark.parametrize("setpoint", [0.0, 1.5, 5.0, 12.0])
def test_volt_setpoint_readback(scpi_addr, setpoint):
    """SOUR1:VOLT <val> followed by SOUR1:VOLT? returns the same value."""
    host, port = scpi_addr
    with _connect(host, port) as s:
        s.sendall(f"SOUR1:VOLT {setpoint}\n".encode("ascii"))
        readback = float(_query(s, b"SOUR1:VOLT?\n"))
        assert readback == pytest.approx(setpoint, rel=1e-4), (
            f"Voltage readback {readback} != setpoint {setpoint}"
        )


@pytest.mark.parametrize("setpoint", [0.0, 0.5, 1.0, 3.0])
def test_curr_setpoint_readback(scpi_addr, setpoint):
    """SOUR1:CURR <val> followed by SOUR1:CURR? returns the same value."""
    host, port = scpi_addr
    with _connect(host, port) as s:
        s.sendall(f"SOUR1:CURR {setpoint}\n".encode("ascii"))
        readback = float(_query(s, b"SOUR1:CURR?\n"))
        assert readback == pytest.approx(setpoint, rel=1e-4), (
            f"Current readback {readback} != setpoint {setpoint}"
        )


def test_syst_err_no_error(scpi_addr):
    """SYST:ERR? returns 0,'No error' when no error has occurred."""
    host, port = scpi_addr
    with _connect(host, port) as s:
        resp = _query(s, b"SYST:ERR?\n")
        assert resp == '0,"No error"', f"Unexpected SYST:ERR? response: {resp!r}"


def test_syst_err_clears_after_read(scpi_addr):
    """SYST:ERR? clears the error register: second read returns 0,'No error'."""
    host, port = scpi_addr
    with _connect(host, port) as s:
        # First read — may or may not have an error; second must be clear.
        _query(s, b"SYST:ERR?\n")
        resp = _query(s, b"SYST:ERR?\n")
        assert resp == '0,"No error"', f"Error register not cleared after first read: {resp!r}"
