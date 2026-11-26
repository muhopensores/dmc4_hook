#!/usr/bin/env python3
"""Netplay TTD desync harness runner (see TTD_DESYNC_HARNESS.md).

Usage and the desync investigation workflow that uses the page observer are documented
in PAGE_WATCH_GUIDE.md.

`run`   build/deploy, launch DMC4 into a repeatable mission, gate the first stage on
        TTD recorder readiness, collect the GekkoNet desync diagnostics, and emit a
        machine-readable summary for the next edit/test iteration. `--direct` skips
        TTD entirely: the runner opens the recorder gate itself, no trace is recorded,
        and verdicts rest on events.log plus the page sidecar.
`open`  hand the finalized trace to non-elevated WinDbg so the agent can drive it
        through the already-registered official WinDbg MCP proxy.

Exit codes: 1 = valid-trace desync, 0 = valid-trace clean timeout or stage end,
2 = every setup/build/protocol/process/trace failure.

kernel32 on this OS build exports no CloseEvent, so the named-event handles the
runner creates are released implicitly when this process exits.
"""

import argparse
import ctypes
import datetime
import hashlib
import json
import os
import pathlib
import shutil
import subprocess
import sys
import time
import uuid

GAME_EXE_NAME = "DevilMayCry4_DX9.exe"
PROTOCOL = "DMC4NETPLAY"
DONE_REASONS = ("desync", "stage_end", "session_failed", "harness_error")
DIAGNOSTIC_DRAIN_SECONDS = 30.0
RECORDER_FINALIZE_WAIT_SECONDS = 120.0
EVENT_POLL_SLICE_MS = 1000
PAGE_WATCH_COPY_LIMIT_MIB_MIN = 1
PAGE_WATCH_COPY_LIMIT_MIB_MAX = 1024
PAGE_WATCH_RANGE_LIMIT = 32
PAGE_WATCH_ADDRESS_CEILING = 0x100000000


def parse_page_ranges(tokens):
    """Parse `BASE:SIZE` range tokens (hex, optional 0x) into sorted half-open spans."""
    if not tokens:
        return []
    spans = []
    for token in tokens:
        begin, sep, size = token.partition(":")
        if not sep:
            raise ValueError(f"--page-range must be BASE:SIZE, got {token!r}")
        try:
            base = int(begin, 16)
            length = int(size, 16)
        except ValueError:
            raise ValueError(f"--page-range {token!r} is not BASE:SIZE hex")
        if not (0 <= base < PAGE_WATCH_ADDRESS_CEILING and 0 < length <= 0x100000000):
            raise ValueError(f"--page-range {token!r} is outside the 32-bit address space")
        if base + length > PAGE_WATCH_ADDRESS_CEILING:
            raise ValueError(f"--page-range {token!r} wraps past the 32-bit address space")
        spans.append((base, base + length))
    if len(spans) > PAGE_WATCH_RANGE_LIMIT:
        raise ValueError(f"--page-range accepts at most {PAGE_WATCH_RANGE_LIMIT} ranges")
    merged = []
    for begin, end in sorted(spans):
        if merged and begin <= merged[-1][1]:
            if end > merged[-1][1]:
                merged[-1] = (merged[-1][0], end)
            continue
        merged.append((begin, end))
    if len(merged) > PAGE_WATCH_RANGE_LIMIT:
        raise ValueError(f"--page-range overlaps merge into {len(merged)} spans")
    return merged

kernel32 = ctypes.windll.kernel32


class Event:
    """One parsed harness protocol line."""

    def __init__(self, kind, fields=None, message=None):
        self.kind = kind
        self.fields = fields or {}
        self.message = message

    def __repr__(self):
        return f"Event(kind={self.kind!r}, fields={self.fields!r}, message={self.message!r})"

    def __eq__(self, other):
        return (isinstance(other, Event)
                and other.kind == self.kind
                and other.fields == self.fields
                and other.message == self.message)


def _decimal(token, field):
    if token.isdigit():
        return token
    raise ValueError(f"{field} must be decimal, got {token!r}")


def _hex8(token, field):
    if len(token) == 8 and all(c in "0123456789ABCDEF" for c in token):
        return token
    raise ValueError(f"{field} must be uppercase 8-digit hex, got {token!r}")


def _pair(token, field):
    key, sep, value = token.partition("=")
    if not sep or key != field:
        raise ValueError(f"expected {field}=, got {token!r}")
    return value


def parse_event_line(line, expected_run_id):
    """Parse one harness line for `expected_run_id`.

    Returns an Event, or None for unrelated lines and other runs' lines.
    Raises ValueError for a malformed line carrying the expected run id.
    """
    if not line.startswith(PROTOCOL + " "):
        return None
    run_id, _, rest = line[len(PROTOCOL) + 1:].partition(" ")
    if run_id != expected_run_id:
        return None
    kind, _, rest = rest.partition(" ")
    if kind == "LOG":
        return Event("LOG", {}, rest)
    if kind == "RECORDING":
        if rest:
            raise ValueError("RECORDING takes no payload")
        return Event("RECORDING", {}, None)
    if kind == "STAGE_WAIT":
        pid = _decimal(_pair(rest, "pid"), "pid")
        return Event("STAGE_WAIT", {"pid": pid}, None)
    if kind == "DESYNC":
        frame, _, rest = rest.partition(" ")
        local, _, rest = rest.partition(" ")
        remote, _, rest = rest.partition(" ")
        count, _, rest = rest.partition(" ")
        if rest:
            raise ValueError(f"unexpected DESYNC trailing text {rest!r}")
        fields = {
            "frame": _decimal(_pair(frame, "frame"), "frame"),
            "local": _hex8(_pair(local, "local"), "local"),
            "remote": _hex8(_pair(remote, "remote"), "remote"),
            "count": _decimal(_pair(count, "count"), "count"),
        }
        return Event("DESYNC", fields, None)
    if kind == "DONE":
        reason_tok, _, rest = rest.partition(" ")
        reason = _pair(reason_tok, "reason")
        if reason not in DONE_REASONS:
            raise ValueError(f"DONE reason must be one of {DONE_REASONS}, got {reason!r}")
        desyncs = _decimal(_pair(rest, "desyncs"), "desyncs")
        return Event("DONE", {"reason": reason, "desyncs": desyncs}, None)
    if kind == "ERROR":
        if not rest.startswith("message="):
            raise ValueError("ERROR payload must be message=")
        return Event("ERROR", {}, rest[len("message="):])
    raise ValueError(f"unknown harness event kind {kind!r}")


