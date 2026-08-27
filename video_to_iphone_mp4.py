#!/usr/bin/env python3
"""
video_to_iphone_mp4.py — Recursively convert arbitrary FFmpeg-decodable video
files to compact, validated MP4 files for a selected playback target.

What it does
------------
The script recursively scans FOLDER and probes regular files with ffprobe instead
of trusting filename extensions. A file containing a video stream is input even
when it has an unusual or misleading extension such as .json. Real JSON, text,
images, archives, and other non-video files are ignored. Sources are never
modified or deleted. Normal and audio-only outputs mirror source directories
beneath FOLDER/converted/TARGET/; --container uses the isolated namespace
FOLDER/converted/container/TARGET/. The entire converted/ tree is excluded from
input discovery. Deterministic suffixes prevent same-stem inputs from colliding.

Playback targets
----------------
--target selects compatibility policy; it is separate from the encoder-speed
--preset option. All targets use MP4 with +faststart and AAC-LC audio:

    iphone   auto-select new HEVC Main/yuv420p 8-bit or
             HEVC Main10/yuv420p10le 10-bit encodes, always hvc1;
             safe retained video may use either matched combination;
             maximum 3840x2160 and 60 fps
    windows  H.264 High, yuv420p, avc1; preserve resolution and frame rate
    mac      HEVC Main, yuv420p, hvc1; preserve resolution and frame rate
    lg-tv    H.264 High, yuv420p, avc1; preserve resolution and frame rate
    linux    HEVC Main, yuv420p, MP4-compatible hvc1/hev1 sample entry;
             preserve resolution and frame rate

The iphone target is the default and is the only target with the intentional
4K/60 envelope. It never upscales and preserves aspect ratio when downscaling.
Other targets do not impose an artificial resolution or frame-rate cap; they
only make odd dimensions divisible by two for yuv420p encoder correctness.
The iPhone encoder does not globally force either bit depth. It evaluates HEVC
Main/yuv420p 8-bit and HEVC Main10/yuv420p10le 10-bit as distinct strategies,
subject to actual backend support. Encoding an 8-bit source as Main10 cannot
restore precision or detail absent from that source; Main10 is tested only for
possible compression/quantization efficiency at the mapped quality target.

Audio and silent-video policy
-----------------------------
Audio safety is a target policy shared by normal conversion, --audio-only, and
--container; the modes never use different iPhone rules. For iphone, only AAC-LC mono or
stereo with an explicit standard mono/stereo layout at 44.1 or 48 kHz is copied.
AAC 5.1/5.1(side), PCE-like or unknown layouts, AC-3/E-AC-3, Opus, Vorbis, DTS,
TrueHD, PCM, MP3, non-LC AAC, and every other unproven stream are converted to
AAC-LC stereo at 48 kHz and 192 kbps. This conservative rule applies whether
normal mode encodes/remuxes video or either copy-only mode preserves it.
Other targets retain the existing AAC-LC policy: safe streams through 5.1 may be
copied, conversion preserves up to 5.1, and more channels are downmixed.

By default, videos with no audio stream are skipped before encoding and reported
in a dedicated no-audio summary, not as failures. --convert-no-audio converts
them normally and produces an MP4 with no audio stream. With --audio-only it
allows a silent, target-safe video stream to be remuxed without adding audio;
with --container it allows any silent video that MP4 can mux to be remuxed.

Audio-only video-copy mode
--------------------------
--audio-only is not audio extraction: its result is still a normal MP4 made from
the original discovered source. It forbids video encoding. The source video must
pass the selected target's conservative stream-copy checks; otherwise the file
is listed under "Skipped — video incompatible with --audio-only" instead of
being encoded. A successful command always uses -c:v copy, applies hvc1/avc1 as
required, copies each target-safe audio stream, and repairs each unsafe stream
with the same shared target audio policy used by normal conversion.

Subtitle conversion/removal, +faststart, target output paths, collision handling,
existing-output skips, temporary-file promotion, and final validation still
apply. An already target-compatible source MP4 that needs no subtitle removal is
listed as already compatible rather than duplicated. Video preflight, adaptive
samples, 8/10-bit selection, and hardware/software video encoders are completely
bypassed. --gpu, --crf, and --preset are accepted but do not affect video in this
mode. With no --convert-no-audio, silent input keeps the usual no-audio skip.

Container-only MP4 mode
-----------------------
--container is distinct from --audio-only and the two flags are mutually
exclusive. It ignores the selected target's video codec, profile, pixel format,
resolution, frame-rate, and level rules. Instead it attempts to place the
original primary video bitstream into MP4 with -c:v copy. The selected --target
controls audio safety only. HEVC receives hvc1 and H.264 receives avc1; FFmpeg
selects the MP4 sample entry for other codecs. This mode does not guarantee that
the preserved video plays on the selected device.

Only existing mov_text subtitles are copied in --container mode. Other subtitle
codecs are dropped, never transcoded; --no-subs strips all subtitles. Outputs are
written under FOLDER/converted/container/TARGET/ through a tracked temporary
file. An already-ready MP4 with target-safe audio and no requested subtitle
change is skipped. If FFmpeg cannot mux the unchanged video into MP4, the input
is reported under "Skipped — video cannot be copied into MP4" and no encoder is
started. A completed mux must still validate; validation failure is a real
failure. --gpu, --crf, and --preset have no effect in this mode.

Subtitle policy
---------------
Unless --no-subs is used, compatible text subtitles are transcoded to mov_text.
Styled, image-based, and other incompatible subtitle formats are skipped instead
of failing normal/audio-only conversion. Container mode instead copies only
already-mov_text subtitles and drops the rest without conversion.

Size, remux, and validation policy
----------------------------------
For iPhone, the script first proves whether the existing video stream is already
safe. HEVC Main/yuv420p and Main 10/yuv420p10le inside the iPhone 4K/60/level
envelope are eligible for lossless stream-copy, but neither is automatically
remuxed. Direct remux is one candidate only when that proof passes. Both safe
and incompatible source video enter an adaptive encode-candidate preflight:
incompatible sources compare only encodes, while safe sources compare direct
remux with both encode variants.

The preflight starts with the same five short, video-only source ranges at 10%,
30%, 50%, 70%, and 90% for both 8-bit and 10-bit. If either variant's coefficient
of variation is above 0.25, both receive four additional ranges at 20%, 40%, 60%,
and 80%; stable videos need up to ten sample encodes (five positions times two
variants), while one usable backend needs at most eighteen. A failed backend may
be retried from a clean sample set on the next backend. Samples use the exact
backend, preset, scaling, frame-rate, codec, profile, level, pixel format, and
mapped quality settings that the selected full encode will use. The normal
sample duration is 10 seconds and is shortened for short videos; duplicate end
positions are avoided where possible.

The estimator measures each variant's video independently from audio. It uses
the median bitrate so one outlier cannot dominate, then applies an independent
variability margin: 1.05 for CV <=0.10, 1.10 through CV 0.25, or 1.15 above 0.25.
Complete candidate sizes also include the normal AAC/copy result, retained text
subtitle payload, and conservative container overhead. Equal numeric CRF/CQ
values are not treated as equal quality: centralized backend mappings give the
10-bit candidate a small conservative quality advantage. A future objective
metric can replace this approximate mapping without changing candidate selection.

For a safe source, the smallest quality-qualified encode must predict >=10%
savings versus remux; otherwise remux wins and avoids generation loss. When
remux is incompatible, the smallest supported quality-qualified encode wins
without applying a remux threshold. Failed sample/validation removes only that
variant; if no reliable estimate remains, HEVC Main 8-bit is the conservative
fallback. Sampling remains an estimate, not a guarantee about every scene.

Direct remux uses -c:v copy, applies hvc1 for HEVC or avc1 for H.264, and causes
no video generation loss. Audio is still independently copied or converted to
AAC-LC, and compatible subtitles are independently converted to mov_text while
unsupported subtitles are skipped. A direct-remux candidate is temporary and
must validate before promotion. Failure removes it and falls back to the best
sampled encode variant.

For video that needs or justifies encoding, the script creates and validates a
full target-compatible temporary re-encode rather than trusting FFmpeg's exit
code. The winning full encode uses the exact sampled bit-depth configuration.
If requested, hardware encoders are attempted in a conservative detected order;
every failed or invalid result is discarded before the next equivalent backend
is tried. A selected 10-bit hardware strategy falls back to 10-bit libx265, not
silently to 8-bit. An explicitly sampled alternate variant may be reconsidered
only after every semantically equivalent backend fails.

When preflight selects encoding for an already-safe iPhone source, the completed
encode is still compared with a separately created and validated remux candidate.
This protects against inaccurate sampling: the known-good encode remains intact
until the remux completes and validates, and the smaller valid result wins.

If the valid re-encode is not smaller than its source, stream-copy/remux is only
considered when codec, profile, pixel format/chroma, dimensions, frame rate,
level, and target tag policy are genuinely safe for the selected target. The
remux is written to a separate temporary file, validated, and compared with the
known-good re-encode. The good re-encode is never removed before a replacement
has completed successfully, passed validation, and been selected. The smallest
valid candidate wins. When no smaller valid candidate exists, the compatible
re-encode is kept even when it is larger than the source; an incompatible source
is never copied merely because it is smaller.

Target-aware validation checks an MP4/MOV-family container, a permitted video
codec/profile/pixel format/sample entry, target resolution/frame-rate/level
limits, target-safe AAC-LC audio, and mov_text retained subtitles. iPhone output
audio must satisfy the same conservative mono/stereo layout and sample-rate rule
used before deciding to copy audio; questionable multichannel AAC is rejected.
Validation checks the
properties promised here but cannot emulate every physical playback device.
For iPhone, validation accepts matched encoded or retained combinations—
Main/yuv420p or Main 10/yuv420p10le—not an independent profile/pixel-format
cross-product. Both still require hvc1 and the same 4K/60/level envelope.
Container-mode validation deliberately replaces those target video checks with
an exact source/output codec-name comparison, MP4 container checks, hvc1/avc1
where applicable, the normal target audio policy, and mov_text-only subtitles.
The final summary groups successful outputs by video method first. Actual video
re-encodes are then split by exact final byte count into size reduced, size
increased, or size unchanged. Direct and fallback remux results share a separate
"video not re-encoded" section regardless of size direction because their audio,
subtitles, container, and tags may still change. Existing-output skips, no-audio
skips, failures, and interruptions remain separate; aggregate successful totals
include every completed re-encode and remux but exclude those other categories.
Audio-only successes appear in the copied/remuxed section; incompatible-video
and already-target-compatible skips have their own sections. Container successes
also appear as copied/remuxed; unmuxable and already-ready inputs are separate
skip categories.

GPU behavior
------------
--gpu means auto-detect and try suitable hardware encoding for the target codec,
not NVIDIA-only. Depending on the OS, hardware, drivers, devices, and installed
FFmpeg build, H.264 candidates are h264_nvenc, h264_amf, h264_qsv,
h264_videotoolbox, and h264_vaapi; HEVC candidates are hevc_nvenc, hevc_amf,
hevc_qsv, hevc_videotoolbox, and hevc_vaapi. Listing an encoder is not considered
proof that it works: an actual encode must finish and validate. If all candidates
fail, HEVC targets use CPU libx265 and H.264 targets use CPU libx264. Main10 is
omitted when an actual backend sample cannot produce and validate it; the other
candidates continue. --crf and --preset are translated approximately for each
backend because their quality scales and speed controls are not numerically
equivalent, and 8-bit/10-bit quality mappings are explicitly distinct.
No hardware encoder is detected or initialized for --audio-only or --container.

Safe interruption behavior
--------------------------
Long encodes, direct remuxes, fallback remuxes, audio/container-only repairs, and both
bit-depths' uniquely named preflight samples write to tracked temporary paths.
Ctrl-C stops and waits
for the active FFmpeg child, removes every incomplete/unvalidated artifact—
including initial or extra sample
files—for the current input, preserves previously validated outputs, and does
not start another sample, encoder, remux, or fallback. Normal completion and
technical preflight failure also remove sample files immediately.
The accumulated batch summary is still printed, the interrupted input is listed
separately from failures, and the process exits non-zero (status 130).

Compatibility limitations
-------------------------
"Arbitrary input" means any non-DRM, non-corrupt video that the installed FFmpeg
build can demux and decode. Encrypted/DRM media, corrupt files, and unsupported
codecs or formats cannot be guaranteed. The Windows target is a conservative
default-player codec choice, not a promise that every PC smoothly decodes any
resolution or frame rate. LG support varies by model and model year. The Linux
target assumes capable software playback such as VLC. Hardware acceleration
depends on the OS, drivers, FFmpeg build, and installed hardware.
Container mode is additionally limited to unchanged video codecs that the
installed FFmpeg build can mux into MP4; a successful remux is not a playback
compatibility promise for the selected target.

External requirements
---------------------
- ffmpeg on PATH
- ffprobe on PATH
- hardware acceleration is optional; normal --gpu mode has a CPU fallback and
  --audio-only/--container initialize no video encoder

How to use
----------
1. Install FFmpeg and confirm that both `ffmpeg` and `ffprobe` are available on
   PATH.
2. Open a terminal in the directory containing this script, or invoke the script
   by its full path.
3. Pass the folder containing the source videos. With no other options, the
   iphone target is selected:

       python video_to_iphone_mp4.py ~/Videos

4. Select a different playback policy with --target. For example:

       python video_to_iphone_mp4.py ~/Videos --target windows

5. Add --gpu to try suitable hardware encoders. This is not a GPU-only mode: if
   no advertised hardware backend completes and validates, the script falls back
   to CPU libx264 or libx265 as documented above.
   To repair audio without ever encoding video, use:

       python video_to_iphone_mp4.py ~/Videos --target iphone --audio-only

   Unsafe source video is skipped. Add --convert-no-audio if silent, safely
   copyable video should be remuxed. --gpu/--crf/--preset do not affect video in
   this mode.
6. To copy any video stream that FFmpeg can mux into MP4, regardless of target
   video compatibility, use:

       python video_to_iphone_mp4.py ~/Videos --target iphone --container

   The target still controls audio safety. Subtitles are copied only when already
   mov_text; other subtitles are dropped. --container and --audio-only cannot be
   combined, and --gpu/--crf/--preset do not affect this mode.
7. Find normal/audio-only MP4 files under FOLDER/converted/TARGET/ and container
   outputs under FOLDER/converted/container/TARGET/. Source structure is mirrored
   in either namespace. Existing output paths are skipped.
8. Review the final summary for re-encoded size-reduced, size-increased, and
   size-unchanged groups; copied/remuxed video; existing-output and no-audio
   skips; failures; and any interrupted input.
9. For normal --target iphone operation, automatic bit-depth selection compares
   HEVC Main 8-bit and Main10 10-bit samples; safe source video also includes direct remux as a
   candidate. Bit-depth auto-selection needs no separate bit-depth flag. Pressing
   Ctrl-C safely cleans every current candidate sample/operation, preserves
   completed outputs, and prints the summary before exiting.

Command-line usage
------------------
python video_to_iphone_mp4.py FOLDER [options]

Options
-------
FOLDER
    Folder to probe recursively. Normal/audio-only outputs go under
    FOLDER/converted/TARGET/; container outputs use
    FOLDER/converted/container/TARGET/.

-h, --help
    Show argparse help and exit.

--target {iphone,windows,mac,lg-tv,linux}
    Playback compatibility target. Default: iphone.

--crf N
    User-facing quality target from 0 to 51. Lower means better quality and
    usually larger files. Hardware backends and iPhone bit depths use explicit,
    approximate mappings rather than assuming equal numeric quality.
    Default: 22. Accepted but unused with --audio-only or --container.

--preset PRESET
    Encoder speed/compression-efficiency setting. Choices: ultrafast,
    superfast, veryfast, faster, fast, medium, slow, slower, veryslow.
    Default: medium. Accepted but unused with --audio-only or --container.

--gpu
    Auto-detect suitable H.264 or HEVC hardware backends, validate each result,
    and fall back to libx264 or libx265 when necessary. No video backend is
    initialized by --audio-only or --container, where this option has no effect.

--no-subs
    Strip subtitles. Otherwise normal/audio-only modes convert compatible text
    subtitles to mov_text; container mode copies only existing mov_text.

--convert-no-audio
    Convert silent videos. Without it, they are skipped and reported separately.

--audio-only
    Produce a normal target MP4 while forbidding video encoding. Target-safe
    source video is copied; safe audio is copied and unsafe audio is repaired.
    Video requiring conversion is skipped. This is not audio extraction.

--container
    Copy any video bitstream that FFmpeg can mux into MP4, ignoring target video
    compatibility while retaining target-aware audio repair. Existing mov_text
    subtitles may be copied; other subtitle codecs are dropped. No video encoder
    or fallback is allowed. Mutually exclusive with --audio-only.

Examples
--------
python video_to_iphone_mp4.py ~/Videos
python video_to_iphone_mp4.py ~/Videos --target iphone
python video_to_iphone_mp4.py ~/Videos --target windows --preset slow
python video_to_iphone_mp4.py ~/Videos --target mac --gpu
python video_to_iphone_mp4.py ~/Videos --target lg-tv
python video_to_iphone_mp4.py ~/Videos --target linux --gpu
python video_to_iphone_mp4.py ~/Videos --target iphone --audio-only
python video_to_iphone_mp4.py ~/Videos --target windows --audio-only
python video_to_iphone_mp4.py ~/Videos --target iphone --container
python video_to_iphone_mp4.py ~/Videos --target windows --container
python video_to_iphone_mp4.py ~/Videos --crf 24 --no-subs
python video_to_iphone_mp4.py ~/Videos --container --convert-no-audio

The module header and argparse help describe the same CLI. Update both whenever
the command-line interface or conversion policy changes.
"""

