#!/usr/bin/env python3

import argparse
import math
import re
import time
from collections import deque
from datetime import datetime
from pathlib import Path

# ==========================================
# CONFIGURATION
# ==========================================

DEVICE = "/dev/hidg0"

UP = 0x52
DOWN = 0x51
RIGHT = 0x4F

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
    help="Key hold duration in seconds (default: 0.12)"
)

parser.add_argument(
    "--key-delay",
    type=float,
    default=DEFAULT_KEY_DELAY,
    help="Delay after each key in seconds (default: 0.15)"
)

parser.add_argument(
    "--pause",
    type=float,
    default=DEFAULT_PAUSE,
    help="Pause after submitting a PIN (default: 3.0)"
)

parser.add_argument(
    "--skip-tried",
    action="store_true",
    default=False,
    help="Skip PINs already submitted in the log"
)

args = parser.parse_args()

# ==========================================
# VALIDATION
# ==========================================

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

    if not LOG_FILE.exists():
        return tried

    pattern = re.compile(
        r"\bSUBMITTED\s+(\d{5})(?!\d)"
    )

    with LOG_FILE.open("r", encoding="utf-8") as log:
        for line in log:
            match = pattern.search(line)

            if match:
                number = int(match.group(1))

                # Keep only relevant PINs.
                if args.start <= number <= args.end:
                    tried.add(number)

    return tried


tried_pins = set()

if args.skip_tried:
    tried_pins = load_tried_pins()

# ==========================================
# BUILD ATTEMPT LIST
# ==========================================

all_pins = range(args.start, args.end + 1)

pins_to_try = [
    number
    for number in all_pins
    if number not in tried_pins
]

total_range = args.end - args.start + 1
total_attempts = len(pins_to_try)
skipped_attempts = total_range - total_attempts

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


def key_count(number):
    """
    Calculate how many keyboard presses
    are required to enter this PIN from 00000.
    """
    pin = f"{number:05d}"

    # Five RIGHT presses, including submission.
    count = 5

    for char in pin:
        digit = int(char)

        # Shortest circular distance from zero.
        count += min(digit, 10 - digit)

    return count


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
    - UP increases a digit.
    - DOWN decreases a digit.
    - RIGHT advances.
    - RIGHT on digit five submits.
    """
    global phase

    for index, char in enumerate(pin):
        digit = int(char)

        if digit <= 5:
            direction = UP
            presses = digit
        else:
            direction = DOWN
            presses = 10 - digit

        for _ in range(presses):
            press(keyboard, direction)

        if index == 4:
            phase = "submitting"

        press(keyboard, RIGHT)


# ==========================================
# PREPARE ESTIMATION
# ==========================================

remaining_keys = sum(
    key_count(number)
    for number in pins_to_try
)

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

print()
print("Press CTRL+C to stop.")
print("TV must start at 00000, leftmost digit selected.")
print("=" * 55, flush=True)

# ==========================================
# HANDLE EMPTY RANGE
# ==========================================

if not pins_to_try:
    print()
    print("No PINs left to test in this range.")
    print("All requested PINs are already logged.")
    print("Nothing was sent to the TV.")
    raise SystemExit(0)

# ==========================================
# MAIN LOOP
# ==========================================

program_start = time.monotonic()

try:
    with open(DEVICE, "wb", buffering=0) as keyboard:

        try:
            # Append mode preserves all existing entries.
            with open(
                LOG_FILE,
                "a",
                encoding="utf-8",
                buffering=1
            ) as log:

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

                    # The submission key was sent.
                    last_submitted = current_pin
                    recent.append(current_pin)
                    phase = "waiting"

                    completed_attempts += 1

                    keys_used = key_count(number)

                    completed_keys += keys_used
                    remaining_keys -= keys_used

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
    print(f"Full history: {LOG_FILE}")
    print("=" * 55)