class RunState:
    """Accumulated run observations fed by the live monitor."""

    def __init__(self):
        self.desync_count = 0
        self.first_desync = None
        self.done_reason = None

    def consume(self, event):
        if event.kind == "DESYNC":
            self.desync_count += 1
            if self.first_desync is None:
                self.first_desync = {
                    "frame": int(event.fields["frame"]),
                    "local": event.fields["local"],
                    "remote": event.fields["remote"],
                }
        elif event.kind == "DONE":
            self.done_reason = event.fields["reason"]


def classify_done(run_state):
    """Map a DONE-observed run state onto (status, reason)."""
    reason = run_state.done_reason
    if reason == "desync":
        return "desync", "desync"
    if reason == "stage_end":
        return "pass", "stage_end"
    return "error", reason


PAGE_WATCH_FILENAME = "pages.jsonl"
PAGE_WATCH_ROW_KINDS = ("header", "arena", "baseline", "written", "changed", "coverage",
                        "event", "interval", "error", "end")
PAGE_WATCH_ERROR_CODES = ("invalid_config", "io_failed", "allocation_failed", "copy_limit",
                          "watch_reservation_failed", "write_watch_failed", "query_failed",
                          "no_watch_arenas", "unsupported_allocator_sites", "patch_failed")
PAGE_WATCH_MODES = ("hybrid",)
PAGE_WATCH_SCOPES = ("broad", "ranges")
PAGE_WATCH_RESERVATION_STATES = ("allocated", "decommitted", "released")
PAGE_WATCH_COVERAGE_STATES = ("added", "removed", "remapped", "unreadable")
PAGE_WATCH_EVENTS = ("session", "advance", "save", "load", "desync")
PAGE_WATCH_HARVESTS = ("boundary", "release")
PAGE_WATCH_ENDINGS = ("loop_entry", "done")


def _page_hex8(row, field):
    token = row[field]
    if (isinstance(token, str) and len(token) == 8
        and all(c in "0123456789ABCDEF" for c in token)):
        return token
    raise ValueError(f"{field} must be uppercase 8-digit hex, got {token!r}")


def _page_int(row, field, minimum):
    token = row.get(field)
    if isinstance(token, int) and not isinstance(token, bool) and token >= minimum:
        return token
    raise ValueError(f"{field} must be an integer >= {minimum}, got {token!r}")


def _page_bool(row, field):
    token = row.get(field)
    if isinstance(token, bool):
        return token
    raise ValueError(f"{field} must be a JSON boolean, got {token!r}")


