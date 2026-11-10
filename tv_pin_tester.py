#!/usr/bin/env python3
# USB setup (Linux):
# - Use hardware with a USB device/OTG port and gadget support, such as a
#   compatible Raspberry Pi. A normal USB host-only port cannot act as a keyboard.
# - Enable USB peripheral mode and the Linux USB gadget/configfs drivers for
#   your board. Exact boot settings depend on the hardware and distribution.
# - Configure a HID keyboard gadget with a report descriptor for standard
#   8-byte keyboard reports. Add it to a USB configuration and bind it to the
#   USB device controller (UDC). This creates /dev/hidg0; grant write access.
# - Connect the device/OTG port to the TV's USB port with a data-capable cable.
#   If the HID node has a different name, update DEVICE below.
# The script sends keyboard reports; it does not configure the USB gadget.

import argparse
import math
import re
import time
from collections import deque
from contextlib import nullcontext
from datetime import datetime
from pathlib import Path

# ==========================================
# CONFIGURATION
# ==========================================

DEVICE = "/dev/hidg0"

PIN_LENGTH = 5

# USB HID top-row digits (not numpad): 1-9 are 0x1E-0x26; 0 is 0x27.
DIGIT_KEYS = {str(digit): 0x1D + digit for digit in range(1, 10)}
DIGIT_KEYS["0"] = 0x27

# Config works on tested TV
# PAUSE = 0
# HOLD = 0.02
# DELAY = 0.02

DEFAULT_HOLD = 0.12
DEFAULT_KEY_DELAY = 0.02
DEFAULT_PAUSE = 0.5

BASE_DIR = Path(__file__).resolve().parent
LOG_FILE = BASE_DIR / "tv_pin_attempts.log"

# ==========================================
# COMMAND LINE ARGUMENTS
# ==========================================

parser = argparse.ArgumentParser(
    description="Philips TV five-digit PIN tester"
)

parser.add_argument(
    "start",
    type=int,
    help="First PIN, inclusive"
)

parser.add_argument(
    "end",
    type=int,
    help="Last PIN, inclusive"
)

parser.add_argument(
    "--hold",
    type=float,
    default=DEFAULT_HOLD,
    help=f"Key hold duration in seconds (default: {DEFAULT_HOLD})"
)

parser.add_argument(
    "--key-delay",
    type=float,
    default=DEFAULT_KEY_DELAY,
    help=f"Delay after each key in seconds (default: {DEFAULT_KEY_DELAY})"
)

parser.add_argument(
    "--pause",
    type=float,
    default=DEFAULT_PAUSE,
    help=f"Pause after submitting a PIN (default: {DEFAULT_PAUSE})"
)

parser.add_argument(
    "--skip-tried",
    action="store_true",
    help="Exclude logged PINs before running, without printing individual skips"
)

parser.add_argument(
    "--no-log",
    action="store_true",
    help="Do not create or write the log (still read it with --skip-tried)"
)

parser.add_argument(
    "--distinct-first",
    action="store_true",
    help="Try PINs with more distinct digits first (including leading zeros)"
)

distinct_limits = parser.add_mutually_exclusive_group()
distinct_limits.add_argument(
    "--min-distinct",
    type=int,
    choices=range(0, 6),
    help=(
        "Only try PINs in the input range with at least this many different "
        "digit values. For example, 3 tries groups with 5, then 4, then 3 "
        "distinct digits; 5 only tries PINs with no repeated digits. "
        "Values 0 and 1 include all groups. Leading zeros count: 00123 has "
        "4 distinct digits. Requires --distinct-first; cannot be combined "
        "with --max-distinct."
    )
)

distinct_limits.add_argument(
    "--max-distinct",
    type=int,
    choices=range(0, 6),
    help=(
        "Only try PINs in the input range with at most this many different "
        "digit values. For example, 3 tries groups with 3, then 2, then 1 "
        "distinct digits. Values 0 and 1 only try PINs whose digits are all "
        "the same (such as 00000 or 11111); 5 includes all groups. Leading "
        "zeros count: 00123 has 4 distinct digits. Requires --distinct-first; "
        "cannot be combined with --min-distinct."
    )
)

args = parser.parse_args()

# ==========================================
# VALIDATION
# ==========================================

if args.min_distinct is not None and not args.distinct_first:
    parser.error("--min-distinct requires --distinct-first")

if args.max_distinct is not None and not args.distinct_first:
    parser.error("--max-distinct requires --distinct-first")

if not (0 <= args.start <= args.end <= 99999):
    parser.error(
        "PIN range must satisfy 0 <= start <= end <= 99999"
    )

for name in ("hold", "key_delay", "pause"):
    value = getattr(args, name)

    if not math.isfinite(value) or value < 0:
        parser.error(
            f"{name} must be a finite, nonnegative number"
        )

# ==========================================
# LOAD PREVIOUSLY TRIED PINS
# ==========================================

