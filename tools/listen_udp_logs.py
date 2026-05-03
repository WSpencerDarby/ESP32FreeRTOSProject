import argparse
import socket


def main() -> None:
    parser = argparse.ArgumentParser(description="Listen for ESP32 UDP log lines.")
    parser.add_argument("--host", default="0.0.0.0", help="Interface to bind.")
    parser.add_argument("--port", type=int, default=4210, help="UDP port to listen on.")
    parser.add_argument("--output", help="Optional file to append received logs to.")
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((args.host, args.port))

    output_file = open(args.output, "a", encoding="utf-8") if args.output else None
    print(f"Listening for UDP logs on {args.host}:{args.port}")

    try:
        while True:
            data, address = sock.recvfrom(2048)
            line = data.decode("utf-8", errors="replace").rstrip()
            rendered = f"{address[0]}:{address[1]} {line}"
            print(rendered)
            if output_file:
                output_file.write(rendered + "\n")
                output_file.flush()
    finally:
        if output_file:
            output_file.close()


if __name__ == "__main__":
    main()