def parse_page_row(text):
    """Parse and validate one `pages.jsonl` row; returns the decoded dict."""
    row = json.loads(text)
    if not isinstance(row, dict):
        raise ValueError(f"row must be a JSON object, got {row!r}")
    kind = row.get("kind")
    if kind not in PAGE_WATCH_ROW_KINDS:
        raise ValueError(f"unknown page row kind {kind!r}")
    if kind == "header":
        if _page_int(row, "version", 0) != 1:
            raise ValueError(f"header version must be 1, got {row['version']!r}")
        if row.get("mode") not in PAGE_WATCH_MODES:
            raise ValueError(f"header mode must be one of {PAGE_WATCH_MODES}")
        if row.get("scope") not in PAGE_WATCH_SCOPES:
            raise ValueError(f"header scope must be one of {PAGE_WATCH_SCOPES}")
        page_size = _page_int(row, "page_size", 1)
        _page_hex8(row, "image_base")
        _page_int(row, "image_size", 0)
        _page_int(row, "copy_limit_bytes", 1)
        ranges = row.get("ranges")
        if not isinstance(ranges, list):
            raise ValueError("header ranges must be a list")
        for span in ranges:
            begin, sep, end = str(span).partition(":")
            if (len(begin) != 8 or len(end) != 8
                or not all(c in "0123456789ABCDEF" for c in begin + end)
                or int(end, 16) <= int(begin, 16)):
                raise ValueError(f"range must be BBBBBBBB:SSSSSSSS with end > begin: {span!r}")
        if not isinstance(row.get("run_id"), str) or not row["run_id"]:
            raise ValueError("header run_id must be a non-empty string")
    elif kind == "arena":
        _page_int(row, "reservation_id", 1)
        _page_hex8(row, "base")
        _page_int(row, "bytes", 1)
        if row.get("state") not in PAGE_WATCH_RESERVATION_STATES:
            raise ValueError(f"arena state must be one of {PAGE_WATCH_RESERVATION_STATES}")
        _page_int(row, "loop", 0)
        if row.get("state") == "allocated":
            _page_hex8(row, "allocation_type")
    elif kind == "baseline":
        if _page_int(row, "loop", 1) != 1:
            raise ValueError(f"baseline loop must be 1, got {row['loop']!r}")
        for field in ("watched_bytes", "compared_bytes", "unreadable_pages", "copy_bytes"):
            _page_int(row, field, 0)
    elif kind in ("written", "changed", "coverage", "interval"):
        _page_int(row, "from_loop", 1)
        if kind == "written":
            _page_int(row, "reservation_id", 1)
            _page_hex8(row, "address")
            if row.get("harvest") not in PAGE_WATCH_HARVESTS:
                raise ValueError(f"written harvest must be one of {PAGE_WATCH_HARVESTS}")
        if kind == "changed":
            _page_hex8(row, "address")
            _page_hex8(row, "allocation_base")
            _page_hex8(row, "protection")
            _page_int(row, "changed_bytes", 1)
            _page_int(row, "first_offset", 0)
            before, after = row.get("before"), row.get("after")
            for name in (before, after):
                if (not isinstance(name, str) or len(name) % 2 != 0 or len(name) > 32
                    or not all(c in "0123456789ABCDEF" for c in name)):
                    raise ValueError("changed before/after must be uppercase hex, "
                                     "1..16 bytes, equal shape")
            if len(before) != len(after):
                raise ValueError("changed before/after windows must be the same length")
        if kind == "coverage":
            _page_hex8(row, "address")
            if row.get("state") not in PAGE_WATCH_COVERAGE_STATES:
                raise ValueError(f"coverage state must be one of {PAGE_WATCH_COVERAGE_STATES}")
        if kind == "interval":
            end = row.get("end")
            if end not in PAGE_WATCH_ENDINGS:
                raise ValueError(f"interval end must be one of {PAGE_WATCH_ENDINGS}")
            if end == "loop_entry":
                if _page_int(row, "to_loop", 1) != _page_int(row, "from_loop", 1) + 1:
                    raise ValueError("loop_entry interval to_loop must be from_loop + 1")
            elif row.get("to_loop") is not None:
                raise ValueError("done interval to_loop must be null")
            _page_bool(row, "partial")
            for field in ("written_page_observations", "changed_page_observations",
                          "changed_bytes", "watched_bytes", "compared_bytes",
                          "unreadable_pages", "copy_bytes"):
                _page_int(row, field, 0)
    elif kind == "error":
        if row.get("code") not in PAGE_WATCH_ERROR_CODES:
            raise ValueError(f"error code must be one of {PAGE_WATCH_ERROR_CODES}")
        _page_int(row, "win32_error", 0)
        _page_int(row, "loop", 0)
    elif kind == "end":
        if row.get("status") not in ("ok", "error"):
            raise ValueError(f"end status must be ok or error, got {row.get('status')!r}")
    elif kind == "event":
        if row.get("event") not in PAGE_WATCH_EVENTS:
            raise ValueError(f"event kind must be one of {PAGE_WATCH_EVENTS}")
        # Events can fire before the first move-loop entry (the stress session is
        # established by the recorder gate before any loop_begin), so loop 0 is legal
        # here and means a pre-loop event; every payload row stays loop >= 1.
        _page_int(row, "loop", 0)
        _page_int(row, "session", 1)
        if row["event"] in ("advance", "save", "load", "desync"):
            _page_int(row, "frame", -1)
    return row