def load_tried_pins():
    """
    Read previously submitted PINs from the log.

    Only SUBMITTED entries are considered.
    Other lines are ignored.

    The log file is never modified here.
    """
    tried = set()

    pattern = re.compile(
        r"\bSUBMITTED\s+(\d{5})(?!\d)"
    )

    try:
        with LOG_FILE.open("r", encoding="utf-8") as log:
            for line in log:
                match = pattern.search(line)

                if match:
                    number = int(match.group(1))

                    # Keep only relevant PINs.
                    if args.start <= number <= args.end:
                        tried.add(number)
    except FileNotFoundError:
        # No history yet; there is nothing to skip.
        return set()

    return tried


tried_pins = set()

if args.skip_tried:
    try:
        tried_pins = load_tried_pins()
    except (OSError, UnicodeError) as error:
        parser.exit(1, f"Cannot read PIN log {LOG_FILE}: {error}\n")

# ==========================================
# BUILD ATTEMPT LIST
# ==========================================

all_pins = range(args.start, args.end + 1)
total_range = len(all_pins)

if args.distinct_first:
    minimum_distinct = max(1, args.min_distinct or 1)
    maximum_distinct = max(1, args.max_distinct) if args.max_distinct is not None else 5
    print(
        f"Building distinct-first order for {total_range:,} PINs "
        f"({args.start:05d} - {args.end:05d})...",
        flush=True
    )
    buckets = {distinct: [] for distinct in range(1, 6)}
    progress_interval = max(1, (total_range + 9) // 10)

    for processed, number in enumerate(all_pins, start=1):
        distinct = len(set(f"{number:05d}"))
        buckets[distinct].append(number)

        if processed % progress_interval == 0 or processed == total_range:
            print(
                f"Classified: {processed:,}/{total_range:,} "
                f"({processed / total_range * 100:.2f}%)",
                flush=True
            )

    for distinct in range(5, 0, -1):
        print(
            f"  {distinct} distinct digits: {len(buckets[distinct]):,} PINs"
            f"{' (excluded)' if not minimum_distinct <= distinct <= maximum_distinct else ''}",
            flush=True
        )

    print("Combining PIN groups...", flush=True)
    all_pins = [
        number
        for distinct in range(maximum_distinct, minimum_distinct - 1, -1)
        for number in buckets[distinct]
    ]
    del buckets
    print("Distinct-first order ready.", flush=True)

pins_to_try = [
    number
    for number in all_pins
    if number not in tried_pins
]

selected_count = len(all_pins)
excluded_attempts = total_range - selected_count
total_attempts = len(pins_to_try)
skipped_attempts = selected_count - total_attempts

# ==========================================
# HELPERS
# ==========================================

def format_duration(seconds):
    """Format a duration into useful units."""
    if seconds < 1:
        return f"{seconds:.2f}s"

    seconds = round(seconds)

    hours, remainder = divmod(seconds, 3600)
    minutes, secs = divmod(remainder, 60)

    if hours:
        return f"{hours}h {minutes:02d}m {secs:02d}s"

    if minutes:
        return f"{minutes}m {secs:02d}s"

    return f"{secs}s"


def press(keyboard, key):
    """Send a USB HID key press and release."""

    report = bytes([
        0, 0, key, 0, 0, 0, 0, 0
    ])

    keyboard.write(report)

    try:
        time.sleep(args.hold)

    finally:
        # Always release the key.
        keyboard.write(bytes(8))

    time.sleep(args.key_delay)


def enter_pin(keyboard, pin):
    """
    Enter a five-digit PIN.

    Assumptions:
    - Screen starts at 00000.
    - Leftmost digit selected.
    - Top-row digit keys set the digit and automatically advance.
    - The fifth digit automatically submits the PIN.
    - Rejection resets to 00000 with the leftmost digit selected.
    """
    global phase

    for index, char in enumerate(pin):
        if index == PIN_LENGTH - 1:
            phase = "submitting"

        press(keyboard, DIGIT_KEYS[char])


# ==========================================
# PREPARE ESTIMATION
# ==========================================

remaining_keys = total_attempts * PIN_LENGTH

completed_keys = 0
completed_attempts = 0
total_input_seconds = 0.0

last_submitted = None
current_pin = None
phase = "not started"

recent = deque(maxlen=10)

# ==========================================
# STARTUP INFORMATION
# ==========================================

print("=" * 55)
print("PHILIPS TV PIN TESTER")
print("=" * 55)

print(f"Range: {args.start:05d} - {args.end:05d}")
print(f"PINs in range: {total_range:,}")
print(f"PIN order: {'DISTINCT FIRST' if args.distinct_first else 'ASCENDING'}")
if args.min_distinct is not None:
    print(f"Minimum distinct digits: {args.min_distinct}")
if args.max_distinct is not None:
    print(f"Maximum distinct digits: {max(1, args.max_distinct)}")
if args.min_distinct is not None or args.max_distinct is not None:
    print(f"PINs selected: {selected_count:,}")
    print(f"PINs excluded by distinct-digit filter: {excluded_attempts:,}")

print(
    f"Skip previously tried: "
    f"{'YES' if args.skip_tried else 'NO'}"
)

print(f"Already tried / skipped: {skipped_attempts:,}")
print(f"PINs remaining: {total_attempts:,}")

print()
print(f"Key hold: {args.hold}s")
print(f"Key delay: {args.key_delay}s")
print(f"PIN pause: {args.pause}s")

print()
print(f"Planned key presses: {remaining_keys:,}")
print(f"Log file: {LOG_FILE}")
print(f"Log writing: {'DISABLED' if args.no_log else 'ENABLED'}")

print()
print("Press CTRL+C to stop.")
print("TV must start at 00000, leftmost digit selected.")
print("=" * 55, flush=True)

# ==========================================
# MAIN LOOP
# ==========================================

program_start = time.monotonic()

try:
    # A fully logged range needs no device access.
    keyboard_context = (
        open(DEVICE, "wb", buffering=0)
        if total_attempts else nullcontext()
    )
    with keyboard_context as keyboard:

        try:
            # Do not open the log at all when writing is disabled.
            # Append mode preserves all existing entries otherwise.
            log_context = (
                nullcontext() if args.no_log or not total_attempts else open(
                    LOG_FILE,
                    "a",
                    encoding="utf-8",
                    buffering=1
                )
            )
            with log_context as log:

                for number in pins_to_try:

                    current_pin = f"{number:05d}"
                    phase = "entering"

                    print(
                        f"\nENTERING: {current_pin}",
                        flush=True
                    )

                    # Measure actual input duration.
                    start_time = time.monotonic()

                    enter_pin(keyboard, current_pin)

                    input_duration = (
                        time.monotonic() - start_time
                    )

                    # The fifth digit was sent, submitting the PIN.
                    last_submitted = current_pin
                    recent.append(current_pin)
                    phase = "waiting"

                    completed_attempts += 1

                    completed_keys += PIN_LENGTH
                    remaining_keys -= PIN_LENGTH

                    total_input_seconds += input_duration

                    remaining_attempts = (
                        total_attempts - completed_attempts
                    )

                    # Calculate measured seconds per key.
                    seconds_per_key = (
                        total_input_seconds / completed_keys
                    )

                    # Remaining keystrokes plus
                    # remaining between-PIN pauses.
                    eta = (
                        remaining_keys * seconds_per_key
                        + remaining_attempts * args.pause
                    )

                    if log is not None:
                        timestamp = datetime.now().isoformat(
                            timespec="seconds"
                        )

                        # Always append; never overwrite.
                        log.write(
                            f"{timestamp} SUBMITTED "
                            f"{current_pin}\n"
                        )

                    percentage = (
                        completed_attempts
                        / total_attempts * 100
                    )

                    print(
                        f"SUBMITTED: {current_pin} | "
                        f"{completed_attempts:,}/"
                        f"{total_attempts:,} "
                        f"({percentage:.2f}%) | "
                        f"Took: {input_duration:.2f}s | "
                        f"ETA: {format_duration(eta)}",
                        flush=True
                    )

                    # Only pause when another PIN remains.
                    if remaining_attempts > 0:
                        time.sleep(args.pause)

                phase = "completed"

        finally:
            # Release any held keyboard key.
            if keyboard is not None:
                try:
                    keyboard.write(bytes(8))
                except OSError:
                    pass

except KeyboardInterrupt:
    print(
        "\n\nSTOPPED BY USER (CTRL+C)!",
        flush=True
    )

except OSError as error:
    print(
        f"\nUSB or file error: {error}",
        flush=True
    )

finally:
    elapsed = time.monotonic() - program_start

    print()
    print("=" * 55)
    print("FINAL STATUS")
    print("=" * 55)

    print(f"Phase: {phase}")
    print(f"Current PIN: {current_pin}")
    print(f"Last submitted PIN: {last_submitted}")

    print()
    if not selected_count:
        print("No PINs in this range match the distinct-digit filter.")
        print("Nothing was sent to the TV.")
        print()
    elif not total_attempts:
        print("No PINs left to test in this range.")
        print("All selected PINs are already logged.")
        print("Nothing was sent to the TV.")
        print()

    print(f"PINs skipped: {skipped_attempts:,}")
    print(
        f"PINs submitted this run: "
        f"{completed_attempts:,}/{total_attempts:,}"
    )

    print(
        f"Elapsed time: {format_duration(elapsed)}"
    )

    print()
    print("Last submitted PINs:")

    if recent:
        for pin in recent:
            print(f"  {pin}")
    else:
        print("  None")

    if phase in ("entering", "submitting"):
        print()
        print(
            "WARNING: The current PIN may be "
            "partially entered or submitted."
        )
        print(
            "Reset the TV PIN screen before "
            "starting another batch."
        )

    print()
    if args.no_log:
        print("Log writing disabled; this run was not saved.")
    else:
        print(f"Full history: {LOG_FILE}")
    print("=" * 55)