import argparse
import json
import math
import platform
import shutil
import statistics
import subprocess
import sys
from collections import Counter
from dataclasses import dataclass
from pathlib import Path

H264_SAFE_PROFILES = ("Baseline", "Constrained Baseline", "Main", "High", "Constrained High")
HEVC_SAFE_PROFILES = ("Main",)
H264_SAFE_PIX_FMTS = ("yuv420p",)
HEVC_SAFE_PIX_FMTS = ("yuv420p",)
TEXT_SUBTITLE_CODECS = {"subrip", "srt", "mov_text", "webvtt", "text"}
MIN_PREDICTED_REENCODE_SAVINGS = 0.10
PREFLIGHT_SAMPLE_DURATION_SECONDS = 10.0
PREFLIGHT_MIN_SAMPLE_DURATION_SECONDS = 0.5
PREFLIGHT_INITIAL_POSITIONS = (0.10, 0.30, 0.50, 0.70, 0.90)
PREFLIGHT_EXTRA_POSITIONS = (0.20, 0.40, 0.60, 0.80)
PREFLIGHT_HIGH_VARIANCE_CV = 0.25
PREFLIGHT_LOW_VARIANCE_CV = 0.10
PREFLIGHT_MARGIN_LOW = 1.05
PREFLIGHT_MARGIN_NORMAL = 1.10
PREFLIGHT_MARGIN_HIGH = 1.15
PREFLIGHT_CONTAINER_OVERHEAD_FRACTION = 0.005
PREFLIGHT_CONTAINER_OVERHEAD_MIN_BYTES = 256 * 1024
PREFLIGHT_SUBTITLE_ESTIMATE_MARGIN = 1.25
VIDEO_METHOD_REENCODED = "reencoded"
VIDEO_METHOD_COPIED_REMUXED = "copied_remuxed"
IPHONE_SAFE_AUDIO_SAMPLE_RATES = frozenset({44_100, 48_000})
IPHONE_SAFE_AUDIO_LAYOUTS = {1: "mono", 2: "stereo"}
IPHONE_REPAIR_AUDIO_BITRATE = 192_000
IPHONE_REPAIR_AUDIO_SAMPLE_RATE = 48_000
# Ten-bit candidates receive a small quality bias rather than reusing an equal
# numeric CRF/CQ blindly. Lower is better for CRF/CQ/QP-style backends; higher
# is better for VideoToolbox's percentage-like q:v scale. This conservative
# mapping asks Main10 to match or slightly exceed the 8-bit quality target.
TEN_BIT_QUALITY_ADJUSTMENTS = {
    "software": -1,
    "nvenc": -1,
    "qsv": -1,
    "amf": -1,
    "vaapi": -1,
    "videotoolbox": 2,
}
PRESET_CHOICES = (
    "ultrafast", "superfast", "veryfast", "faster", "fast",
    "medium", "slow", "slower", "veryslow",
)


@dataclass(frozen=True)
class CodecPolicy:
    name: str
    display_name: str
    software_encoder: str
    encode_profile: str
    safe_profiles: tuple
    safe_pixel_formats: tuple
    output_tag: str
    valid_tags: tuple


@dataclass(frozen=True)
class VideoEncodeVariant:
    """One semantically distinct target video-encoding strategy."""

    name: str
    codec_name: str
    display_name: str
    profile: str
    pixel_format: str
    hardware_pixel_format: str
    bit_depth: int


HEVC_MAIN_8BIT = VideoEncodeVariant(
    name="hevc-main-8bit",
    codec_name="hevc",
    display_name="HEVC Main 8-bit",
    profile="main",
    pixel_format="yuv420p",
    hardware_pixel_format="nv12",
    bit_depth=8,
)
HEVC_MAIN10_10BIT = VideoEncodeVariant(
    name="hevc-main10-10bit",
    codec_name="hevc",
    display_name="HEVC Main10 10-bit",
    profile="main10",
    pixel_format="yuv420p10le",
    hardware_pixel_format="p010le",
    bit_depth=10,
)
H264_HIGH_8BIT = VideoEncodeVariant(
    name="h264-high-8bit",
    codec_name="h264",
    display_name="H.264 High 8-bit",
    profile="high",
    pixel_format="yuv420p",
    hardware_pixel_format="nv12",
    bit_depth=8,
)
IPHONE_ENCODE_VARIANTS = (HEVC_MAIN_8BIT, HEVC_MAIN10_10BIT)


CODECS = {
    "h264": CodecPolicy(
        name="h264",
        display_name="H.264 High",
        software_encoder="libx264",
        encode_profile="high",
        safe_profiles=H264_SAFE_PROFILES,
        safe_pixel_formats=H264_SAFE_PIX_FMTS,
        output_tag="avc1",
        valid_tags=("avc1",),
    ),
    "hevc": CodecPolicy(
        name="hevc",
        display_name="HEVC Main",
        software_encoder="libx265",
        encode_profile="main",
        safe_profiles=HEVC_SAFE_PROFILES,
        safe_pixel_formats=HEVC_SAFE_PIX_FMTS,
        output_tag="hvc1",
        valid_tags=("hvc1",),
    ),
}


@dataclass(frozen=True)
class TargetProfile:
    name: str
    video_codec: str
    allowed_remux_video_codecs: tuple
    safe_existing_video_combinations: tuple
    max_width: int = None
    max_height: int = None
    max_fps: float = None
    h264_level_limit: int = None
    hevc_level_limit: int = None
    encode_level: str = None
    valid_hevc_tags: tuple = ("hvc1",)
    requires_faststart: bool = True
    audio_codec: str = "aac"
    audio_profile: str = "LC"

    @property
    def codec(self):
        return CODECS[self.video_codec]

    def codec_policy(self, codec_name):
        return CODECS.get(codec_name)

    def valid_tags_for(self, codec_name):
        if codec_name == "hevc":
            return self.valid_hevc_tags
        policy = self.codec_policy(codec_name)
        return policy.valid_tags if policy else ()

    def level_limit_for(self, codec_name):
        if codec_name == "h264":
            return self.h264_level_limit
        if codec_name == "hevc":
            return self.hevc_level_limit
        return None


TARGETS = {
    "iphone": TargetProfile(
        name="iphone",
        video_codec="hevc",
        allowed_remux_video_codecs=("hevc", "h264"),
        safe_existing_video_combinations=(
            ("hevc", "Main", "yuv420p"),
            ("hevc", "Main 10", "yuv420p10le"),
            *(('h264', profile, 'yuv420p') for profile in H264_SAFE_PROFILES),
        ),
        max_width=3840,
        max_height=2160,
        max_fps=60.0,
        h264_level_limit=52,
        hevc_level_limit=153,
        encode_level="5.1",
        valid_hevc_tags=("hvc1",),
    ),
    "windows": TargetProfile(
        name="windows",
        video_codec="h264",
        allowed_remux_video_codecs=("h264",),
        safe_existing_video_combinations=tuple(
            ("h264", profile, "yuv420p") for profile in H264_SAFE_PROFILES
        ),
        h264_level_limit=62,
    ),
    "mac": TargetProfile(
        name="mac",
        video_codec="hevc",
        allowed_remux_video_codecs=("hevc",),
        safe_existing_video_combinations=(("hevc", "Main", "yuv420p"),),
        hevc_level_limit=186,
        valid_hevc_tags=("hvc1",),
    ),
    "lg-tv": TargetProfile(
        name="lg-tv",
        video_codec="h264",
        allowed_remux_video_codecs=("h264",),
        safe_existing_video_combinations=tuple(
            ("h264", profile, "yuv420p") for profile in H264_SAFE_PROFILES
        ),
        h264_level_limit=62,
    ),
    "linux": TargetProfile(
        name="linux",
        video_codec="hevc",
        allowed_remux_video_codecs=("hevc",),
        safe_existing_video_combinations=(("hevc", "Main", "yuv420p"),),
        hevc_level_limit=186,
        valid_hevc_tags=("hvc1", "hev1"),
    ),
}


@dataclass(frozen=True)
class EncoderCandidate:
    name: str
    backend: str
    device: str = None


@dataclass(frozen=True)
class EncodePlan:
    """An encoder backend paired with the exact sampled encode variant."""

    encoder: EncoderCandidate
    variant: VideoEncodeVariant


@dataclass(frozen=True)
class PreflightDecision:
    should_reencode: bool
    reason: str
    predicted_savings: float = None
    encoder_candidate: EncoderCandidate = None
    encode_variant: VideoEncodeVariant = None
    fallback_plans: tuple = ()
    variant_estimates: tuple = ()
    source_video_bitrate: float = None
    encoded_video_bitrate: float = None
    conservative_video_bitrate: float = None
    predicted_direct_bytes: float = None
    predicted_encode_bytes: float = None
    sample_count: int = None
    coefficient_of_variation: float = None
    estimate_margin: float = None


@dataclass(frozen=True)
class PreflightEstimate:
    predicted_savings: float
    encoder_candidate: EncoderCandidate
    encode_variant: VideoEncodeVariant
    source_video_bitrate: float
    encoded_video_bitrate: float
    conservative_video_bitrate: float
    predicted_direct_bytes: float
    predicted_encode_bytes: float
    sample_count: int
    coefficient_of_variation: float
    estimate_margin: float


class ActivePaths:
    """Track incomplete or unvalidated artifacts for interruption cleanup."""

    def __init__(self):
        self._paths = set()

    def register(self, path: Path):
        self._paths.add(path)

    def complete(self, path: Path):
        self._paths.discard(path)

    def discard(self, path: Path):
        safe_unlink(path)
        self.complete(path)

    def cleanup(self):
        for path in tuple(self._paths):
            safe_unlink(path)
            self._paths.discard(path)


def safe_unlink(path: Path):
    try:
        path.unlink()
    except FileNotFoundError:
        pass


