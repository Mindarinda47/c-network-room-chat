#!/usr/bin/env python3
import contextlib
import os
import signal
import socket
import subprocess
import sys
import time


ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SERVER = os.path.join(ROOT, "build", "chat-server")


def reserve_port():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
        probe.bind(("127.0.0.1", 0))
        return probe.getsockname()[1]


def connect_client(port):
    deadline = time.time() + 3
    while True:
        client = None
        try:
            client = socket.create_connection(("127.0.0.1", port), timeout=1)
            client.settimeout(1)
            receive_until(client, "Join a room")
            return client
        except (OSError, AssertionError):
            if client is not None:
                client.close()
            if time.time() >= deadline:
                raise
            time.sleep(0.05)


def receive_until(client, expected):
    data = bytearray()
    deadline = time.time() + 2
    while expected.encode() not in data:
        if time.time() >= deadline:
            raise AssertionError(f"did not receive {expected!r}: {data!r}")
        try:
            chunk = client.recv(4096)
        except socket.timeout:
            continue
        if not chunk:
            raise AssertionError("server closed the connection")
        data.extend(chunk)
    return data.decode(errors="replace")


def send_parts(client, *parts):
    for part in parts:
        client.sendall(part.encode())
        time.sleep(0.02)


def assert_not_received(client, unexpected, duration=0.3):
    previous_timeout = client.gettimeout()
    deadline = time.time() + duration
    data = bytearray()
    try:
        while time.time() < deadline:
            client.settimeout(max(0.01, deadline - time.time()))
            try:
                chunk = client.recv(4096)
            except socket.timeout:
                break
            if not chunk:
                raise AssertionError("server closed the connection")
            data.extend(chunk)
            if unexpected.encode() in data:
                raise AssertionError(f"unexpected cross-room message: {data!r}")
    finally:
        client.settimeout(previous_timeout)


def require_connection_closed(client):
    previous_timeout = client.gettimeout()
    deadline = time.time() + 2
    try:
        while time.time() < deadline:
            client.settimeout(max(0.01, deadline - time.time()))
            try:
                chunk = client.recv(4096)
            except socket.timeout:
                continue
            if not chunk:
                return
        raise AssertionError("server did not close the client connection")
    finally:
        client.settimeout(previous_timeout)


def main():
    if not os.path.exists(SERVER):
        raise SystemExit("build/chat-server not found; run make first")

    port = reserve_port()
    process = subprocess.Popen(
        [SERVER, str(port)],
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    clients = []
    try:
        first = connect_client(port)
        second = connect_client(port)
        third = connect_client(port)
        clients.extend([first, second, third])

        send_parts(first, "/jo", "in 0\n")
        receive_until(first, "Joined room 0")

        second.sendall(b"/list\n/join 0\n")
        response = receive_until(second, "Joined room 0")
        if "Rooms:" not in response:
            raise AssertionError("coalesced /list response missing")

        first.sendall(b"hello from first\n")
        receive_until(second, "[client 1] hello from first")

        third.sendall(b"/join 1\n")
        receive_until(third, "Joined room 1")
        third.sendall(b"isolated message\n")
        assert_not_received(first, "isolated message")
        assert_not_received(second, "isolated message")

        second.sendall(b"/leave\n")
        receive_until(second, "Moved to the lobby")
        receive_until(first, "client 2 left room 0")

        for client in clients:
            client.sendall(b"/quit\n")
        for client in clients:
            require_connection_closed(client)

    finally:
        for client in clients:
            with contextlib.suppress(OSError):
                client.close()
        if process.poll() is None:
            process.send_signal(signal.SIGINT)
        try:
            output, _ = process.communicate(timeout=3)
        except subprocess.TimeoutExpired:
            process.kill()
            output, _ = process.communicate()

    if process.returncode not in (0, -signal.SIGINT):
        raise AssertionError(
            f"server exited with {process.returncode}:\n{output}"
        )
    print("integration test: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
