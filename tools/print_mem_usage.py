#!/usr/bin/env python3
"""Parse SDCC .mem output and print firmware memory usage summary."""

from re import MULTILINE, finditer, search
from argparse import ArgumentParser, Namespace
from pathlib import Path
from rich.console import Console
from rich import box
from rich.table import Table
from rich.text import Text


def parse_mem_report(mem_text: str):
    """Parse the SDCC .mem report text and extract flash usage, stack availability, and RAM usage."""
    flash_used = None
    stack_available = None
    internal_ram_size = None
    external_ram_used = None

    # SDCC summary table provides actual code size used in flash.
    flash_match = search(
        r"ROM/EPROM/FLASH\s+0x[0-9a-fA-F]+\s+0x[0-9a-fA-F]+\s+(\d+)\s+(\d+)",
        mem_text,
    )
    if flash_match:
        flash_used = int(flash_match.group(1))

    # This value is reported as free bytes from the current stack start.
    stack_match = search(r"with\s+(\d+)\s+bytes available\.", mem_text)
    if stack_match:
        stack_available = int(stack_match.group(1))

    external_match = search(r"EXTERNAL RAM\s+(\d+)\s+(\d+)", mem_text)
    if external_match:
        external_ram_used = int(external_match.group(1))

    # Infer internal RAM size from the rendered layout rows (e.g. 0x00..0xF0 => 256 bytes).
    row_addrs = [int(m.group(1), 16) for m in finditer(r"^0x([0-9a-fA-F]{2}):", mem_text, MULTILINE)]
    if row_addrs:
        internal_ram_size = (max(row_addrs) + 0x10)

    return flash_used, stack_available, internal_ram_size, external_ram_used


def pct(used: int, total: int) -> float:
    """Calculate percentage usage, guarding against division by zero."""
    if total <= 0:
        return 0.0
    return (used * 100.0) / total


def parse_args() -> Namespace:
    """Parse command-line arguments."""
    parser = ArgumentParser(description='Parse SDCC .mem output and print firmware memory usage summary.')
    parser.add_argument('ihx_path', type=Path, help='Path to firmware .ihx output')
    parser.add_argument('report_out', type=Path, help='Path to generated report file')
    parser.add_argument('flash_bytes', type=int, help='Configured flash size limit in bytes')
    parser.add_argument('ram_bytes', type=int, help='Configured RAM size limit in bytes')
    return parser.parse_args()


def usage_style(percentage: float) -> str:
    """Determine text style based on usage percentage."""
    if percentage >= 100.0:
        return 'bold red'
    if percentage >= 85.0:
        return 'bold yellow'
    return 'bold green'


def format_usage_line(label: str, used: int | None, total: int | None, unavailable_reason: str, suffix: str = '') -> str:
    """Format a single line of usage text, handling unavailable data gracefully."""
    if used is None or total is None:
        return f"- {label}: unavailable ({unavailable_reason})"
    return f"- {label}: {used}/{total} bytes ({pct(used, total):.1f}%){suffix}"


def build_usage_text(used: int | None, total: int | None, suffix: str = '') -> Text:
    """Build a rich Text object for usage display, applying color based on percentage."""
    if used is None or total is None:
        return Text('Unavailable (parse error)', style='yellow')
    usage_pct = pct(used, total)
    return Text(f"{used}/{total} bytes ({usage_pct:.1f}%){suffix}", style=usage_style(usage_pct))


def main() -> int:
    """Main entry point for the memory usage report script."""
    args = parse_args()
    console = Console(
        force_terminal=True,
        color_system='truecolor',
        no_color=False,
        legacy_windows=False,
    )

    ihx_path = args.ihx_path
    report_out = args.report_out
    flash_limit = args.flash_bytes
    ram_limit = args.ram_bytes

    # The SDCC linker emits a .mem report next to the generated .ihx file.
    mem_path = ihx_path.with_suffix('.mem')
    if not mem_path.exists():
        print(f"error: SDCC memory report not found: {mem_path}")
        return 1

    mem_text = mem_path.read_text(encoding='utf-8', errors='replace')
    flash_used, stack_available, internal_ram_size, external_ram_used = parse_mem_report(mem_text)

    lines = ["Memory Usage Summary"]
    lines.append(f"- Source report: {mem_path.name}")

    internal_ram_used = None
    # Internal RAM usage is inferred from: total internal bytes - free stack bytes.
    if stack_available is not None and internal_ram_size is not None:
        internal_ram_used = max(internal_ram_size - stack_available, 0)

    lines.append(
        format_usage_line('Flash', flash_used, flash_limit, 'could not parse ROM/EPROM/FLASH row')
    )
    lines.append(
        format_usage_line(
            'Internal RAM (estimated)',
            internal_ram_used,
            internal_ram_size,
            'could not parse layout/stack availability',
            f', stack free={stack_available}' if stack_available is not None else '',
        )
    )
    lines.append(
        format_usage_line('External RAM', external_ram_used, ram_limit, 'could not parse EXTERNAL RAM row')
    )

    report_text = "\n".join(lines) + "\n"
    # Persist a plain-text report for CI logs and post-build tooling.
    report_out.write_text(report_text, encoding='utf-8')

    table = Table(
        title='Memory Usage Summary',
        title_style='bold magenta',
        box=box.SIMPLE_HEAD,
        safe_box=False,
        border_style='bright_blue',
        header_style='bold bright_cyan',
        pad_edge=True,
    )
    table.add_column('Metric', style='bold cyan')
    table.add_column('Value', style='white')

    table.add_row('Source report', mem_path.name)

    table.add_row('Flash', build_usage_text(flash_used, flash_limit))
    table.add_row(
        'Internal RAM (estimated)',
        build_usage_text(
            internal_ram_used,
            internal_ram_size,
            f', stack free={stack_available}' if stack_available is not None else '',
        ),
    )
    table.add_row('External RAM', build_usage_text(external_ram_used, ram_limit))

    console.print(table)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