def run_probe(path: Path, *, select_streams=None, show_format=False):
    """Return parsed ffprobe JSON, or None if probing failed."""
    cmd = ["ffprobe", "-v", "error", "-print_format", "json", "-show_streams"]
    if show_format:
        cmd.append("-show_format")
    if select_streams:
        cmd += ["-select_streams", select_streams]
    cmd.append(str(path))
    result = subprocess.run(cmd, capture_output=True, check=False)
    if result.returncode != 0:
        return None
    try:
        return json.loads(result.stdout.decode(errors="replace"))
    except json.JSONDecodeError:
        return None


def probe_streams(path: Path, stream_type: str):
    data = run_probe(path, select_streams=stream_type)
    return data.get("streams", []) if data else []


def get_duration_seconds(path: Path):
    cmd = [
        "ffprobe", "-v", "error", "-show_entries", "format=duration",
        "-of", "csv=p=0", str(path),
    ]
    result = subprocess.run(cmd, capture_output=True, check=False)
    if result.returncode != 0:
        return None
    try:
        value = float(result.stdout.decode(errors="replace").strip())
        return value if math.isfinite(value) and value > 0 else None
    except ValueError:
        return None


def parse_fraction(value):
    if not value or value in {"0/0", "N/A"}:
        return None
    try:
        if "/" in str(value):
            numerator, denominator = str(value).split("/", 1)
            denominator = float(denominator)
            if denominator == 0:
                return None
            return float(numerator) / denominator
        return float(value)
    except (TypeError, ValueError, ZeroDivisionError):
        return None


def stream_fps(stream):
    return parse_fraction(stream.get("avg_frame_rate")) or parse_fraction(stream.get("r_frame_rate"))


def stream_level(stream):
    try:
        level = int(stream.get("level"))
        return level if level > 0 else None
    except (TypeError, ValueError):
        return None


def parse_time_str(time_str: str):
    try:
        hours, minutes, seconds = time_str.split(":")
        return int(hours) * 3600 + int(minutes) * 60 + float(seconds)
    except (ValueError, AttributeError):
        return None


def format_hms(seconds: float) -> str:
    seconds = max(0, int(seconds))
    hours, remainder = divmod(seconds, 3600)
    minutes, seconds = divmod(remainder, 60)
    if hours:
        return f"{hours}:{minutes:02d}:{seconds:02d}"
    return f"{minutes}:{seconds:02d}"


def stop_process(proc):
    """Terminate an FFmpeg child without leaving it running in the background."""
    if proc.poll() is not None:
        return
    proc.terminate()
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()


def run_ffmpeg_with_progress(cmd, duration, label, active_path=None, active_paths=None):
    """Run FFmpeg, propagate Ctrl-C, and keep incomplete output tracked."""
    sys.stdout.flush()
    if active_path is not None and active_paths is not None:
        active_paths.register(active_path)
    proc = subprocess.Popen(
        cmd, stdout=subprocess.PIPE, stderr=sys.stderr, text=True, bufsize=1,
    )
    current_time = 0.0
    speed = None
    try:
        assert proc.stdout is not None
        for line in proc.stdout:
            line = line.strip()
            if "=" not in line:
                continue
            key, _, value = line.partition("=")
            if key == "out_time":
                parsed = parse_time_str(value)
                if parsed is not None:
                    current_time = parsed
            elif key == "speed":
                speed = value.strip().rstrip("x")
            if key != "progress":
                continue
            if duration and duration > 0:
                percent = min(100.0, 100.0 * current_time / duration)
                remaining = max(0.0, duration - current_time)
                try:
                    numeric_speed = float(speed) if speed else None
                except ValueError:
                    numeric_speed = None
                eta = remaining / numeric_speed if numeric_speed and numeric_speed > 0 else None
                eta_text = format_hms(eta) if eta is not None else "?"
                bar_length = 30
                filled = int(bar_length * percent / 100)
                bar = "#" * filled + "-" * (bar_length - filled)
                print(
                    f"\r  [{label}] [{bar}] {percent:5.1f}%  "
                    f"{format_hms(current_time)}/{format_hms(duration)}  "
                    f"speed={speed or '?'}x  ETA={eta_text}   ",
                    end="", flush=True,
                )
            else:
                print(
                    f"\r  [{label}] {format_hms(current_time)} elapsed  "
                    f"speed={speed or '?'}x   ",
                    end="", flush=True,
                )
        print()
        proc.wait()
        return proc.returncode == 0
    except KeyboardInterrupt:
        print("\n  Interrupted; stopping the active FFmpeg process...", flush=True)
        stop_process(proc)
        raise
    finally:
        if proc.stdout is not None:
            proc.stdout.close()


def contains_video(path: Path) -> bool:
    return bool(probe_streams(path, "v:0"))


def discover_video_files(folder: Path, converted_root: Path):
    """Discover video by file contents and exclude every target output tree."""
    candidates = [
        path for path in folder.rglob("*")
        if path.is_file() and converted_root not in path.parents
    ]
    print(f"Scanning {len(candidates)} file(s) for video streams...")
    return [path for path in sorted(candidates) if contains_video(path)]


def build_subtitle_args(input_path: Path, keep_subs: bool):
    if not keep_subs:
        return []
    subtitle_streams = probe_streams(input_path, "s")
    compatible_indices = []
    skipped = set()
    for subtitle_index, stream in enumerate(subtitle_streams):
        codec = stream.get("codec_name", "")
        if codec in TEXT_SUBTITLE_CODECS:
            compatible_indices.append(subtitle_index)
        else:
            skipped.add(codec or "unknown")
    if skipped:
        print(
            f"  Note: skipping subtitle codec(s) {sorted(skipped)} because they "
            "cannot be preserved reliably as MP4 text subtitles."
        )
    args = []
    for subtitle_index in compatible_indices:
        args += ["-map", f"0:s:{subtitle_index}"]
    if compatible_indices:
        args += ["-c:s", "mov_text"]
    return args


def build_container_subtitle_args(input_path: Path, keep_subs: bool):
    """Copy existing MP4 text subtitles; never transcode in container mode."""
    if not keep_subs:
        return []
    subtitle_streams = probe_streams(input_path, "s")
    compatible_indices = [
        index for index, stream in enumerate(subtitle_streams)
        if stream.get("codec_name") == "mov_text"
    ]
    skipped = sorted({
        stream.get("codec_name") or "unknown"
        for stream in subtitle_streams
        if stream.get("codec_name") != "mov_text"
    })
    if skipped:
        print(
            f"  Note: dropping subtitle codec(s) {skipped} in --container "
            "because subtitle conversion is disabled."
        )
    args = []
    for subtitle_index in compatible_indices:
        args += ["-map", f"0:s:{subtitle_index}"]
    if compatible_indices:
        args += ["-c:s", "copy"]
    return args


def positive_int(value):
    try:
        parsed = int(value)
        return parsed if parsed > 0 else None
    except (TypeError, ValueError):
        return None


def audio_is_safe_for_target(stream, target_profile: TargetProfile):
    """Return whether one source/output audio stream satisfies target policy."""
    profile = (stream.get("profile") or "").upper()
    channels = positive_int(stream.get("channels"))
    if stream.get("codec_name") != target_profile.audio_codec:
        return False
    if profile != target_profile.audio_profile.upper():
        return False
    if target_profile.name != "iphone":
        return channels is not None and channels <= 6

    expected_layout = IPHONE_SAFE_AUDIO_LAYOUTS.get(channels)
    channel_layout = (stream.get("channel_layout") or "").lower()
    sample_rate = positive_int(stream.get("sample_rate"))
    return (
        expected_layout is not None
        and channel_layout == expected_layout
        and sample_rate in IPHONE_SAFE_AUDIO_SAMPLE_RATES
    )


def audio_transcode_settings(stream, target_profile: TargetProfile):
    """Return bitrate/channels/sample-rate for one unsafe target audio stream."""
    channels = positive_int(stream.get("channels")) or 2
    if target_profile.name == "iphone":
        return IPHONE_REPAIR_AUDIO_BITRATE, 2, IPHONE_REPAIR_AUDIO_SAMPLE_RATE
    if channels > 6:
        return 192_000, 2, 48_000
    return (384_000 if channels > 2 else 192_000), None, 48_000


def audio_stream_description(stream):
    return (
        f"{stream.get('codec_name', 'unknown')}/"
        f"{stream.get('profile') or 'unknown profile'}/"
        f"{stream.get('channel_layout') or 'unknown layout'}"
    )


def build_audio_args(input_path: Path, target_profile: TargetProfile):
    """Map every audio stream and apply the shared target compatibility policy."""
    audio_streams = probe_streams(input_path, "a")
    if not audio_streams:
        return ["-map", "0:a?"], None
    if len(audio_streams) == 1:
        stream = audio_streams[0]
        if audio_is_safe_for_target(stream, target_profile):
            return ["-map", "0:a:0", "-c:a", "copy"], None
        bitrate, channels, sample_rate = audio_transcode_settings(
            stream, target_profile,
        )
        args = [
            "-map", "0:a:0", "-c:a", "aac", "-profile:a", "aac_low",
            "-b:a", f"{bitrate // 1000}k", "-ar:a", str(sample_rate),
        ]
        if channels is not None:
            args += ["-ac:a", str(channels)]
        repair = "AAC-LC"
        if target_profile.name == "iphone":
            repair += " stereo 192k/48kHz"
        note = f"audio {[audio_stream_description(stream)]} -> {repair}"
        return args, note

    args = []
    unsafe_descriptions = []
    for audio_index, stream in enumerate(audio_streams):
        args += ["-map", f"0:a:{audio_index}"]
        output_selector = f":{audio_index}"
        if audio_is_safe_for_target(stream, target_profile):
            args += [f"-c:a{output_selector}", "copy"]
            continue

        bitrate, channels, sample_rate = audio_transcode_settings(
            stream, target_profile,
        )
        args += [
            f"-c:a{output_selector}", "aac",
            f"-profile:a{output_selector}", "aac_low",
            f"-b:a{output_selector}", f"{bitrate // 1000}k",
            f"-ar:a{output_selector}", str(sample_rate),
        ]
        if channels is not None:
            args += [f"-ac:a{output_selector}", str(channels)]
        unsafe_descriptions.append(audio_stream_description(stream))

    if not unsafe_descriptions:
        return args, None
    repair = "AAC-LC"
    if target_profile.name == "iphone":
        repair += " stereo 192k/48kHz"
    note = f"audio {sorted(set(unsafe_descriptions))} -> {repair}"
    return args, note


def default_encode_variant(target_profile: TargetProfile):
    """Return the single normal variant used outside iPhone auto selection."""
    if target_profile.video_codec == "h264":
        return H264_HIGH_8BIT
    return HEVC_MAIN_8BIT


def encode_variants_for_target(target_profile: TargetProfile):
    if target_profile.name == "iphone":
        return IPHONE_ENCODE_VARIANTS
    return (default_encode_variant(target_profile),)


def backend_can_attempt_variant(candidate: EncoderCandidate,
                                variant: VideoEncodeVariant):
    """Filter structurally impossible pairs; samples prove actual usability."""
    expected_software = CODECS[variant.codec_name].software_encoder
    if (variant.codec_name not in candidate.name and
            candidate.name != expected_software):
        return False
    if variant.bit_depth == 8:
        return True
    return (
        variant.codec_name == "hevc" and
        candidate.backend in {
            "software", "nvenc", "qsv", "amf", "videotoolbox", "vaapi",
        }
    )


def build_video_filter(target_profile: TargetProfile, *, vaapi=False,
                       variant=None):
    """Build target-aware scaling and a variant-aware VAAPI upload path."""
    variant = variant or default_encode_variant(target_profile)
    if target_profile.max_width and target_profile.max_height:
        video_filter = (
            "scale="
            f"w='min(iw,{target_profile.max_width})':"
            f"h='min(ih,{target_profile.max_height})':"
            "force_original_aspect_ratio=decrease:force_divisible_by=2"
        )
    else:
        video_filter = "scale=w='trunc(iw/2)*2':h='trunc(ih/2)*2'"
    if vaapi:
        video_filter += f",format={variant.hardware_pixel_format},hwupload"
    return video_filter


def nvenc_preset_for(preset: str) -> str:
    mapping = {
        "ultrafast": "p1", "superfast": "p1", "veryfast": "p2",
        "faster": "p3", "fast": "p3", "medium": "p4",
        "slow": "p5", "slower": "p6", "veryslow": "p7",
    }
    return mapping.get(preset, "p4")


def qsv_preset_for(preset: str) -> str:
    mapping = {
        "ultrafast": "veryfast", "superfast": "veryfast", "veryfast": "veryfast",
        "faster": "faster", "fast": "fast", "medium": "medium",
        "slow": "slow", "slower": "slower", "veryslow": "veryslow",
    }
    return mapping.get(preset, "medium")


def amf_quality_for(preset: str) -> str:
    if preset in {"ultrafast", "superfast", "veryfast", "faster", "fast"}:
        return "speed"
    if preset in {"slow", "slower", "veryslow"}:
        return "quality"
    return "balanced"


def mapped_quality(crf: int, backend: str, variant=None) -> int:
    """Map the user quality dial, including a conservative Main10 bias."""
    variant = variant or HEVC_MAIN_8BIT
    crf = max(0, min(51, crf))
    if backend == "videotoolbox":
        base = max(1, min(100, round(100 - crf * 100 / 51)))
        adjustment = TEN_BIT_QUALITY_ADJUSTMENTS[backend] if variant.bit_depth == 10 else 0
        return max(1, min(100, base + adjustment))
    base = max(0, min(51, crf))
    adjustment = TEN_BIT_QUALITY_ADJUSTMENTS.get(backend, 0) if variant.bit_depth == 10 else 0
    return max(0, min(51, base + adjustment))


def available_ffmpeg_encoders():
    result = subprocess.run(
        ["ffmpeg", "-hide_banner", "-encoders"],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=False,
    )
    if result.returncode != 0:
        return set()
    encoders = set()
    for line in result.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 2 and parts[0].startswith("V"):
            encoders.add(parts[1])
    return encoders