def summarize_page_watch(path):
    """Two-pass streaming summary of one `pages.jsonl`, validating the row contract.

    Pass one establishes the header, reservation identities and loop boundaries;
    pass two re-reads the payload rows and compares them against the per-interval
    counters. Raises ValueError for a malformed or internally inconsistent sidecar.
    """
    try:
        with open(path, "r", encoding="utf-8") as handle:
            lines = [line for line in handle.read().split("\n") if line]
    except (OSError, UnicodeDecodeError) as exc:
        raise ValueError(f"cannot read sidecar: {exc}")
    if not lines:
        raise ValueError("sidecar is empty")
    rows = [parse_page_row(line) for line in lines]

    header = rows[0]
    if header["kind"] != "header":
        raise ValueError("first row must be the header")
    if sum(1 for row in rows if row["kind"] == "header") != 1:
        raise ValueError("exactly one header row")
    # The end row is written by finish(), which the DLL only reaches on a clean stop.
    # The runner kills the game at its timeout, so a sidecar without the end row is a
    # partial but valid observation; a present end row must be unique and last.
    end_rows = [row for row in rows if row["kind"] == "end"]
    partial = len(end_rows) == 0
    if len(end_rows) > 1 or (not partial and rows[-1]["kind"] != "end"):
        raise ValueError("exactly one end row, and it must be last")
    end_status = "missing" if partial else rows[-1]["status"]
    baselines = sum(1 for row in rows if row["kind"] == "baseline")
    errored = end_status == "error" or any(row["kind"] == "error" for row in rows)
    if baselines > 1 or (baselines == 0 and not errored):
        raise ValueError("expected exactly one baseline row, or none when the run errored")
    if any(row["kind"] == "error" and row["code"] not in PAGE_WATCH_ERROR_CODES for row in rows):
        raise ValueError("unknown error code")
    page_size = header["page_size"]
    image_base = int(header["image_base"], 16)
    image_size = header["image_size"]

    kinds = {}
    for row in rows:
        kinds[row["kind"]] = kinds.get(row["kind"], 0) + 1

    # Pass one: loop boundaries plus the reservation identity map.
    boundaries = []
    arena_spans = {}
    watched_total = {}
    errors = []
    previous_from_loop = 0
    for row in rows:
        kind = row["kind"]
        if kind == "arena":
            reservation_id = row["reservation_id"]
            if row["state"] == "allocated":
                arena_spans[reservation_id] = (int(row["base"], 16),
                                               int(row["base"], 16) + row["bytes"])
                watched_total[reservation_id] = row["bytes"]
            elif row["state"] == "released":
                watched_total[reservation_id] = 0
        elif kind == "interval":
            if row["from_loop"] <= previous_from_loop:
                raise ValueError("interval from_loop must strictly increase")
            if previous_from_loop != 0 and row["from_loop"] != previous_from_loop + 1:
                raise ValueError("interval rows must cover consecutive loops")
            previous_from_loop = row["from_loop"]
            boundaries.append(row)
        elif kind == "error":
            errors.append(row)
    if boundaries and sum(watched_total.values()) != boundaries[-1]["watched_bytes"]:
        raise ValueError("final interval watched_bytes disagrees with the arena rows")

    summary = {
        "path": str(path),
        "rows": len(rows),
        "kinds": kinds,
        "header": header,
        "end_status": end_status,
        "partial": partial,
        "errors": [{"code": row["code"], "win32_error": row["win32_error"],
                    "loop": row["loop"]} for row in errors],
        "loops": {"baseline": 1,
                  "max": boundaries[-1]["from_loop"] if boundaries else 1,
                  "interval_rows": len(boundaries),
                  "partial_rows": sum(1 for row in boundaries if row["partial"]),
                  "final_end": boundaries[-1]["end"] if boundaries else None,
                  "final_partial": boundaries[-1]["partial"] if boundaries else None},
        "reservations": {
            "ids": sorted(arena_spans),
            "count": len(arena_spans),
            "states": {},
            "watched_bytes_peak": max((row["watched_bytes"] for row in boundaries),
                                      default=0),
        },
        "written": {"rows": 0, "harvest": {}, "reservation_ids": set()},
        "coverage": {"rows": 0, "by_state": {}},
        "changed": {"rows": 0, "bytes": 0, "image_rows": 0, "largest_window_bytes": 0},
        "events": {"session": 0, "advance": 0, "save": 0, "load": 0, "desync": 0,
                   "negative_frames": 0, "pre_loop": 0},
        "counters": {},
        "violations": [],
    }
    for row in rows:
        if row["kind"] == "arena":
            states = summary["reservations"]["states"]
            states[row["state"]] = states.get(row["state"], 0) + 1

    # Pass two: payload rows grouped by the interval they belong to.
    per_interval = {row["from_loop"]: {"written": 0, "changed": 0, "changed_bytes": 0,
                                       "unreadable": 0} for row in boundaries}
    coverage_history = {}
    for row in rows:
        kind = row["kind"]
        if kind == "event":
            summary["events"][row["event"]] += 1
            if row["loop"] == 0:
                summary["events"]["pre_loop"] += 1
            if row.get("frame", 0) < 0:
                summary["events"]["negative_frames"] += 1
            continue
        if kind not in ("written", "changed", "coverage"):
            continue
        bucket = per_interval.get(row["from_loop"])
        if bucket is None:
            if not (partial and row["from_loop"] == previous_from_loop + 1):
                raise ValueError(f"payload row outside any interval: {row!r}")
            # The final interval was still open when the game was killed: its rows are
            # valid but unverified, because no interval row exists to compare the
            # per-interval counters against.
            bucket = {"written": 0, "changed": 0, "changed_bytes": 0, "unreadable": 0}
            per_interval[row["from_loop"]] = bucket
        if kind == "written":
            bucket["written"] += 1
            summary["written"]["rows"] += 1
            harvest = row["harvest"]
            summary["written"]["harvest"][harvest] = summary["written"]["harvest"].get(
                harvest, 0) + 1
            summary["written"]["reservation_ids"].add(row["reservation_id"])
            span = arena_spans.get(row["reservation_id"])
            address = int(row["address"], 16)
            if span is None or not (span[0] <= address < span[1]):
                raise ValueError(f"written row outside its reservation: {row!r}")
        elif kind == "changed":
            bucket["changed"] += 1
            bucket["changed_bytes"] += row["changed_bytes"]
            summary["changed"]["rows"] += 1
            summary["changed"]["bytes"] += row["changed_bytes"]
            summary["changed"]["largest_window_bytes"] = max(
                summary["changed"]["largest_window_bytes"], len(row["before"]) // 2)
            rva = row.get("image_rva")
            if rva is not None:
                summary["changed"]["image_rows"] += 1
                address = int(row["address"], 16)
                if not (image_base <= address < image_base + image_size):
                    raise ValueError(f"image_rva row is outside the image: {row!r}")
                if rva != address - image_base:
                    raise ValueError(f"image_rva must be address - image_base: {row!r}")
            elif image_size and image_base <= int(row["address"], 16) < image_base + image_size:
                summary["violations"].append(
                    f"page inside the image without an image_rva: {row['address']}")
            if row["first_offset"] + len(row["before"]) // 2 > page_size:
                raise ValueError(f"changed window exceeds the page: {row!r}")
        else:
            bucket["coverage_" + row["state"]] = bucket.get("coverage_" + row["state"], 0) + 1
            summary["coverage"]["rows"] += 1
            by_state = summary["coverage"]["by_state"]
            by_state[row["state"]] = by_state.get(row["state"], 0) + 1
            address = row["address"]
            previous = coverage_history.get(address)
            if row["state"] == "added":
                if previous == "added":
                    raise ValueError(f"duplicate added coverage without a removal: "
                                     f"{address}")
            elif previous == "removed":
                raise ValueError(f"coverage row after a removal: {address} {row['state']}")
            coverage_history[address] = row["state"]
    for row in boundaries:
        bucket = per_interval[row["from_loop"]]
        if bucket["written"] != row["written_page_observations"]:
            raise ValueError(f"interval written count disagrees: loop {row['from_loop']} "
                             f"observed {row['written_page_observations']} rows {bucket['written']}")
        if bucket["changed"] != row["changed_page_observations"]:
            raise ValueError(f"interval changed count disagrees: loop {row['from_loop']} "
                             f"observed {row['changed_page_observations']} rows "
                             f"{bucket['changed']}")
        if bucket["changed_bytes"] != row["changed_bytes"]:
            raise ValueError(f"interval changed_bytes disagrees: loop {row['from_loop']} "
                             f"observed {row['changed_bytes']} rows {bucket['changed_bytes']}")
        if bucket.get("coverage_unreadable", 0) != row["unreadable_pages"]:
            raise ValueError(f"interval unreadable_pages disagrees: loop {row['from_loop']} "
                             f"observed {row['unreadable_pages']} rows "
                             f"{bucket.get('coverage_unreadable', 0)}")
    if any(row["partial"] and row["end"] != "done" for row in boundaries):
        raise ValueError("only the done interval may be partial")
    live_copy = 0
    for row in boundaries:
        live_copy = row["copy_bytes"]
    summary["counters"] = ({key: sum(row[key] for row in boundaries)
                           for key in ("written_page_observations", "changed_page_observations",
                                       "changed_bytes", "compared_bytes", "unreadable_pages")}
                           | {"copy_bytes_final": live_copy})
    summary["written"]["reservation_ids"] = sorted(summary["written"]["reservation_ids"])
    summary["status"] = "ok"
    return summary

def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, "rb") as handle:
        for chunk in iter(lambda: handle.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def is_elevated():
    # IsUserAnAdmin is a shell32 export, not kernel32.
    return bool(ctypes.windll.shell32.IsUserAnAdmin())


def create_named_event(name):
    # CreateEventW(lpEventAttributes, bManualReset, bInitialState, lpName)
    return kernel32.CreateEventW(None, 1, 0, name)


def set_event(handle):
    return bool(kernel32.SetEvent(handle))


def wait_event(handle, milliseconds):
    return kernel32.WaitForSingleObjectEx(handle, int(milliseconds), False)


def wait_event_slice(handle, total_ms):
    """Wait for a named event in bounded slices so the caller can poll process exits."""
    remaining = int(total_ms)
    while remaining > 0:
        slice_ms = min(EVENT_POLL_SLICE_MS, remaining)
        if wait_event(handle, slice_ms) == 0:
            return True
        remaining -= slice_ms
    return False


def read_new_lines(path, offset, pending):
    """Incrementally read complete lines; keep the partial tail for the next call."""
    try:
        with open(path, "rb") as handle:
            handle.seek(offset)
            chunk = handle.read()
    except FileNotFoundError:
        return offset, pending, []
    if not chunk:
        return offset, pending, []
    offset += len(chunk)
    data = pending + chunk
    lines = data.split(b"\n")
    pending = lines.pop()
    return offset, pending, [line.decode("utf-8", "replace") for line in lines]


def atomic_replace(src, dst):
    tmp = dst.parent / f"{dst.name}.tmp-{os.getpid()}"
    try:
        shutil.copyfile(str(src), str(tmp))
        os.replace(str(tmp), str(dst))
    except OSError as exc:
        if tmp.exists():
            tmp.unlink(missing_ok=True)
        raise exc
    return True


def reap(process, timeout):
    """Wait for a process to exit, then terminate it; returns the exit code."""
    if process is None:
        return None
    code = None
    try:
        code = process.wait(timeout=timeout)
    except subprocess.TimeoutExpired:
        process.kill()
        try:
            code = process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            code = None
    return code


def cmd_run(args):
    run_dir = None
    game = None
    recorder = None
    game_log_handle = recorder_log_handle = build_log_handle = None
    game_exit_code = recorder_exit_code = None
    dll_sha256 = deployed_sha256 = None
    status, reason = "error", "build_failed"
    run_state = RunState()
    trace_path = events_log = recorder_log_path = None
    trace_bytes = 0
    exit_code = 2

    try:
        # ---- page-watch options are rejected before anything is built or deployed ----
        page_watch = bool(getattr(args, "page_watch", False))
        copy_limit_mib = getattr(args, "page_copy_limit_mib", 512)
        try:
            page_ranges = parse_page_ranges(getattr(args, "page_range", None) or [])
        except ValueError as exc:
            print(f"--page-range: {exc}", file=sys.stderr)
            return 2
        if not (PAGE_WATCH_COPY_LIMIT_MIB_MIN <= copy_limit_mib
                <= PAGE_WATCH_COPY_LIMIT_MIB_MAX):
            print(f"--page-copy-limit-mib must be an integer from "
                  f"{PAGE_WATCH_COPY_LIMIT_MIB_MIN} to {PAGE_WATCH_COPY_LIMIT_MIB_MAX} MiB, "
                  f"got {copy_limit_mib}", file=sys.stderr)
            return 2
        if (page_ranges or copy_limit_mib != 512) and not page_watch:
            print("--page-range/--page-copy-limit-mib apply only with --page-watch", file=sys.stderr)
            return 2

        # ---- preflight (before any build or deploy) ----
        if sys.platform != "win32":
            print("preflight: not Windows (sys.platform=%s)" % sys.platform, file=sys.stderr)
            return 2
        if not is_elevated():
            print("preflight: this Python process is not elevated "
                  "(shell32.IsUserAnAdmin is false); re-run from an elevated terminal",
                  file=sys.stderr)
            return 2
        game_dir = pathlib.Path(args.game_dir).resolve()
        game_exe = game_dir / GAME_EXE_NAME
        if not game_exe.exists():
            print("preflight: game executable missing: %s" % game_exe, file=sys.stderr)
            return 2
        direct = bool(getattr(args, "direct", False))
        ttd = None
        if not direct:
            ttd = args.ttd or os.environ.get("TTD_EXE") or shutil.which("TTD.exe")
            if not ttd or not pathlib.Path(ttd).exists():
                print("preflight: TTD executable missing: %s" % ttd, file=sys.stderr)
                return 2
        build_dir = pathlib.Path(args.build_dir).resolve()
        if not build_dir.is_dir():
            print("preflight: build directory missing: %s (run from the repository root "
                  "or pass an absolute --build-dir)" % build_dir, file=sys.stderr)
            return 2

        output_root = (pathlib.Path(args.output_root) if args.output_root
                       else build_dir / "netplay-runs")
        run_id = uuid.uuid4().hex
        stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        run_dir = (output_root / f"{stamp}-{run_id}").resolve()
        run_dir.mkdir(parents=True)

        events_log = run_dir / "events.log"
        game_log = run_dir / "game.log"
        recorder_log_path = None if direct else run_dir / "recorder.log"
        trace_path = None if direct else run_dir / "trace.run"
        pages_log = (run_dir / PAGE_WATCH_FILENAME) if page_watch else None

        # ---- build ----
        if args.build:
            build_log_handle = open(run_dir / "build.log", "w", encoding="utf-8", errors="replace")
            build = subprocess.run(
                ["cmake", "--build", str(build_dir), "--config", "Debug", "--target", "dmc4_hook"],
                stdout=build_log_handle, stderr=subprocess.STDOUT)
            if build.returncode != 0:
                reason = "build_failed"
                return 2

        dll_src = build_dir / "bin" / "dinput8.dll"
        pdb_src = build_dir / "bin" / "dinput8.pdb"
        if not dll_src.exists():
            reason = "build_failed"
            return 2
        dll_sha256 = sha256_file(dll_src)

        if args.deploy:
            if not pdb_src.exists():
                reason = "build_failed"
                return 2
            try:
                atomic_replace(dll_src, game_dir / "dinput8.dll")
                atomic_replace(pdb_src, game_dir / "dinput8.pdb")
            except OSError:
                reason = "build_failed"
                return 2
            deployed_sha256 = sha256_file(game_dir / "dinput8.dll")
        else:
            # --deploy stays optional: a manually copied or symlinked DLL is fine. The
            # summary still reports what the game will actually load, and a stale
            # in-place DLL is called out instead of silently mismatching the summary.
            target = game_dir / "dinput8.dll"
            if target.exists():
                deployed_sha256 = sha256_file(target)
                if deployed_sha256 != dll_sha256:
                    print(f"warning: {target} does not match {dll_src} (sha differs); "
                          "pass --deploy or refresh your symlink", file=sys.stderr)

        # ---- pre-create the recorder handshake events ----
        stage_name = f"dmc4-netplay-stage-{run_id}"
        record_name = f"dmc4-netplay-record-{run_id}"
        stage_ev = create_named_event(stage_name)
        record_ev = create_named_event(record_name)
        if not stage_ev or not record_ev:
            reason = "harness_error"
            return 2

        game_args = [
            "-mutex", f"dmc4-netplay-{run_id}",
            "-arcade", "-character", str(args.character),
            "-difficulty", str(args.difficulty), "-mission", str(args.mission),
            "-stress",
            "-netplay-run", run_id,
            "-netplay-log", str(events_log),
        ]
        if page_watch:
            game_args += ["-netplay-page-watch", "-netplay-page-log", str(pages_log),
                          "-netplay-page-copy-limit-mib", str(copy_limit_mib)]
            if page_ranges:
                game_args += ["-netplay-page-ranges",
                              ",".join(f"{begin:08X}:{end - begin:08X}"
                                       for begin, end in page_ranges)]
        if args.macro:
            game_args += ["-macro", str(pathlib.Path(args.macro).resolve())]
        game_args += list(args.game_arg)

        command = {
            "run_id": run_id,
            "run_dir": str(run_dir),
            "game_exe": str(game_exe),
            "game_dir": str(game_dir),
            "ttd": str(ttd) if ttd else None,
            "direct": direct,
            "build_dir": str(build_dir),
            "output_root": str(output_root),
            "mission": args.mission,
            "character": args.character,
            "difficulty": args.difficulty,
            "run_seconds": args.run_seconds,
            "startup_timeout_seconds": args.startup_timeout_seconds,
            "macro": str(pathlib.Path(args.macro).resolve()) if args.macro else None,
            "game_args": list(args.game_arg),
            "page_watch": page_watch,
            "page_log": str(pages_log) if pages_log else None,
            "page_ranges": [f"{begin:08X}:{end - begin:08X}"
                            for begin, end in page_ranges],
            "page_copy_limit_mib": copy_limit_mib if page_watch else None,
            "built": bool(args.build),
            "deployed": bool(args.deploy),
            "launch_command": [str(game_exe)] + game_args,
        }
        with open(run_dir / "command.json", "w", encoding="utf-8") as handle:
            json.dump(command, handle, indent=2)

        startup_deadline = time.monotonic() + args.startup_timeout_seconds

        game_log_handle = open(game_log, "wb")
        game = subprocess.Popen([str(game_exe)] + game_args, cwd=str(game_dir),
                                stdout=game_log_handle, stderr=subprocess.STDOUT)

        # ---- stage gate: the game blocks its stage callback until the recorder attaches ----
        remaining_ms = int((startup_deadline - time.monotonic()) * 1000)
        while remaining_ms > 0:
            if wait_event_slice(stage_ev, min(EVENT_POLL_SLICE_MS * 8, remaining_ms)):
                break
            if game.poll() is not None:
                game_exit_code = game.returncode
                reason = "game_exited"
                return 2
            remaining_ms = int((startup_deadline - time.monotonic()) * 1000)
        else:
            reason = "startup_timeout"
            return 2

        # ---- attach the recorder, or open the gate directly ----
        if direct:
            # No TTD: the DLL's stage gate blocks only on the record event. Setting it
            # lets the gate pass so the sidecar journal opens and RECORDING is emitted.
            set_event(record_ev)
        else:
            recorder_log_handle = open(recorder_log_path, "wb")
            recorder_argv = [str(ttd), "-accepteula", "-noUI", "-recordmode", "Automatic",
                             "-onInitCompleteEvent", record_name,
                             "-out", str(trace_path), "-attach", str(game.pid)]
            try:
                recorder = subprocess.Popen(recorder_argv,
                                            stdout=recorder_log_handle, stderr=subprocess.STDOUT)
            except PermissionError:
                # WindowsApps packages refuse CreateProcess on their own executables
                # (WinError 5 even when elevated): copy the image into the run
                # directory and launch that copy instead.
                local_ttd = run_dir / "ttd.exe"
                shutil.copy2(str(ttd), str(local_ttd))
                recorder_argv[0] = str(local_ttd)
                print("ttd relaunched from copy: %s" % local_ttd, file=sys.stderr)
                recorder = subprocess.Popen(recorder_argv,
                                            stdout=recorder_log_handle, stderr=subprocess.STDOUT)

        if not direct:
            remaining_ms = int((startup_deadline - time.monotonic()) * 1000)
            while remaining_ms > 0:
                if wait_event_slice(record_ev, min(EVENT_POLL_SLICE_MS * 8, remaining_ms)):
                    break
                if game.poll() is not None:
                    game_exit_code = game.returncode
                    reason = "game_exited"
                    return 2
                if recorder.poll() is not None:
                    recorder_exit_code = recorder.returncode
                    reason = "recorder_failed"
                    return 2
                remaining_ms = int((startup_deadline - time.monotonic()) * 1000)
            else:
                reason = "recorder_failed"
                return 2

        # ---- monitor events.log ----
        offset, pending = 0, b""
        clean_deadline = time.monotonic() + args.run_seconds
        drain_deadline = None
        protocol_error = None
        error_message = None
        done_seen = False
        desync_seen = False
        exited = None

        while True:
            offset, pending, lines = read_new_lines(events_log, offset, pending)
            for line in lines:
                try:
                    event = parse_event_line(line, run_id)
                except ValueError as exc:
                    protocol_error = str(exc)
                    break
                if event is None:
                    continue
                run_state.consume(event)
                if event.kind == "ERROR":
                    error_message = event.message
                if event.kind == "DESYNC" and not desync_seen:
                    desync_seen = True
                    drain_deadline = time.monotonic() + DIAGNOSTIC_DRAIN_SECONDS
                if event.kind == "DONE":
                    done_seen = True
                    break
            if done_seen or protocol_error:
                break

            now = time.monotonic()
            if game.poll() is not None:
                game_exit_code = game.returncode
                exited = "game"
                break
            if recorder is not None and recorder.poll() is not None:
                recorder_exit_code = recorder.returncode
                exited = "recorder"
                break
            if drain_deadline is not None:
                if now >= drain_deadline:
                    reason = "diagnostic_timeout"
                    return 2
            elif now >= clean_deadline:
                status, reason = "pass", "timeout"
                return 2
            time.sleep(0.05)

        if protocol_error:
            reason = "protocol_error"
            return 2
        if error_message:
            reason = "harness_error"
            return 2
        if done_seen:
            status, reason = classify_done(run_state)
            return 2
        reason = "game_exited" if exited == "game" else "recorder_failed"
        return 2

    except KeyboardInterrupt:
        reason = "interrupted"
    except Exception as exc:
        import traceback
        print("harness runner fault:", repr(exc))
        traceback.print_exc()
        status, reason = "error", "harness_error"
    finally:
        # ---- reap: TTD finalizes the attach recording when the recorded process exits ----
        if game is not None and game.poll() is None:
            game.kill()
        game_exit_code = reap(game, 30.0)
        recorder_exit_code = reap(recorder, RECORDER_FINALIZE_WAIT_SECONDS)

        for handle in (build_log_handle, game_log_handle, recorder_log_handle):
            if handle:
                handle.close()

        trace_bytes = trace_path.stat().st_size if (trace_path and trace_path.exists()) else 0
        trace_err = bool(trace_path) and pathlib.Path(str(trace_path) + ".err").exists()
        recorder_text = ""
        if recorder_log_path:
            try:
                recorder_text = recorder_log_path.read_text(encoding="utf-8", errors="replace")
            except FileNotFoundError:
                recorder_text = ""
        trace_valid = (trace_bytes > 0 and not trace_err
                       and "Full trace dumped to" in recorder_text)

        if not direct:
            # Without TTD there is no trace by design; desync and pass verdicts rest on
            # events.log (DESYNC/detail/DONE lines) plus the page sidecar alone.
            if status == "desync" and not trace_valid:
                status, reason = "error", "trace_invalid"
            elif status == "pass" and not trace_valid:
                status, reason = "error", "trace_invalid"


        page_watch_summary = None
        if page_watch:
            if pages_log is None or not pages_log.exists():
                page_watch_summary = {"path": str(pages_log), "status": "missing"}
                status, reason = "error", "page_watch_missing"
            else:
                try:
                    page_watch_summary = summarize_page_watch(pages_log)
                except ValueError as exc:
                    page_watch_summary = {"path": str(pages_log), "status": "contract_error",
                                          "error": str(exc)}
                    status, reason = "error", "page_watch_contract"
                else:
                    if page_watch_summary["end_status"] == "error":
                        page_watch_summary["status"] = "error"
                    elif page_watch_summary["end_status"] == "missing":
                        # The game was killed before finish(): the sidecar is a partial
                        # observation, not a contract violation.
                        page_watch_summary["status"] = "partial"
                    else:
                        page_watch_summary["status"] = "ok"
        summary = {
            "status": status,
            "reason": reason,
            "run_dir": str(run_dir) if run_dir else None,
            "trace": str(trace_path) if (trace_path and trace_bytes > 0) else None,
            "events_log": str(events_log) if (events_log and events_log.exists()) else None,
            "recorder_log": str(recorder_log_path) if recorder_log_path else None,
            "trace_bytes": trace_bytes,
            "desync_count": run_state.desync_count,
            "first_desync": run_state.first_desync,
            "game_exit_code": game_exit_code,
            "recorder_exit_code": recorder_exit_code,
            "dll_sha256": dll_sha256,
            "deployed_dll_sha256": deployed_sha256,
            "page_watch": page_watch_summary,
        }
        if run_dir:
            with open(run_dir / "summary.json", "w", encoding="utf-8") as handle:
                json.dump(summary, handle, indent=2)
        print(json.dumps(summary, indent=2))

        if status == "desync":
            exit_code = 1
        elif status == "pass":
            exit_code = 0
        else:
            exit_code = 2
    return exit_code


def cmd_open(args):
    if is_elevated():
        # The official WinDbg MCP proxy gets Access denied when WinDbg runs as
        # administrator, so replay must start from a non-elevated terminal.
        print(json.dumps({"status": "error", "reason": "elevated",
                          "hint": "re-run `open` from a non-elevated terminal"}, indent=2))
        return 2
    run_dir = pathlib.Path(args.run_dir).resolve()
    trace = run_dir / "trace.run"
    recorder_log = run_dir / "recorder.log"
    trace_bytes = trace.stat().st_size if trace.exists() else 0
    trace_err = pathlib.Path(str(trace) + ".err").exists()
    recorder_text = ""
    try:
        recorder_text = recorder_log.read_text(encoding="utf-8", errors="replace")
    except FileNotFoundError:
        recorder_text = ""
    if not (trace_bytes > 0 and not trace_err and "Full trace dumped to" in recorder_text):
        print(json.dumps({"status": "error", "reason": "trace_invalid",
                          "run_dir": str(run_dir)}, indent=2))
        return 2

    windbg = args.windbg or os.environ.get("WINDBG_EXE") or shutil.which("WinDbgX")
    if not windbg:
        print(json.dumps({"status": "error", "reason": "windbg_missing"}, indent=2))
        return 2

    symbols = args.symbols
    if not symbols:
        try:
            with open(run_dir / "command.json", encoding="utf-8") as handle:
                command = json.load(handle)
            symbols = str(pathlib.Path(command["build_dir"]) / "bin")
        except (OSError, KeyError, json.JSONDecodeError):
            symbols = None
    if not symbols or not pathlib.Path(symbols).is_dir():
        print(json.dumps({"status": "error", "reason": "symbols_missing"}, indent=2))
        return 2

    proxy = None
    for candidate in (pathlib.Path(".mcp.json"),
                      pathlib.Path(__file__).resolve().parents[1] / ".mcp.json"):
        try:
            with open(candidate, encoding="utf-8") as handle:
                proxy = json.load(handle)["mcpServers"]["WinDbg"]["command"]
            break
        except (OSError, KeyError, json.JSONDecodeError):
            continue

    argv = [str(windbg), "-z", str(trace), "-y", str(symbols)]
    process = subprocess.Popen(argv)
    payload = {
        "status": "opened",
        "windbg_pid": process.pid,
        "windbg_command": argv,
        "trace": str(trace),
        "symbols": str(symbols),
        "mcp_proxy": proxy,
    }
    with open(run_dir / "windbg.json", "w", encoding="utf-8") as handle:
        json.dump(payload, handle, indent=2)
    print(json.dumps(payload, indent=2))
    return 0


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)

    run = sub.add_parser("run", help="record one mission run and report a summary")
    run.add_argument("--game-dir", required=True)
    run.add_argument("--ttd")
    run.add_argument("--direct", action="store_true",
                     help="launch the game without TTD: no trace is recorded and desync/"
                          "pass verdicts rest on events.log and the page sidecar alone")
    run.add_argument("--build-dir", default="build_ninja")
    run.add_argument("--output-root")
    run.add_argument("--build", action="store_true")
    run.add_argument("--deploy", action="store_true")
    run.add_argument("--run-seconds", type=float, default=60.0)
    run.add_argument("--startup-timeout-seconds", type=float, default=180.0)
    run.add_argument("--mission", type=int, default=12)
    run.add_argument("--character", type=int, default=6)
    run.add_argument("--difficulty", type=int, default=7)
    run.add_argument("--macro")
    run.add_argument("--game-arg", action="append", default=[])
    run.add_argument("--page-watch", action="store_true",
                     help="observe written pages of the game's allocator reservations "
                          "and write the pages.jsonl sidecar beside events.log")
    run.add_argument("--page-range", action="append", default=None, metavar="BASE:SIZE",
                     help="restrict comparison to a hex BASE:SIZE span (repeatable, "
                          "at most 32); requires --page-watch")
    run.add_argument("--page-copy-limit-mib", type=int, default=512, metavar="N",
                     help="fallback-copy byte budget in MiB (1..1024, default 512); "
                          "requires --page-watch")
    run.set_defaults(handler=cmd_run)

    openp = sub.add_parser("open", help="open a finalized trace in non-elevated WinDbg")
    openp.add_argument("--run-dir", required=True)
    openp.add_argument("--windbg")
    openp.add_argument("--symbols")
    openp.set_defaults(handler=cmd_open)

    args = parser.parse_args(argv[1:])
    return args.handler(args)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
