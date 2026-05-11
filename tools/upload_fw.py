#!/usr/bin/env python3
from __future__ import annotations

from argparse import ArgumentParser, Namespace
from pathlib import Path
from subprocess import run
import sys

from serial.tools import list_ports


def parse_args() -> Namespace:
    parser = ArgumentParser(description='Upload firmware using stored or auto-detected serial port.')
    parser.add_argument('--fw', type=Path, required=True, help='Firmware .ihx file path')
    parser.add_argument('--protocol', required=True, help='stcgal protocol')
    parser.add_argument('--port-file', type=Path, required=True, help='Path to stored upload port file')
    parser.add_argument('--fallback-port', default='auto', help='Fallback upload port or auto')
    return parser.parse_args()


def detect_port() -> str | None:
    ports = list(list_ports.comports())
    if not ports:
        return None
    # Prefer common USB-UART adapters by description/manufacturer.
    preferred_tokens = ('ch340', 'cp210', 'ftdi', 'usb serial', 'usb-serial', 'wchusbserial', 'uart')

    def score(p: object) -> int:
        text = ' '.join([
            getattr(p, 'description', '') or '',
            getattr(p, 'manufacturer', '') or '',
            getattr(p, 'hwid', '') or '',
        ]).lower()
        return sum(10 for token in preferred_tokens if token in text)

    return sorted(ports, key=score, reverse=True)[0].device


def resolve_port(port_file: Path, fallback_port: str) -> str | None:
    if port_file.exists():
        value = port_file.read_text(encoding='utf-8', errors='replace').strip()
        if value:
            return value

    if fallback_port != 'auto':
        return fallback_port

    return detect_port()


def main() -> int:
    args = parse_args()

    port = resolve_port(args.port_file, args.fallback_port)
    if not port:
        print('error: no upload port available. Run detect-upload-port or set -Dupload_port=COMx')
        return 1

    args.port_file.parent.mkdir(parents=True, exist_ok=True)
    args.port_file.write_text(port + '\n', encoding='utf-8')

    cmd = [
        sys.executable,
        '-m', 'stcgal',
        '-p', port,
        '-P', args.protocol,
        str(args.fw),
    ]
    print(f'uploading via port: {port}')
    result = run(cmd, check=False)
    return result.returncode


if __name__ == '__main__':
    raise SystemExit(main())