def hardware_hints():
    """Best-effort vendor hints used only for ordering, never as proof."""
    hints = set()
    system = platform.system().lower()
    text = ""
    commands = []
    if system == "linux" and shutil.which("lspci"):
        commands.append(["lspci"])
    elif system == "windows" and shutil.which("wmic"):
        commands.append(["wmic", "path", "win32_VideoController", "get", "name"])
    elif system == "darwin" and shutil.which("system_profiler"):
        commands.append(["system_profiler", "SPDisplaysDataType"])
    for command in commands:
        try:
            result = subprocess.run(
                command, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                text=True, timeout=5, check=False,
            )
            text += result.stdout.lower()
        except (OSError, subprocess.TimeoutExpired):
            pass
    if "nvidia" in text or Path("/dev/nvidiactl").exists():
        hints.add("nvenc")
    if any(word in text for word in ("amd", "advanced micro devices", "radeon")):
        hints.add("amf")
        hints.add("vaapi")
    if "intel" in text:
        hints.add("qsv")
        hints.add("vaapi")
    if system == "darwin":
        hints.add("videotoolbox")
    return hints


def hardware_encoder_candidates(target_profile: TargetProfile, available=None, hints=None):
    """Return listed, platform-appropriate candidates in a hardware-aware order."""
    available = available if available is not None else available_ffmpeg_encoders()
    hints = hints if hints is not None else hardware_hints()
    codec = target_profile.video_codec
    system = platform.system().lower()
    names = {
        "nvenc": f"{codec}_nvenc",
        "amf": f"{codec}_amf",
        "qsv": f"{codec}_qsv",
        "videotoolbox": f"{codec}_videotoolbox",
        "vaapi": f"{codec}_vaapi",
    }
    if system == "darwin":
        platform_order = ["videotoolbox", "nvenc", "qsv"]
    elif system == "windows":
        platform_order = ["nvenc", "amf", "qsv"]
    else:
        platform_order = ["nvenc", "qsv", "amf", "vaapi"]
    ordered_backends = [backend for backend in platform_order if backend in hints]
    ordered_backends += [backend for backend in platform_order if backend not in ordered_backends]

    vaapi_devices = sorted(Path("/dev/dri").glob("renderD*")) if Path("/dev/dri").is_dir() else []
    candidates = []
    for backend in ordered_backends:
        name = names[backend]
        if name not in available:
            continue
        if backend == "vaapi":
            if not vaapi_devices:
                continue
            candidates.append(EncoderCandidate(name, backend, str(vaapi_devices[0])))
        else:
            candidates.append(EncoderCandidate(name, backend))
    return candidates


def encoder_video_args(target_profile: TargetProfile, candidate: EncoderCandidate,
                       crf: int, preset: str, variant=None):
    """Return (pre-input args, target-aware encoder args) for one backend."""
    variant = variant or default_encode_variant(target_profile)
    if variant.codec_name != target_profile.video_codec:
        raise ValueError(
            f"Variant {variant.name} does not match target codec "
            f"{target_profile.video_codec}"
        )
    codec = target_profile.codec
    vaapi = candidate.backend == "vaapi"
    pre_input = ["-vaapi_device", candidate.device] if vaapi else []
    common = [
        "-vf", build_video_filter(
            target_profile, vaapi=vaapi, variant=variant,
        ),
        "-profile:v", variant.profile,
        "-tag:v", codec.output_tag,
    ]
    if not vaapi:
        pixel_format = (
            variant.pixel_format
            if candidate.backend == "software"
            else variant.hardware_pixel_format
        )
        common += ["-pix_fmt", pixel_format]
    if target_profile.max_fps is not None:
        common += ["-fpsmax", str(int(target_profile.max_fps))]
    if target_profile.encode_level is not None:
        common += ["-level:v", target_profile.encode_level]

    quality = mapped_quality(crf, candidate.backend, variant)
    if candidate.backend == "software":
        # H.264 High cannot signal x264's lossless CRF 0 mode, so clamp that
        # one endpoint to 1 while keeping the shared 0-51 user-facing dial.
        software_crf = max(1, quality) if target_profile.video_codec == "h264" else quality
        specific = [
            "-c:v", codec.software_encoder, "-crf", str(software_crf),
            "-preset", preset,
        ]
        # libx264's fastest presets disable every High-profile-only tool and can
        # consequently signal Baseline despite -profile:v high. Retain two High
        # tools so the encoded Windows/LG stream actually signals High.
        if target_profile.video_codec == "h264":
            specific += ["-x264-params", "cabac=1:8x8dct=1"]
    elif candidate.backend == "nvenc":
        specific = [
            "-c:v", candidate.name, "-preset", nvenc_preset_for(preset),
            "-rc", "vbr", "-cq", str(quality), "-b:v", "0",
        ]
    elif candidate.backend == "qsv":
        specific = [
            "-c:v", candidate.name, "-preset", qsv_preset_for(preset),
            "-global_quality", str(quality),
        ]
    elif candidate.backend == "amf":
        specific = [
            "-c:v", candidate.name, "-quality", amf_quality_for(preset),
            "-rc", "cqp", "-qp_i", str(quality), "-qp_p", str(quality),
        ]
    elif candidate.backend == "videotoolbox":
        specific = ["-c:v", candidate.name, "-q:v", str(quality), "-realtime", "false"]
    elif candidate.backend == "vaapi":
        specific = ["-c:v", candidate.name, "-global_quality", str(quality)]
    else:
        raise ValueError(f"Unknown encoder backend: {candidate.backend}")
    return pre_input, [*specific, *common]


def try_reencode(input_path: Path, out_path: Path, target_profile: TargetProfile,
                 candidate: EncoderCandidate, crf: int, preset: str,
                 keep_subs: bool, duration, active_paths: ActivePaths,
                 variant=None):
    variant = variant or default_encode_variant(target_profile)
    audio_args, audio_note = build_audio_args(input_path, target_profile)
    if audio_note:
        print(f"  Note: {audio_note}")
    pre_input, video_args = encoder_video_args(
        target_profile, candidate, crf, preset, variant,
    )
    cmd = [
        "ffmpeg", "-y", *pre_input, "-i", str(input_path),
        "-loglevel", "warning", "-progress", "pipe:1", "-nostats",
        "-map", "0:v:0", *audio_args, *video_args,
        *build_subtitle_args(input_path, keep_subs),
        "-max_muxing_queue_size", "4096", "-avoid_negative_ts", "make_zero",
        "-movflags", "+faststart", str(out_path),
    ]
    backend_text = (
        f"CPU/{candidate.name} preset {preset}"
        if candidate.backend == "software"
        else f"hardware/{candidate.name} ({candidate.backend})"
    )
    print(
        f"Converting: {input_path.name} -> {out_path} "
        f"({variant.display_name}, quality {crf}, {backend_text})"
    )
    if duration:
        print(f"  Duration: {format_hms(duration)}")
    return run_ffmpeg_with_progress(
        cmd, duration, "encode", active_path=out_path,
        active_paths=active_paths,
    )


def video_compatibility_reasons(stream, target_profile: TargetProfile, *, remux=False):
    """Return target-policy violations for a probed video stream."""
    reasons = []
    codec_name = stream.get("codec_name", "")
    codec_policy = target_profile.codec_policy(codec_name)
    allowed_codecs = target_profile.allowed_remux_video_codecs
    if codec_name not in allowed_codecs or not codec_policy:
        expected = "/".join(target_profile.allowed_remux_video_codecs)
        return [f"video codec {codec_name or 'unknown'} is not permitted (expected {expected})"]

    profile = stream.get("profile", "")
    pixel_format = stream.get("pix_fmt", "")
    width = int(stream.get("width") or 0)
    height = int(stream.get("height") or 0)
    fps = stream_fps(stream)
    level = stream_level(stream)
    tag = (stream.get("codec_tag_string") or "").lower()

    combination = (codec_name, profile, pixel_format)
    if combination not in target_profile.safe_existing_video_combinations:
        reasons.append(
            f"{codec_policy.display_name} profile/pixel-format combination "
            f"{profile or 'unknown'}/{pixel_format or 'unknown'} is not safe"
        )
    if width <= 0 or height <= 0:
        reasons.append("unknown video dimensions")
    elif ((target_profile.max_width and width > target_profile.max_width) or
          (target_profile.max_height and height > target_profile.max_height)):
        reasons.append(
            f"resolution {width}x{height} exceeds the "
            f"{target_profile.max_width}x{target_profile.max_height} target"
        )
    if target_profile.max_fps is not None and fps is None:
        reasons.append("unknown frame rate")
    elif target_profile.max_fps is not None and fps > target_profile.max_fps + 0.01:
        reasons.append(f"frame rate {fps:.2f} fps exceeds {target_profile.max_fps:g} fps")
    level_limit = target_profile.level_limit_for(codec_name)
    if target_profile.name == "iphone" and level_limit is not None and level is None:
        reasons.append(f"{codec_policy.display_name} level is unknown")
    elif level_limit is not None and level is not None and level > level_limit:
        reasons.append(
            f"{codec_policy.display_name} level {level} exceeds target ceiling {level_limit}"
        )

    # A remux command explicitly writes the target tag. For an already-produced
    # candidate, the promised sample entry must actually be present.
    if not remux and tag not in target_profile.valid_tags_for(codec_name):
        expected_tags = "/".join(target_profile.valid_tags_for(codec_name))
        reasons.append(f"sample entry is {tag or 'unknown'}, expected {expected_tags}")
    return reasons


def can_safely_remux_for_target(input_path: Path, target_profile: TargetProfile):
    streams = probe_streams(input_path, "v:0")
    if not streams:
        return False, "no video stream"
    reasons = video_compatibility_reasons(streams[0], target_profile, remux=True)
    if reasons:
        return False, "; ".join(reasons)
    return True, "safe to stream-copy"


def try_remux(input_path: Path, out_path: Path, target_profile: TargetProfile,
              keep_subs: bool, duration, active_paths: ActivePaths,
              *, direct=False):
    safe, reason = can_safely_remux_for_target(input_path, target_profile)
    if not safe:
        print(f"  Remux not used: {reason}.")
        return False
    video_stream = probe_streams(input_path, "v:0")[0]
    codec_policy = target_profile.codec_policy(video_stream.get("codec_name", ""))
    audio_args, audio_note = build_audio_args(input_path, target_profile)
    if audio_note:
        print(f"  Note: {audio_note}")
    cmd = [
        "ffmpeg", "-y", "-i", str(input_path),
        "-loglevel", "warning", "-progress", "pipe:1", "-nostats",
        "-map", "0:v:0", *audio_args, "-c:v", "copy",
        "-tag:v", codec_policy.output_tag,
        *build_subtitle_args(input_path, keep_subs),
        "-max_muxing_queue_size", "4096", "-avoid_negative_ts", "make_zero",
        "-movflags", "+faststart", str(out_path),
    ]
    if direct:
        print("  Trying direct remux (video copied without generation loss)...")
        progress_label = "direct-remux"
    else:
        print("  Trying safe remux candidate (video copied without quality loss)...")
        progress_label = "remux"
    return run_ffmpeg_with_progress(
        cmd, duration, progress_label, active_path=out_path,
        active_paths=active_paths,
    )


def copied_video_tag_args(video_stream):
    """Return conservative MP4 sample-entry arguments for known video codecs."""
    codec_policy = CODECS.get(video_stream.get("codec_name", ""))
    return ["-tag:v", codec_policy.output_tag] if codec_policy else []


def try_container_remux(input_path: Path, out_path: Path,
                        target_profile: TargetProfile, keep_subs: bool,
                        duration, active_paths: ActivePaths):
    """Attempt an MP4 remux without applying target video compatibility rules."""
    video_streams = probe_streams(input_path, "v:0")
    if not video_streams:
        return False
    audio_args, audio_note = build_audio_args(input_path, target_profile)
    if audio_note:
        print(f"  Note: {audio_note}")
    cmd = [
        "ffmpeg", "-y", "-i", str(input_path),
        "-loglevel", "warning", "-progress", "pipe:1", "-nostats",
        "-map", "0:v:0", *audio_args, "-c:v", "copy",
        *copied_video_tag_args(video_streams[0]),
        *build_container_subtitle_args(input_path, keep_subs),
        "-max_muxing_queue_size", "4096", "-avoid_negative_ts", "make_zero",
        "-movflags", "+faststart", str(out_path),
    ]
    print("  Trying MP4 container remux (video bitstream copied unchanged)...")
    return run_ffmpeg_with_progress(
        cmd, duration, "container-remux", active_path=out_path,
        active_paths=active_paths,
    )


def audio_validation_reasons(audio_streams, target_profile: TargetProfile):
    reasons = []
    for stream in audio_streams:
        if not audio_is_safe_for_target(stream, target_profile):
            if target_profile.name == "iphone":
                expected_audio = (
                    "AAC-LC mono/stereo with mono/stereo layout at 44.1/48 kHz"
                )
            else:
                expected_audio = "AAC-LC with no more than 5.1 channels"
            reasons.append(
                f"audio stream {stream.get('index', '?')} is not target-safe "
                f"({audio_stream_description(stream)}; expected {expected_audio})"
            )
    return reasons


def subtitle_validation_reasons(subtitle_streams):
    return [
        f"subtitle stream {stream.get('index', '?')} is "
        f"{stream.get('codec_name', 'unknown')}, expected mov_text"
        for stream in subtitle_streams
        if stream.get("codec_name") != "mov_text"
    ]


def validate_output(path: Path, target_profile: TargetProfile):
    """Return (valid, reasons) for the promises made by a target profile."""
    data = run_probe(path, show_format=True)
    if not data:
        return False, ["ffprobe could not read the output"]
    reasons = []
    format_name = (data.get("format") or {}).get("format_name", "")
    if "mp4" not in format_name and "mov" not in format_name:
        reasons.append(f"unexpected container '{format_name or 'unknown'}'")
    streams = data.get("streams", [])
    video_streams = [stream for stream in streams if stream.get("codec_type") == "video"]
    audio_streams = [stream for stream in streams if stream.get("codec_type") == "audio"]
    subtitle_streams = [stream for stream in streams if stream.get("codec_type") == "subtitle"]
    if not video_streams:
        reasons.append("no video stream")
    else:
        reasons.extend(video_compatibility_reasons(video_streams[0], target_profile, remux=False))
    reasons.extend(audio_validation_reasons(audio_streams, target_profile))
    reasons.extend(subtitle_validation_reasons(subtitle_streams))
    return not reasons, reasons


