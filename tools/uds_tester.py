#!/usr/bin/env python3
import argparse
import socket
import sys


def parse_args():
    parser = argparse.ArgumentParser(description="Interactive UDS TCP tester.")
    parser.add_argument("--host", default="127.0.0.1", help="diagnosticd host, default: 127.0.0.1")
    parser.add_argument("--port", type=int, default=5000, help="diagnosticd TCP port, default: 5000")
    parser.add_argument(
        "requests",
        nargs="*",
        help="UDS request lines. If omitted, an interactive prompt is opened.",
    )
    return parser.parse_args()


def recv_line(sock_file):
    line = sock_file.readline()
    if not line:
        raise ConnectionError("server closed the connection")
    return line.decode("ascii").strip()


def send_request(sock, sock_file, request):
    wire_request = request.strip()
    if not wire_request:
        return

    sock.sendall((wire_request + "\n").encode("ascii"))
    print(f"> {wire_request}")
    print(f"< {recv_line(sock_file)}")


def run_batch(sock, sock_file, requests):
    for request in requests:
        send_request(sock, sock_file, request)


def run_prompt(sock, sock_file):
    print("Connected. Type UDS hex bytes, or 'quit' to exit.")
    while True:
        try:
            request = input("uds> ").strip()
        except EOFError:
            print()
            return

        if request.lower() in {"quit", "exit"}:
            return

        send_request(sock, sock_file, request)


def main():
    args = parse_args()
    with socket.create_connection((args.host, args.port), timeout=5.0) as sock:
        with sock.makefile("rb") as sock_file:
            if args.requests:
                run_batch(sock, sock_file, args.requests)
            elif sys.stdin.isatty():
                run_prompt(sock, sock_file)
            else:
                run_batch(sock, sock_file, [line.strip() for line in sys.stdin if line.strip()])

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
