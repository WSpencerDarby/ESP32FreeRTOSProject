import argparse
import csv
import json
import socket
import sys
import time


FIELDS = [
    "recv_ts",
    "source_ip",
    "source_port",
    "device_ms",
    "level",
    "tag",
    "task",
    "period_ms",
    "samples",
    "deadline_miss",
    "cpu_pct",
    "state",
    "exec_us_last",
    "exec_us_avg",
    "exec_us_max",
    "jitter_us_last",
    "jitter_us_avg_abs",
    "jitter_us_max_abs",
    "message",
]


def parse_line(line, addr):
    parts = line.split("\t", 3)
    row = {
        "recv_ts": time.time(),
        "source_ip": addr[0],
        "source_port": addr[1],
        "device_ms": "",
        "level": "UNKNOWN",
        "tag": "APP",
        "task": "",
        "period_ms": "",
        "samples": "",
        "deadline_miss": "",
        "cpu_pct": "",
        "state": "",
        "exec_us_last": "",
        "exec_us_avg": "",
        "exec_us_max": "",
        "jitter_us_last": "",
        "jitter_us_avg_abs": "",
        "jitter_us_max_abs": "",
        "message": line,
    }

    # Firmware sends: millis, level, tag, message.
    if len(parts) == 4:
        row["device_ms"] = parts[0]
        row["level"] = parts[1]
        row["tag"] = parts[2]
        row["message"] = parts[3]

    if row["tag"] == "SCHED_STATS":
        for item in row["message"].split():
            if "=" not in item:
                continue
            key, value = item.split("=", 1)
            if key in row:
                row[key] = value

    return row


def print_row(row, fmt, csv_writer=None):
    if fmt == "jsonl":
        print(json.dumps(row))
    elif fmt == "csv":
        csv_writer.writerow(row)
        sys.stdout.flush()
    elif fmt == "raw":
        print(f"{row['device_ms']}\t{row['level']}\t{row['tag']}\t{row['message']}")
    else:
        print(
            f"{row['source_ip']}:{row['source_port']}\t"
            f"{row['device_ms']}\t{row['level']}\t{row['tag']}\t{row['message']}"
        )


def main():
    parser = argparse.ArgumentParser(description="Listen for ESP32 UDP logs.")
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=4210)
    parser.add_argument("--output")
    parser.add_argument("--format", choices=["terminal", "raw", "jsonl", "csv"], default="terminal")
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((args.host, args.port))

    out = open(args.output, "a", newline="", encoding="utf-8") if args.output else None
    out_csv = None
    screen_csv = None

    if args.format == "csv":
        screen_csv = csv.DictWriter(sys.stdout, fieldnames=FIELDS, lineterminator="\n")
        screen_csv.writeheader()
        if out:
            out_csv = csv.DictWriter(out, fieldnames=FIELDS, lineterminator="\n")
            if out.tell() == 0:
                out_csv.writeheader()

    print(f"Listening on {args.host}:{args.port}", file=sys.stderr)

    try:
        while True:
            data, addr = sock.recvfrom(2048)
            line = data.decode("utf-8", errors="replace").strip()
            row = parse_line(line, addr)

            print_row(row, args.format, screen_csv)

            if out:
                if out_csv:
                    out_csv.writerow(row)
                elif args.format == "jsonl":
                    out.write(json.dumps(row) + "\n")
                elif args.format == "raw":
                    out.write(f"{row['device_ms']}\t{row['level']}\t{row['tag']}\t{row['message']}\n")
                else:
                    out.write(
                        f"{row['source_ip']}:{row['source_port']}\t"
                        f"{row['device_ms']}\t{row['level']}\t{row['tag']}\t{row['message']}\n"
                    )
                out.flush()
    finally:
        if out:
            out.close()


if __name__ == "__main__":
    main()