def validate_container_output(path: Path, input_path: Path,
                              target_profile: TargetProfile):
    """Validate MP4 muxing, unchanged video codec, target audio, and subtitles."""
    source_video = probe_streams(input_path, "v:0")
    data = run_probe(path, show_format=True)
    if not data:
        return False, ["ffprobe could not read the output"]
    reasons = []
    format_name = (data.get("format") or {}).get("format_name", "")
    if "mp4" not in format_name and "mov" not in format_name:
        reasons.append(f"unexpected container '{format_name or 'unknown'}'")
    streams = data.get("streams", [])
    video_streams = [stream for stream in streams if stream.get("codec_type") == "video"]
    audio_streams = [stream for stream in streams if stream.get("codec_type") == "audio"]
    subtitle_streams = [stream for stream in streams if stream.get("codec_type") == "subtitle"]
    if not source_video:
        reasons.append("source has no video stream")
    if not video_streams:
        reasons.append("output has no video stream")
    elif source_video:
        source_codec = source_video[0].get("codec_name", "")
        output_codec = video_streams[0].get("codec_name", "")
        if output_codec != source_codec:
            reasons.append(
                f"video codec changed from {source_codec or 'unknown'} to "
                f"{output_codec or 'unknown'}"
            )
        expected_tag = CODECS.get(source_codec)
        if expected_tag:
            actual_tag = (video_streams[0].get("codec_tag_string") or "").lower()
            if actual_tag != expected_tag.output_tag:
                reasons.append(
                    f"sample entry is {actual_tag or 'unknown'}, expected "
                    f"{expected_tag.output_tag}"
                )
    reasons.extend(audio_validation_reasons(audio_streams, target_profile))
    reasons.extend(subtitle_validation_reasons(subtitle_streams))
    return not reasons, reasons


def validated_candidate(path: Path, label: str, target_profile: TargetProfile):
    if not path.exists() or path.stat().st_size <= 0:
        print(f"  {label} candidate is missing or empty.")
        return False
    valid, reasons = validate_output(path, target_profile)
    if not valid:
        print(f"  {label} candidate failed {target_profile.name} validation:")
        for reason in reasons:
            print(f"    - {reason}")
        return False
    return True


def validated_container_candidate(path: Path, input_path: Path, label: str,
                                  target_profile: TargetProfile):
    if not path.exists() or path.stat().st_size <= 0:
        print(f"  {label} candidate is missing or empty.")
        return False
    valid, reasons = validate_container_output(path, input_path, target_profile)
    if not valid:
        print(f"  {label} candidate failed container validation:")
        for reason in reasons:
            print(f"    - {reason}")
        return False
    return True


def temp_remux_path(out_path: Path):
    return out_path.with_name(f".{out_path.stem}.remux-tmp.mp4")


def temp_direct_remux_path(out_path: Path):
    return out_path.with_name(f".{out_path.stem}.direct-remux-tmp.mp4")


def temp_audio_only_path(out_path: Path):
    return out_path.with_name(f".{out_path.stem}.audio-only-tmp.mp4")


def temp_container_path(out_path: Path):
    return out_path.with_name(f".{out_path.stem}.container-tmp.mp4")


def temp_encode_path(out_path: Path):
    return out_path.with_name(f".{out_path.stem}.encode-tmp.mp4")


def temp_preflight_path(out_path: Path, candidate: EncoderCandidate,
                        variant: VideoEncodeVariant, position: float):
    position_percent = round(position * 100)
    return out_path.with_name(
        f".{out_path.stem}.preflight-{candidate.backend}-{variant.bit_depth}bit-"
        f"{position_percent}-tmp.mp4"
    )


def parsed_positive_number(value):
    try:
        number = float(value)
        return number if math.isfinite(number) and number > 0 else None
    except (TypeError, ValueError):
        return None


def reported_stream_bitrate(stream):
    """Return stream bitrate from ffprobe fields/tags when available."""
    bitrate = parsed_positive_number(stream.get("bit_rate"))
    if bitrate:
        return bitrate
    tags = stream.get("tags") or {}
    normalized_tags = {key.upper(): value for key, value in tags.items()}
    for key in ("BPS", "BPS-ENG"):
        bitrate = parsed_positive_number(normalized_tags.get(key))
        if bitrate:
            return bitrate
    return None


def estimate_source_video_bitrate(input_path: Path, video_stream, duration):
    """Estimate source video bitrate without using total container bitrate."""
    bitrate = reported_stream_bitrate(video_stream)
    total_container_bitrate = (
        input_path.stat().st_size * 8 / duration if duration else None
    )
    if (bitrate and total_container_bitrate and
            bitrate <= total_container_bitrate * 1.05):
        return bitrate
    if not duration:
        return None
    payload_bytes = packet_payload_bytes(input_path, "v:0")
    return payload_bytes * 8 / duration if payload_bytes else None


def packet_payload_bytes(path: Path, stream_selector: str):
    """Sum packet payload bytes for selected streams without decoding them."""
    cmd = [
        "ffprobe", "-v", "error", "-select_streams", stream_selector,
        "-show_entries", "packet=size", "-of", "csv=p=0", str(path),
    ]
    result = subprocess.run(cmd, capture_output=True, text=True, check=False)
    if result.returncode != 0:
        return None
    total = 0
    found = False
    for line in result.stdout.splitlines():
        value = line.strip().split(",", 1)[0]
        try:
            size = int(value)
        except ValueError:
            continue
        if size > 0:
            total += size
            found = True
    return total if found else None


def target_audio_bitrate(audio_streams, target_profile: TargetProfile):
    """Return aggregate bitrate for transcoded streams, or None when all copy."""
    if not audio_streams or all(
            audio_is_safe_for_target(stream, target_profile)
            for stream in audio_streams):
        return None
    return sum(
        audio_transcode_settings(stream, target_profile)[0]
        for stream in audio_streams
        if not audio_is_safe_for_target(stream, target_profile)
    )


def estimate_target_audio_bytes(input_path: Path, target_profile: TargetProfile,
                                duration, source_video_bytes):
    """Estimate audio bytes produced by the shared target audio policy."""
    audio_streams = probe_streams(input_path, "a")
    if not audio_streams:
        return 0.0
    estimated_bytes = 0.0
    unresolved_copy = False
    for audio_index, stream in enumerate(audio_streams):
        if not audio_is_safe_for_target(stream, target_profile):
            bitrate, _, _ = audio_transcode_settings(stream, target_profile)
            estimated_bytes += bitrate * duration / 8
            continue
        copied_payload = packet_payload_bytes(input_path, f"a:{audio_index}")
        if copied_payload is not None:
            estimated_bytes += copied_payload
            continue
        reported_bitrate = reported_stream_bitrate(stream)
        if reported_bitrate is not None:
            estimated_bytes += reported_bitrate * duration / 8
        else:
            unresolved_copy = True
    if not unresolved_copy:
        return estimated_bytes
    # This deliberately overestimates copied audio by assigning every remaining
    # source byte to it; shared audio in both candidates then makes the predicted
    # percentage saving more conservative rather than more optimistic.
    residual = max(0, input_path.stat().st_size - source_video_bytes)
    return max(estimated_bytes, float(residual))


def estimate_subtitle_bytes(input_path: Path, keep_subs: bool):
    """Estimate retained mov_text payload from compatible source subtitles."""
    if not keep_subs:
        return 0.0
    total = 0
    for subtitle_index, stream in enumerate(probe_streams(input_path, "s")):
        if stream.get("codec_name", "") not in TEXT_SUBTITLE_CODECS:
            continue
        payload = packet_payload_bytes(input_path, f"s:{subtitle_index}")
        if payload:
            total += payload
    return total * PREFLIGHT_SUBTITLE_ESTIMATE_MARGIN


def predicted_mp4_size(video_bytes, audio_bytes, subtitle_bytes):
    """Include shared subtitle payload and conservative MP4 mux overhead."""
    payload_bytes = video_bytes + audio_bytes + subtitle_bytes
    overhead = max(
        PREFLIGHT_CONTAINER_OVERHEAD_MIN_BYTES,
        payload_bytes * PREFLIGHT_CONTAINER_OVERHEAD_FRACTION,
    )
    return payload_bytes + overhead


def video_packet_bitrate(path: Path, *, start=None, duration=None):
    """Measure video packet payload bitrate, optionally over a source interval."""
    cmd = [
        "ffprobe", "-v", "error", "-select_streams", "v:0",
    ]
    if start is not None and duration is not None:
        cmd += ["-read_intervals", f"{start:.3f}%+{duration:.3f}"]
    cmd += [
        "-show_entries", "packet=size,duration_time,pts_time",
        "-of", "json", str(path),
    ]
    result = subprocess.run(cmd, capture_output=True, check=False)
    if result.returncode != 0:
        return None
    try:
        packets = json.loads(result.stdout.decode(errors="replace")).get("packets", [])
    except json.JSONDecodeError:
        return None
    packet_bytes = 0
    packet_duration = 0.0
    timestamps = []
    for packet in packets:
        try:
            packet_bytes += int(packet.get("size") or 0)
        except (TypeError, ValueError):
            continue
        parsed_duration = parsed_positive_number(packet.get("duration_time"))
        if parsed_duration:
            packet_duration += parsed_duration
        try:
            timestamp = float(packet.get("pts_time"))
            if math.isfinite(timestamp):
                timestamps.append(timestamp)
        except (TypeError, ValueError):
            pass
    if packet_bytes <= 0:
        return None
    if packet_duration <= 0 and len(timestamps) >= 2:
        packet_duration = max(timestamps) - min(timestamps)
    if packet_duration <= 0:
        packet_duration = duration or get_duration_seconds(path) or 0
    return packet_bytes * 8 / packet_duration if packet_duration > 0 else None


def representative_sample_plan(duration, positions=PREFLIGHT_INITIAL_POSITIONS,
                               exclude_starts=()):
    """Return distinct (position, start, duration) samples within the media."""
    if not duration or duration <= 0:
        return []
    sample_duration = min(
        PREFLIGHT_SAMPLE_DURATION_SECONDS,
        duration / len(PREFLIGHT_INITIAL_POSITIONS),
    )
    if sample_duration < PREFLIGHT_MIN_SAMPLE_DURATION_SECONDS:
        return []
    latest_start = max(0.0, duration - sample_duration)
    used_starts = list(exclude_starts)
    plan = []
    for position in positions:
        start = min(position * duration, latest_start)
        if any(abs(start - used_start) <= 0.01 for used_start in used_starts):
            continue
        plan.append((position, start, sample_duration))
        used_starts.append(start)
    return plan


def coefficient_of_variation(values):
    """Return population standard deviation divided by the positive mean."""
    if not values:
        return None
    mean = statistics.fmean(values)
    if mean <= 0:
        return None
    return statistics.pstdev(values) / mean


def preflight_margin_for_cv(cv):
    """Select a named conservative margin independently from sample expansion."""
    if cv <= PREFLIGHT_LOW_VARIANCE_CV:
        return PREFLIGHT_MARGIN_LOW
    if cv <= PREFLIGHT_HIGH_VARIANCE_CV:
        return PREFLIGHT_MARGIN_NORMAL
    return PREFLIGHT_MARGIN_HIGH


def variability_label(cv):
    if cv <= PREFLIGHT_LOW_VARIANCE_CV:
        return "low"
    if cv <= PREFLIGHT_HIGH_VARIANCE_CV:
        return "moderate"
    return "high"


def predicted_savings_fraction(source_bitrate, predicted_bitrate):
    if not source_bitrate or source_bitrate <= 0:
        return None
    return 1.0 - predicted_bitrate / source_bitrate


def reencode_is_worthwhile(predicted_savings):
    """The exact threshold rule is >= 10% predicted final-file savings."""
    return (
        predicted_savings is not None and
        (
            predicted_savings >= MIN_PREDICTED_REENCODE_SAVINGS or
            math.isclose(
                predicted_savings, MIN_PREDICTED_REENCODE_SAVINGS,
                rel_tol=1e-12, abs_tol=1e-12,
            )
        )
    )


def encoder_candidates_for(target_profile, gpu, hardware_candidates,
                           preferred_candidate=None):
    candidates = list(hardware_candidates) if gpu else []
    software = EncoderCandidate(target_profile.codec.software_encoder, "software")
    candidates.append(software)
    if preferred_candidate is None:
        return candidates
    return [preferred_candidate, *(
        candidate for candidate in candidates
        if candidate != preferred_candidate
    )]


def try_preflight_sample(input_path: Path, sample_path: Path,
                         target_profile: TargetProfile,
                         candidate: EncoderCandidate, crf: int, preset: str,
                         start: float, sample_duration: float,
                         active_paths: ActivePaths, variant=None):
    """Encode one video-only sample with the real target encoder settings."""
    variant = variant or default_encode_variant(target_profile)
    pre_input, video_args = encoder_video_args(
        target_profile, candidate, crf, preset, variant,
    )
    cmd = [
        "ffmpeg", "-y", *pre_input, "-ss", f"{start:.3f}",
        "-i", str(input_path), "-t", f"{sample_duration:.3f}",
        "-loglevel", "warning", "-progress", "pipe:1", "-nostats",
        "-map", "0:v:0", "-an", "-sn", *video_args,
        "-movflags", "+faststart", str(sample_path),
    ]
    return run_ffmpeg_with_progress(
        cmd, sample_duration, "preflight", active_path=sample_path,
        active_paths=active_paths,
    )


