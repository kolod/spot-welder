#!/usr/bin/env python3
from __future__ import annotations

from argparse import ArgumentParser, Namespace
from pathlib import Path
from re import search

from serial.tools import list_ports


def parse_args() -> Namespace:
    parser = ArgumentParser(description='Detect likely serial upload port and store it for Meson upload task.')
    parser.add_argument('--output', type=Path, required=True, help='Path to write detected port')
    parser.add_argument('--preferred', default='auto', help='Preferred port override, or auto')
    return parser.parse_args()


def port_rank(port: object) -> tuple[int, int, str]:
    text = ' '.join([
        getattr(port, 'description', '') or '',
        getattr(port, 'manufacturer', '') or '',
        getattr(port, 'hwid', '') or '',
    ]).lower()

    score = 0
    for token in ('ch340', 'cp210', 'ftdi', 'usb serial', 'usb-serial', 'wchusbserial', 'uart'):
        if token in text:
            score += 10

    device = getattr(port, 'device', '') or ''
    com_match = search(r'COM(\d+)$', device, flags=0)
    com_num = int(com_match.group(1)) if com_match else 9999
    return (score, -com_num, device)


def detect_port() -> str | None:
    ports = list(list_ports.comports())
    if not ports:
        return None
    ranked = sorted(ports, key=port_rank, reverse=True)
    return ranked[0].device


def main() -> int:
    args = parse_args()

    selected = args.preferred
    if selected == 'auto':
        selected = detect_port()

    if not selected:
        print('error: could not auto-detect upload port; pass -Dupload_port=COMx and retry')
        return 1

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(selected + '\n', encoding='utf-8')
    print(f'detected upload port: {selected} (saved to {args.output})')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