def collect_preflight_bitrates(input_path: Path, out_path: Path,
                               target_profile: TargetProfile,
                               candidate: EncoderCandidate, crf: int,
                               preset: str, sample_plan,
                               active_paths: ActivePaths, variant=None,
                               *, extra=False):
    """Encode one variant over a shared sample plan and return video bitrates."""
    variant = variant or default_encode_variant(target_profile)
    bitrates = []
    phase = "extra sample" if extra else "sample"
    for index, (position, start, sample_duration) in enumerate(
            sample_plan, start=1):
        sample_path = temp_preflight_path(
            out_path, candidate, variant, position,
        )
        active_paths.discard(sample_path)
        print(
            f"  Preflight {phase} {round(position * 100)}% "
            f"({index}/{len(sample_plan)}) at {format_hms(start)} "
            f"using {candidate.name} / {variant.display_name} "
            f"(mapped quality {mapped_quality(crf, candidate.backend, variant)})..."
        )
        succeeded = try_preflight_sample(
            input_path, sample_path, target_profile, candidate,
            crf, preset, start, sample_duration, active_paths, variant,
        )
        if not succeeded or not validated_candidate(
                sample_path, "Preflight sample", target_profile):
            active_paths.discard(sample_path)
            return None
        encoded_bitrate = video_packet_bitrate(sample_path)
        active_paths.discard(sample_path)
        if encoded_bitrate is None:
            return None
        bitrates.append(encoded_bitrate)
    return bitrates


def estimate_reencode_savings(input_path: Path, out_path: Path,
                              target_profile: TargetProfile, video_stream,
                              duration, crf: int, preset: str, gpu: bool,
                              hardware_candidates, active_paths: ActivePaths,
                              keep_subs: bool = True,
                              direct_remux_valid=True):
    """Return comparable-quality size estimates for supported iPhone variants."""
    initial_plan = representative_sample_plan(
        duration, PREFLIGHT_INITIAL_POSITIONS,
    )
    if len(initial_plan) < 2:
        return None
    source_video_bitrate = estimate_source_video_bitrate(
        input_path, video_stream, duration,
    )
    if direct_remux_valid and source_video_bitrate is None:
        return None
    source_video_bytes = (
        source_video_bitrate * duration / 8
        if source_video_bitrate is not None else 0.0
    )
    audio_bytes = estimate_target_audio_bytes(
        input_path, target_profile, duration, source_video_bytes,
    )
    subtitle_bytes = estimate_subtitle_bytes(input_path, keep_subs)
    predicted_direct_bytes = (
        predicted_mp4_size(source_video_bytes, audio_bytes, subtitle_bytes)
        if direct_remux_valid else None
    )

    for candidate in encoder_candidates_for(
            target_profile, gpu, hardware_candidates):
        initial_percentages = ", ".join(
            f"{round(position * 100)}%" for position, _, _ in initial_plan
        )
        print(f"  Initial samples: {initial_percentages}")
        variant_bitrates = {}
        for variant in encode_variants_for_target(target_profile):
            if not backend_can_attempt_variant(candidate, variant):
                continue
            encoded_bitrates = collect_preflight_bitrates(
                input_path, out_path, target_profile, candidate, crf,
                preset, initial_plan, active_paths, variant,
            )
            if encoded_bitrates is None:
                print(
                    f"  {candidate.name} cannot provide a validated "
                    f"{variant.display_name} candidate; omitting that variant."
                )
                continue
            variant_bitrates[variant] = encoded_bitrates
        if not variant_bitrates:
            print(
                f"  Preflight encoder {candidate.name} was unusable for every "
                "variant; trying the next backend."
            )
            continue

        initial_cvs = {
            variant: coefficient_of_variation(bitrates)
            for variant, bitrates in variant_bitrates.items()
        }
        for variant, cv in initial_cvs.items():
            if cv is not None:
                print(
                    f"  {variant.display_name} initial variability: "
                    f"{variability_label(cv)} (CV {cv:.2f})."
                )
        if any(
                cv is not None and cv > PREFLIGHT_HIGH_VARIANCE_CV
                for cv in initial_cvs.values()):
            initial_starts = [start for _, start, _ in initial_plan]
            extra_plan = representative_sample_plan(
                duration, PREFLIGHT_EXTRA_POSITIONS,
                exclude_starts=initial_starts,
            )
            if extra_plan:
                extra_percentages = ", ".join(
                    f"{round(position * 100)}%"
                    for position, _, _ in extra_plan
                )
                print(
                    f"  Taking {len(extra_plan)} additional sample(s) at "
                    f"{extra_percentages}..."
                )
                for variant in tuple(variant_bitrates):
                    extra_bitrates = collect_preflight_bitrates(
                        input_path, out_path, target_profile, candidate,
                        crf, preset, extra_plan, active_paths, variant,
                        extra=True,
                    )
                    if extra_bitrates is None:
                        print(
                            f"  Extra sampling invalidated "
                            f"{variant.display_name}; omitting that variant."
                        )
                        del variant_bitrates[variant]
                    else:
                        variant_bitrates[variant].extend(extra_bitrates)
            else:
                print(
                    "  No additional distinct sample positions fit this short "
                    "video; using the initial samples with a high margin."
                )
        else:
            print("  Five samples are sufficient; no extra samples needed.")

        estimates = []
        for variant, encoded_bitrates in variant_bitrates.items():
            final_cv = coefficient_of_variation(encoded_bitrates)
            if final_cv is None:
                continue
            estimate_margin = preflight_margin_for_cv(final_cv)
            encoded_video_bitrate = statistics.median(encoded_bitrates)
            conservative_video_bitrate = encoded_video_bitrate * estimate_margin
            print(
                f"  {variant.display_name} final variability: "
                f"{variability_label(final_cv)} (CV {final_cv:.2f}); "
                f"{len(encoded_bitrates)} sample(s), "
                f"{estimate_margin:.2f}x margin."
            )
            predicted_encode_bytes = predicted_mp4_size(
                conservative_video_bitrate * duration / 8,
                audio_bytes,
                subtitle_bytes,
            )
            predicted_savings = (
                predicted_savings_fraction(
                    predicted_direct_bytes, predicted_encode_bytes,
                )
                if predicted_direct_bytes is not None else None
            )
            estimates.append(PreflightEstimate(
                predicted_savings=predicted_savings,
                encoder_candidate=candidate,
                encode_variant=variant,
                source_video_bitrate=source_video_bitrate,
                encoded_video_bitrate=encoded_video_bitrate,
                conservative_video_bitrate=conservative_video_bitrate,
                predicted_direct_bytes=predicted_direct_bytes,
                predicted_encode_bytes=predicted_encode_bytes,
                sample_count=len(encoded_bitrates),
                coefficient_of_variation=final_cv,
                estimate_margin=estimate_margin,
            ))
        if estimates:
            return tuple(estimates)
    return None


def iphone_preflight_decision(input_path: Path, out_path: Path, video_stream,
                              target_profile: TargetProfile, duration,
                              crf: int, preset: str, gpu: bool,
                              hardware_candidates, active_paths: ActivePaths,
                              keep_subs: bool = True,
                              direct_remux_valid=True):
    """Choose remux, Main 8-bit, or Main10 from quality-qualified estimates."""
    estimates = estimate_reencode_savings(
        input_path, out_path, target_profile, video_stream, duration,
        crf, preset, gpu, hardware_candidates, active_paths, keep_subs,
        direct_remux_valid,
    )
    if not estimates:
        return PreflightDecision(
            should_reencode=True,
            reason=(
                "A reliable variant estimate was unavailable; falling back to "
                "the normal HEVC Main 8-bit encode pipeline."
            ),
            encode_variant=HEVC_MAIN_8BIT,
        )
    ordered = sorted(estimates, key=lambda estimate: estimate.predicted_encode_bytes)
    estimate = ordered[0]
    fallback_plans = tuple(
        EncodePlan(item.encoder_candidate, item.encode_variant)
        for item in ordered[1:]
        if (not direct_remux_valid or
            reencode_is_worthwhile(item.predicted_savings))
    )
    common = {
        "predicted_savings": estimate.predicted_savings,
        "encoder_candidate": estimate.encoder_candidate,
        "encode_variant": estimate.encode_variant,
        "fallback_plans": fallback_plans,
        "variant_estimates": tuple(estimates),
        "source_video_bitrate": estimate.source_video_bitrate,
        "encoded_video_bitrate": estimate.encoded_video_bitrate,
        "conservative_video_bitrate": estimate.conservative_video_bitrate,
        "predicted_direct_bytes": estimate.predicted_direct_bytes,
        "predicted_encode_bytes": estimate.predicted_encode_bytes,
        "sample_count": estimate.sample_count,
        "coefficient_of_variation": estimate.coefficient_of_variation,
        "estimate_margin": estimate.estimate_margin,
    }
    if not direct_remux_valid:
        return PreflightDecision(
            should_reencode=True,
            reason=(
                f"Direct remux is incompatible; selected the smallest valid "
                f"quality-qualified variant ({estimate.encode_variant.display_name})."
            ),
            **common,
        )
    if reencode_is_worthwhile(estimate.predicted_savings):
        return PreflightDecision(
            should_reencode=True,
            reason=(
                f"{estimate.encode_variant.display_name} meets the worthwhile "
                "re-encode threshold."
            ),
            **common,
        )
    return PreflightDecision(
        should_reencode=False,
        reason=(
            "Neither encode variant beats direct remux by the worthwhile "
            "savings threshold."
        ),
        **common,
    )


def full_reencode(input_path: Path, out_path: Path, target_profile: TargetProfile,
                  crf: int, preset: str, gpu: bool, hardware_candidates,
                  keep_subs: bool, duration, active_paths: ActivePaths,
                  preferred_candidate=None, variant=None):
    """Try equivalent backends without silently changing the chosen bit depth."""
    variant = variant or default_encode_variant(target_profile)
    candidates = encoder_candidates_for(
        target_profile, gpu, hardware_candidates, preferred_candidate,
    )
    for index, candidate in enumerate(candidates):
        if not backend_can_attempt_variant(candidate, variant):
            continue
        active_paths.discard(out_path)
        if candidate.backend != "software":
            print(
                f"  Hardware attempt {index + 1}: {candidate.name} / "
                f"{variant.display_name}"
            )
        elif gpu:
            print(
                f"  Hardware attempts exhausted; falling back to "
                f"CPU/{candidate.name} with {variant.display_name}."
            )
        succeeded = try_reencode(
            input_path, out_path, target_profile, candidate, crf, preset,
            keep_subs, duration, active_paths, variant,
        )
        if succeeded and validated_candidate(
                out_path,
                f"{candidate.name} {variant.display_name} encode",
                target_profile):
            return True, candidate.name
        if candidate.backend != "software":
            print(
                f"  {candidate.name} failed or produced an invalid "
                f"{variant.display_name} result; trying the next equivalent backend."
            )
    active_paths.discard(out_path)
    return False, None


def format_size_change(original_bytes: int, new_bytes: int):
    """Describe an exact byte-size relation without misleading rounded zeroes."""
    if original_bytes == new_bytes or original_bytes <= 0:
        return "0% change"
    percent_change = 100 * (new_bytes - original_bytes) / original_bytes
    magnitude = abs(percent_change)
    if magnitude < 0.05:
        return "0% change"
    formatted = f"{magnitude:.1f}" if magnitude < 1 else f"{magnitude:.0f}"
    direction = "larger" if percent_change > 0 else "smaller"
    return f"{formatted}% {direction}"


def successful_result(input_path: Path, out_path: Path, original_bytes: int,
                      method: str, implementation: str, video_method: str,
                      operation_method: str | None = None):
    """Build and print the common successful-conversion result."""
    original_mb = original_bytes / (1024 * 1024)
    new_bytes = out_path.stat().st_size
    new_mb = new_bytes / (1024 * 1024)
    percent = 100 * (1 - new_bytes / original_bytes) if original_bytes else 0.0
    size_relation = format_size_change(original_bytes, new_bytes)
    print(
        f"  Done: {out_path.name} ({original_mb:.1f} MB -> {new_mb:.1f} MB, "
        f"{size_relation}) [{method} via {implementation}]"
    )
    return {
        "status": "converted", "name": input_path.name,
        "output_name": out_path.name, "orig_mb": original_mb,
        "new_mb": new_mb, "orig_bytes": original_bytes,
        "new_bytes": new_bytes, "pct": percent, "method": method,
        "video_method": video_method,
        "operation_method": operation_method or video_method,
    }


def promote_valid_candidate(candidate_path: Path, out_path: Path,
                            active_paths: ActivePaths):
    """Atomically promote a validated candidate without pre-deleting a final."""
    candidate_path.replace(out_path)
    active_paths.complete(candidate_path)


def print_iphone_candidate_estimates(decision: PreflightDecision):
    """Explain comparable-quality variant estimates without excessive detail."""
    if decision.predicted_direct_bytes is not None:
        print(
            f"  Direct-remux estimate: "
            f"{decision.predicted_direct_bytes / (1024 * 1024):.1f} MB"
        )
    for estimate in decision.variant_estimates:
        print(
            f"  {estimate.encode_variant.display_name} estimate: "
            f"{estimate.predicted_encode_bytes / (1024 * 1024):.1f} MB "
            f"({estimate.sample_count} samples, CV "
            f"{estimate.coefficient_of_variation:.2f}, "
            f"{estimate.estimate_margin:.2f}x margin)"
        )
        if estimate.predicted_savings is not None:
            print(
                f"    Predicted saving vs remux: "
                f"{estimate.predicted_savings * 100:.0f}%"
            )
    if decision.predicted_direct_bytes is not None:
        print(
            "  Minimum worthwhile saving: "
            f"{MIN_PREDICTED_REENCODE_SAVINGS * 100:.0f}% (>= encodes)."
        )


def encoded_method_for(target_profile: TargetProfile,
                       variant: VideoEncodeVariant):
    if target_profile.name != "iphone":
        return "re-encoded"
    if variant.bit_depth == 10:
        return "re-encoded HEVC Main10"
    return "re-encoded HEVC Main 8-bit"


def input_is_already_target_compatible(input_path: Path,
                                       target_profile: TargetProfile,
                                       keep_subs: bool):
    """Return whether audio-only mode can avoid creating a duplicate output."""
    if input_path.suffix.lower() != ".mp4":
        return False
    valid, _ = validate_output(input_path, target_profile)
    if not valid:
        return False
    return keep_subs or not probe_streams(input_path, "s")


def input_is_container_ready(input_path: Path, target_profile: TargetProfile,
                             keep_subs: bool):
    """Return whether an MP4 already satisfies container-mode promises."""
    if input_path.suffix.lower() != ".mp4":
        return False
    valid, _ = validate_container_output(
        input_path, input_path, target_profile,
    )
    if not valid:
        return False
    return keep_subs or not probe_streams(input_path, "s")


def convert_container_file(input_path: Path, out_path: Path,
                           target_profile: TargetProfile, keep_subs: bool,
                           duration, original_bytes: int,
                           active_paths: ActivePaths):
    """Remux video unchanged into MP4 and apply only target audio policy."""
    if input_is_container_ready(input_path, target_profile, keep_subs):
        print("  Skipping: source is already MP4 with target-safe audio.")
        return {"status": "skipped_container_ready", "name": input_path.name}

    audio_streams = probe_streams(input_path, "a")
    repairs_audio = any(
        not audio_is_safe_for_target(stream, target_profile)
        for stream in audio_streams
    )
    candidate = temp_container_path(out_path)
    active_paths.discard(candidate)
    print("  Container mode: copying the original video bitstream into MP4.")
    completed = try_container_remux(
        input_path, candidate, target_profile, keep_subs,
        duration, active_paths,
    )
    if not completed:
        active_paths.discard(candidate)
        reason = "FFmpeg could not mux the unchanged video stream into MP4"
        print(f"  Skipping: {reason}.")
        return {
            "status": "skipped_container_mux", "name": input_path.name,
            "reason": reason,
        }
    if not validated_container_candidate(
            candidate, input_path, "Container remux", target_profile):
        active_paths.discard(candidate)
        print(f"  Container remux failed validation: {input_path.name}")
        return {"status": "failed", "name": input_path.name}

    promote_valid_candidate(candidate, out_path, active_paths)
    if not validated_container_candidate(
            out_path, input_path, "Final output", target_profile):
        safe_unlink(out_path)
        print(f"  Failed final validation: {input_path.name}")
        return {"status": "failed", "name": input_path.name}
    method = (
        "container remux, audio repaired"
        if repairs_audio else "container remux"
    )
    operation_method = (
        "container_audio_repair" if repairs_audio else "container_remux"
    )
    return successful_result(
        input_path, out_path, original_bytes, method, "container mode",
        VIDEO_METHOD_COPIED_REMUXED, operation_method,
    )


def convert_audio_only_file(input_path: Path, out_path: Path,
                            target_profile: TargetProfile, keep_subs: bool,
                            duration, original_bytes: int,
                            active_paths: ActivePaths):
    """Copy target-safe video, repair audio if needed, and never encode video."""
    safe_video, reason = can_safely_remux_for_target(input_path, target_profile)
    if not safe_video:
        print(
            "  Skipping: video requires re-encoding, but --audio-only forbids "
            f"it ({reason})."
        )
        return {
            "status": "skipped_audio_only_video", "name": input_path.name,
            "reason": (
                f"{reason}; requires video re-encoding for "
                f"{target_profile.name}"
            ),
        }

    if input_is_already_target_compatible(
            input_path, target_profile, keep_subs):
        print("  Skipping: source is already target-compatible.")
        return {
            "status": "skipped_target_compatible", "name": input_path.name,
        }

    audio_streams = probe_streams(input_path, "a")
    repairs_audio = any(
        not audio_is_safe_for_target(stream, target_profile)
        for stream in audio_streams
    )
    candidate = temp_audio_only_path(out_path)
    active_paths.discard(candidate)
    print("  Audio-only mode: copying the original video stream without encoding.")
    completed = try_remux(
        input_path, candidate, target_profile, keep_subs,
        duration, active_paths, direct=True,
    )
    if not completed or not validated_candidate(
            candidate, "Audio-only", target_profile):
        active_paths.discard(candidate)
        print(f"  Audio-only remux failed: {input_path.name}")
        return {"status": "failed", "name": input_path.name}

    promote_valid_candidate(candidate, out_path, active_paths)
    if not validated_candidate(out_path, "Final output", target_profile):
        safe_unlink(out_path)
        print(f"  Failed final validation: {input_path.name}")
        return {"status": "failed", "name": input_path.name}
    method = (
        "video copied, audio repaired"
        if repairs_audio else
        ("video/audio copied, remuxed" if audio_streams else "video copied, remuxed")
    )
    operation_method = (
        "audio_only_repair" if repairs_audio else "audio_only_remux"
    )
    return successful_result(
        input_path, out_path, original_bytes, method, "audio-only",
        VIDEO_METHOD_COPIED_REMUXED, operation_method,
    )


def convert_file(input_path: Path, out_path: Path, target_profile: TargetProfile,
                 crf: int, preset: str, gpu: bool, hardware_candidates,
                 keep_subs: bool, convert_no_audio: bool,
                 active_paths: ActivePaths, audio_only: bool = False,
                 container_mode: bool = False):
    """Convert one file and return a result with a distinct summary status."""
    if not convert_no_audio and not probe_streams(input_path, "a"):
        print(f"Skipping (no audio stream): {input_path}")
        return {"status": "skipped_no_audio", "name": input_path.name}
    if out_path.exists():
        print(f"Skipping (already exists): {out_path}")
        return {"status": "skipped_existing", "name": input_path.name}

    out_path.parent.mkdir(parents=True, exist_ok=True)
    duration = get_duration_seconds(input_path)
    original_bytes = input_path.stat().st_size
    if container_mode:
        return convert_container_file(
            input_path, out_path, target_profile, keep_subs,
            duration, original_bytes, active_paths,
        )
    if audio_only:
        return convert_audio_only_file(
            input_path, out_path, target_profile, keep_subs,
            duration, original_bytes, active_paths,
        )
    encode_tmp = temp_encode_path(out_path)
    remux_tmp = temp_remux_path(out_path)
    direct_tmp = temp_direct_remux_path(out_path)
    for candidate_path in (encode_tmp, remux_tmp, direct_tmp):
        active_paths.discard(candidate_path)

    preferred_encoder = None
    selected_variant = default_encode_variant(target_profile)
    fallback_plans = ()
    iphone_source_is_safe = False
    preflight_selected_encode = False
    video_streams = probe_streams(input_path, "v:0")
    if target_profile.name == "iphone" and video_streams:
        safe, reason = can_safely_remux_for_target(input_path, target_profile)
        if safe:
            iphone_source_is_safe = True
            print("  Source video is already iPhone-compatible.")
        else:
            print(f"  Direct remux is not iPhone-safe: {reason}.")
        print("  Running adaptive preflight for HEVC 8-bit and 10-bit variants...")
        decision = iphone_preflight_decision(
            input_path, out_path, video_streams[0], target_profile,
            duration, crf, preset, gpu, hardware_candidates, active_paths,
            keep_subs, direct_remux_valid=safe,
        )
        print_iphone_candidate_estimates(decision)
        print(f"  {decision.reason}")
        preferred_encoder = decision.encoder_candidate
        selected_variant = decision.encode_variant or HEVC_MAIN_8BIT
        fallback_plans = decision.fallback_plans
        if not decision.should_reencode:
            print("  Selected: direct remux. Trying lossless video copy...")
            direct_ok = try_remux(
                input_path, direct_tmp, target_profile, keep_subs,
                duration, active_paths, direct=True,
            )
            if direct_ok and validated_candidate(
                    direct_tmp, "Direct remux", target_profile):
                promote_valid_candidate(direct_tmp, out_path, active_paths)
                if validated_candidate(out_path, "Final output", target_profile):
                    return successful_result(
                        input_path, out_path, original_bytes,
                        "direct-remux", "stream copy",
                        VIDEO_METHOD_COPIED_REMUXED,
                    )
                safe_unlink(out_path)
                print("  Direct remux failed final validation.")
            else:
                print("  Direct remux failed or did not pass validation.")
            active_paths.discard(direct_tmp)
            print(
                f"  Falling back to sampled {selected_variant.display_name} encode..."
            )
        else:
            preflight_selected_encode = bool(decision.variant_estimates)
            print(f"  Selected: {selected_variant.display_name} full encode.")

    encoded, encoder_name = full_reencode(
        input_path, encode_tmp, target_profile, crf, preset, gpu,
        hardware_candidates, keep_subs, duration, active_paths,
        preferred_candidate=preferred_encoder,
        variant=selected_variant,
    )
    if not encoded and fallback_plans:
        for fallback_plan in fallback_plans:
            print(
                f"  Re-evaluating with sampled fallback "
                f"{fallback_plan.variant.display_name}; bit depth will not "
                "change silently."
            )
            encoded, encoder_name = full_reencode(
                input_path, encode_tmp, target_profile, crf, preset, gpu,
                hardware_candidates, keep_subs, duration, active_paths,
                preferred_candidate=fallback_plan.encoder,
                variant=fallback_plan.variant,
            )
            if encoded:
                selected_variant = fallback_plan.variant
                break
    winner = None
    method = encoded_method_for(target_profile, selected_variant)
    implementation = encoder_name
    video_method = VIDEO_METHOD_REENCODED
    if not encoded:
        print("  Full re-encode failed; trying a safe stream-copy fallback if possible...")
        remux_ok = try_remux(
            input_path, remux_tmp, target_profile, keep_subs,
            duration, active_paths,
        )
        if remux_ok and validated_candidate(remux_tmp, "Remux", target_profile):
            winner = remux_tmp
            method = "remuxed"
            implementation = "stream copy"
            video_method = VIDEO_METHOD_COPIED_REMUXED
        else:
            active_paths.discard(remux_tmp)
            print(f"  Failed: {input_path.name}")
            return {"status": "failed", "name": input_path.name}
    else:
        winner = encode_tmp
        encoded_bytes = encode_tmp.stat().st_size
        compare_safe_remux = (
            encoded_bytes >= original_bytes or
            (iphone_source_is_safe and preflight_selected_encode)
        )
        if compare_safe_remux:
            print(
                "  Checking the completed re-encode against a validated safe "
                "remux candidate..."
            )
            remux_ok = try_remux(
                input_path, remux_tmp, target_profile, keep_subs,
                duration, active_paths,
            )
            if remux_ok and validated_candidate(remux_tmp, "Remux", target_profile):
                if remux_tmp.stat().st_size < encoded_bytes:
                    winner = remux_tmp
                    method = "remuxed"
                    implementation = "stream copy"
                    video_method = VIDEO_METHOD_COPIED_REMUXED
                    print("  Remux candidate is smaller; using it.")
                else:
                    active_paths.discard(remux_tmp)
                    print("  Remux candidate is not smaller; keeping the valid re-encode.")
            else:
                active_paths.discard(remux_tmp)
                print("  Safe remux was unavailable or failed; keeping the valid re-encode.")

    promote_valid_candidate(winner, out_path, active_paths)
    for candidate_path in (encode_tmp, remux_tmp, direct_tmp):
        if candidate_path != winner:
            active_paths.discard(candidate_path)
    if not validated_candidate(out_path, "Final output", target_profile):
        safe_unlink(out_path)
        print(f"  Failed final validation: {input_path.name}")
        return {"status": "failed", "name": input_path.name}
    return successful_result(
        input_path, out_path, original_bytes, method, implementation,
        video_method,
    )


def output_paths_for(video_files, folder: Path, target_root: Path):
    """Mirror source directories and deterministically disambiguate collisions."""
    base_keys = []
    for path in video_files:
        relative_directory = path.parent.relative_to(folder)
        base_keys.append((relative_directory, path.stem.lower()))
    counts = Counter(base_keys)
    mapping = {}
    used = set()
    for path in video_files:
        relative_directory = path.parent.relative_to(folder)
        base_key = (relative_directory, path.stem.lower())
        if counts[base_key] == 1:
            filename = f"{path.stem}.mp4"
        else:
            suffix = path.suffix.lower().lstrip(".") or "input"
            filename = f"{path.stem}__{suffix}.mp4"
        candidate = target_root / relative_directory / filename
        serial = 2
        candidate_key = candidate.as_posix().casefold()
        while candidate_key in used:
            candidate = target_root / relative_directory / f"{Path(filename).stem}_{serial}.mp4"
            serial += 1
            candidate_key = candidate.as_posix().casefold()
        used.add(candidate_key)
        mapping[path] = candidate
    return mapping


def target_output_root(converted_root: Path, target_profile: TargetProfile,
                       container_mode: bool):
    if container_mode:
        return converted_root / "container" / target_profile.name
    return converted_root / target_profile.name


def print_startup_summary(target_profile: TargetProfile, target_root: Path,
                          gpu: bool, hardware_candidates, crf: int,
                          preset: str, keep_subs: bool,
                          audio_only: bool = False,
                          container_mode: bool = False):
    codec = target_profile.codec
    print(f"Target: {target_profile.name}")
    print("Container: MP4 (+faststart)")
    if container_mode:
        print("Video policy: copy any stream FFmpeg can mux into MP4; target video rules ignored")
    elif target_profile.name == "iphone":
        print(
            "Safe retained video: HEVC Main/yuv420p, HEVC Main 10/yuv420p10le, "
            "or conservatively compatible H.264/yuv420p"
        )
        if not audio_only:
            print(
                "Video encode candidates: HEVC Main / yuv420p 8-bit; "
                "HEVC Main10 / yuv420p10le 10-bit; hvc1"
            )
            print(
                "Bit-depth preflight: adaptive 5→9 positions per supported variant, "
                f"CV-based {PREFLIGHT_MARGIN_LOW:.2f}x–{PREFLIGHT_MARGIN_HIGH:.2f}x margin; encode at "
                f">={MIN_PREDICTED_REENCODE_SAVINGS * 100:.0f}% predicted final saving"
            )
    else:
        print(
            f"Video target: {codec.display_name} / yuv420p / {codec.output_tag}"
        )
    if target_profile.name == "iphone":
        print("Audio target: AAC-LC mono/stereo; unsafe audio -> stereo 192k/48kHz")
    else:
        print("Audio target: AAC-LC")
    if container_mode:
        print("Resolution/frame-rate policy: preserve source unchanged")
    else:
        if target_profile.max_width and target_profile.max_height:
            print(f"Maximum resolution: {target_profile.max_width}x{target_profile.max_height}")
        else:
            print("Resolution limit: none (source dimensions preserved, adjusted to even values if needed)")
        if target_profile.max_fps is not None:
            print(f"Maximum frame rate: {target_profile.max_fps:g} fps")
        else:
            print("Frame-rate limit: none (source frame rate preserved)")
    if container_mode:
        print("Processing mode: container-only; video bitstream is copied or the file is skipped")
        print("Video encoding/preflight: disabled")
        print("Note: --gpu, --crf, and --preset do not affect video in --container mode")
    elif audio_only:
        print("Processing mode: audio-only; source video is stream-copied or the file is skipped")
        print("Video encoding/preflight: disabled")
        print("Note: --gpu, --crf, and --preset do not affect video in --audio-only mode")
    elif gpu:
        names = ", ".join(candidate.name for candidate in hardware_candidates) or "none detected"
        print(
            f"Hardware encoding: enabled; ordered {codec.name.upper()} candidates: {names}; "
            f"CPU/{codec.software_encoder} fallback"
        )
    else:
        print(f"Hardware encoding: disabled; CPU/{codec.software_encoder}")
    if not audio_only and not container_mode:
        print(f"Quality: {crf}; encoder preset: {preset}")
    if container_mode:
        subtitle_text = "existing mov_text copied; other subtitle codecs dropped"
        print(f"Subtitles: {subtitle_text if keep_subs else 'stripped'}")
    else:
        print(f"Subtitles: {'compatible text retained as mov_text' if keep_subs else 'stripped'}")
    print(f"Output folder: {target_root}")
    if container_mode:
        print(
            "Container policy: validate a temporary MP4 before promotion; "
            "never fall back to video encoding.\n"
        )
    elif audio_only:
        print(
            "Audio-only policy: validate a temporary video-copy MP4 before "
            "promotion; never fall back to video encoding.\n"
        )
    else:
        print(
            "Size policy: keep the smallest validated target-compatible candidate; "
            "never remove a good re-encode before a replacement validates.\n"
        )


def print_converted_group(title, results):
    """Print one method/size group of successful conversion results."""
    if not results:
        return
    print(f"\n{title} ({len(results)} file(s)):")
    for result in results:
        size_text = format_size_change(
            result["orig_bytes"], result["new_bytes"],
        )
        print(
            f"  {result['name']} -> {result['output_name']} [{result['method']}]\n"
            f"    {result['orig_mb']:.1f} MB -> {result['new_mb']:.1f} MB ({size_text})"
        )


def print_batch_summary(folder: Path, target_profile: TargetProfile,
                        converted, failed, skipped_existing, skipped_no_audio,
                        interrupted=False, interrupted_file=None,
                        skipped_audio_only_video=None,
                        skipped_target_compatible=None,
                        skipped_container_mux=None,
                        skipped_container_ready=None):
    """Print accumulated results after normal completion or interruption."""
    skipped_audio_only_video = skipped_audio_only_video or []
    skipped_target_compatible = skipped_target_compatible or []
    skipped_container_mux = skipped_container_mux or []
    skipped_container_ready = skipped_container_ready or []
    print("\n" + "=" * 64)
    print(f"Summary — target: {target_profile.name}")
    print("=" * 64)
    if converted:
        reencoded = [
            result for result in converted
            if result["video_method"] == VIDEO_METHOD_REENCODED
        ]
        copied_remuxed = [
            result for result in converted
            if result["video_method"] == VIDEO_METHOD_COPIED_REMUXED
        ]
        reencoded_reduced = [
            result for result in reencoded
            if result["new_bytes"] < result["orig_bytes"]
        ]
        reencoded_increased = [
            result for result in reencoded
            if result["new_bytes"] > result["orig_bytes"]
        ]
        reencoded_unchanged = [
            result for result in reencoded
            if result["new_bytes"] == result["orig_bytes"]
        ]
        print_converted_group(
            "Re-encoded — size reduced", reencoded_reduced,
        )
        print_converted_group(
            "Re-encoded — size increased", reencoded_increased,
        )
        print_converted_group(
            "Re-encoded — size unchanged", reencoded_unchanged,
        )
        print_converted_group(
            "Copied/remuxed — video not re-encoded", copied_remuxed,
        )
    if skipped_existing:
        print(f"\nSkipped — output already exists ({len(skipped_existing)} file(s)):")
        for path in skipped_existing:
            print(f"  {path.relative_to(folder)}")
    if skipped_no_audio:
        print(f"\nSkipped — no audio stream ({len(skipped_no_audio)} file(s)):")
        for path in skipped_no_audio:
            print(f"  {path.relative_to(folder)}")
        print("\nRe-run with --convert-no-audio if you want these converted too.")
    if skipped_audio_only_video:
        print(
            "\nSkipped — video incompatible with --audio-only "
            f"({len(skipped_audio_only_video)} file(s)):"
        )
        for result in skipped_audio_only_video:
            print(f"  {result['path'].relative_to(folder)}")
            print(f"    {result['reason']}")
    if skipped_target_compatible:
        print(
            "\nSkipped — already target-compatible "
            f"({len(skipped_target_compatible)} file(s)):"
        )
        for path in skipped_target_compatible:
            print(f"  {path.relative_to(folder)}")
    if skipped_container_mux:
        print(
            "\nSkipped — video cannot be copied into MP4 "
            f"({len(skipped_container_mux)} file(s)):"
        )
        for result in skipped_container_mux:
            print(f"  {result['path'].relative_to(folder)}")
            print(f"    {result['reason']}")
    if skipped_container_ready:
        print(
            "\nSkipped — already MP4 with target-safe audio "
            f"({len(skipped_container_ready)} file(s)):"
        )
        for path in skipped_container_ready:
            print(f"  {path.relative_to(folder)}")
    if failed:
        print(f"\nFailed ({len(failed)} file(s)):")
        for path in failed:
            print(f"  {path.relative_to(folder)}")
    if interrupted:
        print("\nBatch interrupted by user.")
        if interrupted_file is not None:
            print("Interrupted while processing:")
            print(f"  {interrupted_file.relative_to(folder)}")
    if converted:
        total_original_bytes = sum(result["orig_bytes"] for result in converted)
        total_new_bytes = sum(result["new_bytes"] for result in converted)
        total_original_mb = total_original_bytes / (1024 * 1024)
        total_new_mb = total_new_bytes / (1024 * 1024)
        total_text = format_size_change(total_original_bytes, total_new_bytes)
        print("\nTotal successful outputs:")
        print(
            f"  {total_original_mb:.1f} MB -> {total_new_mb:.1f} MB "
            f"({total_text})"
        )
    print()


def build_parser():
    parser = argparse.ArgumentParser(
        prog="video_to_iphone_mp4.py",
        description=(
            "Recursively find files containing FFmpeg-decodable video and make "
            "validated MP4 output for a selected playback target. Input filename "
            "extensions do not matter. Normal iphone mode compares safe remux, "
            "HEVC Main 8-bit, and Main10 10-bit candidates; --audio-only instead "
            "requires target-safe video copy, while --container copies any video "
            "stream FFmpeg can mux into MP4."
        ),
        epilog=(
            "Examples:\n"
            "  python video_to_iphone_mp4.py ~/Videos\n"
            "  python video_to_iphone_mp4.py ~/Videos --target iphone\n"
            "  python video_to_iphone_mp4.py ~/Videos --target windows --preset slow\n"
            "  python video_to_iphone_mp4.py ~/Videos --target mac --gpu\n"
            "  python video_to_iphone_mp4.py ~/Videos --target lg-tv\n"
            "  python video_to_iphone_mp4.py ~/Videos --target linux --gpu\n"
            "  python video_to_iphone_mp4.py ~/Videos --target iphone --audio-only\n"
            "  python video_to_iphone_mp4.py ~/Videos --target iphone --container\n"
            "\nNormal outputs use FOLDER/converted/TARGET/; container outputs "
            "use FOLDER/converted/container/TARGET/."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "folder", help=(
            "Folder to probe recursively; normal outputs use "
            "folder/converted/TARGET/, container outputs use "
            "folder/converted/container/TARGET/."
        ),
    )
    parser.add_argument(
        "--target", choices=tuple(TARGETS), default="iphone",
        help=(
            "Playback compatibility target; in --container it controls audio "
            "only (default: iphone)."
        ),
    )
    parser.add_argument(
        "--crf", type=int, default=22,
        help=(
            "Quality target, 0-51; lower is better/larger. Hardware mappings "
            "and iPhone 8/10-bit mappings are approximate; ignored for video "
            "with --audio-only/--container (default: 22)."
        ),
    )
    parser.add_argument(
        "--preset", choices=PRESET_CHOICES, default="medium",
        help=(
            "Encoder speed/compression-efficiency tradeoff; ignored for video "
            "with --audio-only/--container (default: medium)."
        ),
    )
    parser.add_argument(
        "--gpu", action="store_true",
        help=(
            "Auto-detect suitable target-codec hardware encoders and validate "
            "attempts; fall back to CPU libx264/libx265. Ignored by "
            "--audio-only/--container."
        ),
    )
    parser.add_argument(
        "--no-subs", action="store_true",
        help=(
            "Strip subtitles; otherwise normal modes convert compatible text, "
            "while --container copies only existing mov_text."
        ),
    )
    parser.add_argument(
        "--convert-no-audio", action="store_true",
        help="Convert silent videos; otherwise list them as a distinct skipped category.",
    )
    processing_mode = parser.add_mutually_exclusive_group()
    processing_mode.add_argument(
        "--audio-only", action="store_true",
        help=(
            "Never encode video: copy target-safe source video and repair/copy "
            "audio; skip video that requires encoding."
        ),
    )
    processing_mode.add_argument(
        "--container", action="store_true",
        help=(
            "Copy any MP4-muxable video bitstream unchanged, repair/copy target "
            "audio, and never run a video encoder."
        ),
    )
    return parser


def main():
    parser = build_parser()
    args = parser.parse_args()
    if not args.audio_only and not args.container and not 0 <= args.crf <= 51:
        parser.error("--crf must be between 0 and 51")
    for tool in ("ffmpeg", "ffprobe"):
        if shutil.which(tool) is None:
            sys.exit(f"Error: {tool} not found on PATH. Install FFmpeg first.")

    folder = Path(args.folder).expanduser().resolve()
    if not folder.is_dir():
        sys.exit(f"Error: '{folder}' is not a valid directory.")
    target_profile = TARGETS[args.target]
    converted_root = folder / "converted"
    target_root = target_output_root(
        converted_root, target_profile, args.container,
    )
    hardware_candidates = (
        hardware_encoder_candidates(target_profile)
        if args.gpu and not args.audio_only and not args.container else []
    )

    video_files = discover_video_files(folder, converted_root)
    if not video_files:
        print(f"No FFmpeg-decodable video files found in {folder}.")
        return
    output_paths = output_paths_for(video_files, folder, target_root)
    print(f"Found {len(video_files)} video file(s) in {folder}.")
    print_startup_summary(
        target_profile, target_root, args.gpu, hardware_candidates,
        args.crf, args.preset, not args.no_subs, args.audio_only,
        args.container,
    )

    converted = []
    failed = []
    skipped_existing = []
    skipped_no_audio = []
    skipped_audio_only_video = []
    skipped_target_compatible = []
    skipped_container_mux = []
    skipped_container_ready = []
    active_paths = ActivePaths()
    interrupted = False
    interrupted_file = None
    try:
        for index, input_file in enumerate(video_files, start=1):
            interrupted_file = input_file
            print(f"[{index}/{len(video_files)}] {input_file}")
            result = convert_file(
                input_file, output_paths[input_file], target_profile,
                args.crf, args.preset, args.gpu, hardware_candidates,
                keep_subs=not args.no_subs,
                convert_no_audio=args.convert_no_audio,
                active_paths=active_paths,
                audio_only=args.audio_only,
                container_mode=args.container,
            )
            if result["status"] == "converted":
                converted.append(result)
            elif result["status"] == "failed":
                failed.append(input_file)
            elif result["status"] == "skipped_existing":
                skipped_existing.append(input_file)
            elif result["status"] == "skipped_no_audio":
                skipped_no_audio.append(input_file)
            elif result["status"] == "skipped_audio_only_video":
                result["path"] = input_file
                skipped_audio_only_video.append(result)
            elif result["status"] == "skipped_target_compatible":
                skipped_target_compatible.append(input_file)
            elif result["status"] == "skipped_container_mux":
                result["path"] = input_file
                skipped_container_mux.append(result)
            elif result["status"] == "skipped_container_ready":
                skipped_container_ready.append(input_file)
            interrupted_file = None
    except KeyboardInterrupt:
        interrupted = True
        active_paths.cleanup()
    finally:
        print_batch_summary(
            folder, target_profile, converted, failed,
            skipped_existing, skipped_no_audio,
            interrupted=interrupted, interrupted_file=interrupted_file,
            skipped_audio_only_video=skipped_audio_only_video,
            skipped_target_compatible=skipped_target_compatible,
            skipped_container_mux=skipped_container_mux,
            skipped_container_ready=skipped_container_ready,
        )
    if interrupted:
        sys.exit(130)
    if failed:
        sys.exit(1)


if __name__ == "__main__":
    main()
