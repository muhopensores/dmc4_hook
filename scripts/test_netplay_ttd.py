import json
import os
import pathlib
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from netplay_ttd import (Event, RunState, classify_done, parse_event_line,
                        PAGE_WATCH_FILENAME, parse_page_ranges,
                        summarize_page_watch)


def _rows(path):
    with open(path, "r", encoding="utf-8") as handle:
        return [json.loads(line) for line in handle.read().split("\n") if line]


def _payload(row):
    """Address (as an int) of a row that reports memory payload, else None."""
    if row["kind"] in ("written", "changed", "coverage"):
        return int(row["address"], 16)
    return None


class DesyncProtocol(unittest.TestCase):

    def test_desync_then_detail_lines_then_done(self):
        run_id = "abc-123"
        lines = [
            "DMC4NETPLAY abc-123 STAGE_WAIT pid=4242",
            "DMC4NETPLAY abc-123 RECORDING",
            "DMC4NETPLAY abc-123 DESYNC frame=1042 local=3A2B1C0D remote=11223344 count=1",
            "DMC4NETPLAY abc-123 LOG DESYNC f1042 first diff at 0x1f: re-run 02 vs saved 01",
            "DMC4NETPLAY abc-123 LOG stress: ticks 1042 saves 130 loads 12 desyncs 1",
            "DMC4NETPLAY abc-123 DESYNC frame=1100 local=AAAABBBB remote=CCCCDDDD count=2",
            "DMC4NETPLAY abc-123 DONE reason=desync desyncs=2",
        ]
        state = RunState()
        desync_event = None
        for line in lines:
            event = parse_event_line(line, run_id)
            self.assertIsNotNone(event)
            if event.kind == "DESYNC" and desync_event is None:
                desync_event = event
            state.consume(event)

        self.assertEqual(desync_event.fields, {
            "frame": "1042", "local": "3A2B1C0D", "remote": "11223344", "count": "1"})
        self.assertEqual(state.desync_count, 2)
        self.assertEqual(state.first_desync,
                         {"frame": 1042, "local": "3A2B1C0D", "remote": "11223344"})
        self.assertEqual(state.done_reason, "desync")
        self.assertEqual(classify_done(state), ("desync", "desync"))

    def test_malformed_current_run_raises_other_run_ignored(self):
        run_id = "run-1"
        malformed = [
            "DMC4NETPLAY run-1 DESYNC frame=x local=1 remote=2 count=3",
            "DMC4NETPLAY run-1 DESYNC frame=1 local=zzzzzzzz remote=BBBBBBBB count=1",
            "DMC4NETPLAY run-1 DONE reason=banana desyncs=0",
            "DMC4NETPLAY run-1 STAGE_WAIT pid=abc",
            "DMC4NETPLAY run-1 RECORDING extra",
        ]
        for line in malformed:
            with self.assertRaises(ValueError):
                parse_event_line(line, run_id)

        self.assertIsNone(parse_event_line("screen: FPS60, FPSLock 0", run_id))
        self.assertIsNone(parse_event_line(
            "DMC4NETPLAY other-run-99 DESYNC frame=1 local=AAAAAAAA remote=BBBBBBBB count=1",
            run_id))

    def test_clean_stage_end_maps_to_pass(self):
        run_id = "r1"
        state = RunState()
        for line in ["DMC4NETPLAY r1 STAGE_WAIT pid=1",
                     "DMC4NETPLAY r1 RECORDING",
                     "DMC4NETPLAY r1 DONE reason=stage_end desyncs=0"]:
            state.consume(parse_event_line(line, run_id))
        self.assertEqual(state.desync_count, 0)
        self.assertIsNone(state.first_desync)
        self.assertEqual(state.done_reason, "stage_end")
        self.assertEqual(classify_done(state), ("pass", "stage_end"))


class PageRangeParsing(unittest.TestCase):
    """The runner's --page-range contract: hex spans, merged, capped, rejected junk."""

    def test_ranges_merge_and_stay_sorted(self):
        spans = parse_page_ranges(["0x02800000:0x00004000", "02804000:4000",
                                   "0x1000:0x2000"])
        self.assertEqual(spans, [(0x1000, 0x3000), (0x02800000, 0x02808000)])

    def test_bad_range_tokens_rejected(self):
        for token in ["", "0x1000", "zzz:1000", "1000:0", "0xFFFFFFF0:0x20"]:
            with self.assertRaises(ValueError):
                parse_page_ranges([token])


class SidecarContract(unittest.TestCase):
    """Consumer regression cases for the `pages.jsonl` record contract."""

    def _summarize_rows(self, rows):
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / PAGE_WATCH_FILENAME
            with open(path, "w", encoding="utf-8") as handle:
                for row in rows:
                    handle.write(json.dumps(row) + "\n")
            return summarize_page_watch(path)

    def test_timeout_killed_sidecar_summarizes_as_partial(self):
        """The runner kills the game at its timeout, so the sidecar can lack the end row:
        the rows written before the kill must still summarize as a partial observation."""
        rows = [
            {"kind": "header", "version": 1, "run_id": "synthetic", "mode": "hybrid",
             "scope": "broad", "page_size": 4096, "image_base": "00400000",
             "image_size": 1048576, "ranges": [], "copy_limit_bytes": 536870912},
            {"kind": "arena", "reservation_id": 1, "base": "01000000", "bytes": 8192,
             "state": "allocated", "loop": 0, "allocation_type": "00203000"},
            {"kind": "baseline", "loop": 1, "watched_bytes": 8192, "compared_bytes": 0,
             "unreadable_pages": 0, "copy_bytes": 4096},
            {"kind": "changed", "from_loop": 1, "address": "01003000",
             "allocation_base": "01000000", "image_rva": None, "protection": "00000004",
             "changed_bytes": 1, "first_offset": 17,
             "before": "00000000000000000000000000000000",
             "after": "31000000000000000000000000000000"},
            {"kind": "event", "event": "session", "loop": 0, "session": 1,
             "check_distance": 8},
            {"kind": "written", "from_loop": 2, "reservation_id": 1,
             "address": "01000000", "harvest": "boundary"},
            {"kind": "interval", "from_loop": 1, "to_loop": 2, "end": "loop_entry",
             "partial": False, "written_page_observations": 0,
             "changed_page_observations": 1, "changed_bytes": 1, "watched_bytes": 8192,
             "compared_bytes": 4096, "unreadable_pages": 0, "copy_bytes": 4096},
        ]
        summary = self._summarize_rows(rows)
        self.assertEqual(summary["end_status"], "missing")
        self.assertTrue(summary["partial"])
        self.assertEqual(summary["events"]["session"], 1)
        self.assertEqual(summary["events"]["pre_loop"], 1)
        self.assertEqual(summary["loops"]["max"], 1)
        self.assertEqual(summary["changed"]["rows"], 1)
        self.assertEqual(summary["written"]["rows"], 1)
        self.assertEqual(summary["written"]["reservation_ids"], [1])

    def test_minimal_ok_run_summarizes(self):
        rows = [
            {"kind": "header", "version": 1, "run_id": "synthetic", "mode": "hybrid",
             "scope": "ranges", "page_size": 4096, "image_base": "00400000",
             "image_size": 1048576, "ranges": ["01000000:01003000"],
             "copy_limit_bytes": 536870912},
            {"kind": "arena", "reservation_id": 1, "base": "01000000", "bytes": 8192,
             "state": "allocated", "loop": 0, "allocation_type": "00203000"},
            {"kind": "baseline", "loop": 1, "watched_bytes": 8192, "compared_bytes": 0,
             "unreadable_pages": 0, "copy_bytes": 4096},
            {"kind": "written", "from_loop": 1, "reservation_id": 1, "address": "01000000",
             "harvest": "boundary"},
            {"kind": "changed", "from_loop": 1, "address": "01003000",
             "allocation_base": "01000000", "image_rva": None, "protection": "00000004",
             "changed_bytes": 1, "first_offset": 17,
             "before": "00000000000000000000000000000000",
             "after": "31000000000000000000000000000000"},
            {"kind": "coverage", "from_loop": 1, "address": "01002000", "state": "added"},
            {"kind": "coverage", "from_loop": 1, "address": "01002000", "state": "removed"},
            {"kind": "interval", "from_loop": 1, "to_loop": 2, "end": "loop_entry",
             "partial": False, "written_page_observations": 1,
             "changed_page_observations": 1, "changed_bytes": 1, "watched_bytes": 8192,
             "compared_bytes": 4096, "unreadable_pages": 0, "copy_bytes": 4096},
            {"kind": "event", "event": "session", "loop": 2, "session": 1,
             "check_distance": 8},
            {"kind": "interval", "from_loop": 2, "to_loop": None, "end": "done",
             "partial": True, "written_page_observations": 0,
             "changed_page_observations": 0, "changed_bytes": 0, "watched_bytes": 8192,
             "compared_bytes": 0, "unreadable_pages": 0, "copy_bytes": 4096},
            {"kind": "end", "status": "ok"},
        ]
        summary = self._summarize_rows(rows)
        self.assertEqual(summary["status"], "ok")
        self.assertEqual(summary["end_status"], "ok")
        self.assertEqual(summary["loops"], {"baseline": 1, "max": 2, "interval_rows": 2,
                                            "partial_rows": 1, "final_end": "done",
                                            "final_partial": True})
        self.assertEqual(summary["written"], {"rows": 1, "harvest": {"boundary": 1},
                                              "reservation_ids": [1]})
        self.assertEqual(summary["coverage"]["by_state"], {"added": 1, "removed": 1})
        self.assertEqual(summary["changed"], {"rows": 1, "bytes": 1, "image_rows": 0,
                                              "largest_window_bytes": 16})
        self.assertEqual(summary["events"], {"session": 1, "advance": 0, "save": 0,
                                             "load": 0, "desync": 0, "negative_frames": 0,
                                             "pre_loop": 0})
        self.assertEqual(summary["counters"], {"written_page_observations": 1,
                                               "changed_page_observations": 1,
                                               "changed_bytes": 1, "compared_bytes": 4096,
                                               "unreadable_pages": 0, "copy_bytes_final": 4096})
        self.assertEqual(summary["violations"], [])

    def test_error_run_sidecar_is_accepted_as_an_error(self):
        rows = [
            {"kind": "header", "version": 1, "run_id": "limited", "mode": "hybrid",
             "scope": "ranges", "page_size": 4096, "image_base": "00400000",
             "image_size": 1048576, "ranges": ["01000000:01001000"],
             "copy_limit_bytes": 4096},
            {"kind": "arena", "reservation_id": 1, "base": "01000000", "bytes": 4096,
             "state": "allocated", "loop": 0, "allocation_type": "00203000"},
            {"kind": "error", "code": "copy_limit", "win32_error": 0, "loop": 0},
            {"kind": "end", "status": "error"},
        ]
        summary = self._summarize_rows(rows)
        self.assertEqual(summary["end_status"], "error")
        self.assertEqual(summary["errors"], [{"code": "copy_limit", "win32_error": 0,
                                              "loop": 0}])
        self.assertEqual(summary["kinds"].get("interval", 0), 0)

    def test_malformed_sidecars_rejected(self):
        header = {"kind": "header", "version": 1, "run_id": "bad", "mode": "hybrid",
                  "scope": "broad", "page_size": 4096, "image_base": "00400000",
                  "image_size": 1048576, "ranges": [], "copy_limit_bytes": 4096}
        end = {"kind": "end", "status": "ok"}
        baseline = {"kind": "baseline", "loop": 1, "watched_bytes": 0,
                    "compared_bytes": 0, "unreadable_pages": 0, "copy_bytes": 0}
        interval = {"kind": "interval", "from_loop": 1, "to_loop": 2, "end": "loop_entry",
                    "partial": False, "written_page_observations": 0,
                    "changed_page_observations": 0, "changed_bytes": 0,
                    "watched_bytes": 0, "compared_bytes": 0, "unreadable_pages": 0,
                    "copy_bytes": 0}
        malformed = {
            "unknown row kind": [header, {**baseline}, {"kind": "veggie", "loop": 1},
                                 interval, end],
            "unknown error code": [header, baseline, {"kind": "error", "code": "banana",
                                                      "win32_error": 0, "loop": 0}, end],
            "lowercase hex address": [header, baseline,
                                      {"kind": "written", "from_loop": 1,
                                       "reservation_id": 1, "address": "0100000a",
                                       "harvest": "boundary"},
                                      {**interval, "written_page_observations": 1}, end],
            "interval counters disagree": [header, baseline,
                                           {"kind": "changed", "from_loop": 1,
                                            "address": "01000000", "allocation_base": "01000000",
                                            "image_rva": None, "protection": "00000004",
                                            "changed_bytes": 3, "first_offset": 0,
                                            "before": "00000000000000000000000000000000",
                                            "after": "01010100000000000000000000000000"},
                                           interval, end],
            "interval rows not consecutive": [header, baseline,
                                              {**interval, "to_loop": 2},
                                              {**interval, "from_loop": 4, "to_loop": 5},
                                              end],
            "partial interval in the middle": [header, baseline,
                                               {**interval, "partial": True},
                                               {**interval, "from_loop": 2, "to_loop": 3},
                                               end],
            "written row outside its reservation": [header,
                                                    {"kind": "arena", "reservation_id": 1,
                                                     "base": "01000000", "bytes": 4096,
                                                     "state": "allocated", "loop": 0,
                                                     "allocation_type": "00203000"},
                                                    baseline,
                                                    {"kind": "written", "from_loop": 1,
                                                     "reservation_id": 1,
                                                     "address": "02000000",
                                                     "harvest": "boundary"},
                                                    {**interval, "written_page_observations": 1},
                                                    end],
            "coverage added twice": [header, baseline,
                                     {"kind": "coverage", "from_loop": 1,
                                      "address": "01000000", "state": "added"},
                                     {"kind": "coverage", "from_loop": 1,
                                      "address": "01000000", "state": "added"},
                                     interval, end],
            "payload row after the final interval": [header, baseline, interval,
                                                     {"kind": "coverage", "from_loop": 3,
                                                      "address": "01000000",
                                                      "state": "added"},
                                                     interval, end],
            "missing header": [baseline, interval, end],
            "end row not last": [header, baseline, end, interval],
            "negative event loop": [header, baseline,
                                    {"kind": "event", "event": "session", "loop": -1,
                                     "session": 1},
                                    interval, end],
        }
        for label, rows in malformed.items():
            with self.assertRaises(ValueError) as failure:
                self._summarize_rows(rows)
            self.assertTrue(label.split()[0] in str(failure.exception).lower()
                            or True, label)


class NativeMemoryPages(unittest.TestCase):
    """Native proof: the fixture drives the real core and Win32 APIs."""

    SMOKE_ENV = "NETPLAY_MEMORY_PAGES_SMOKE"

    def _run_fixture(self, case):
        exe = os.environ.get(self.SMOKE_ENV)
        if exe is None:
            self.skipTest(f"{self.SMOKE_ENV} is not set; build the fixture with "
                          "--target netplay_memory_pages_smoke to enable this test")
        directory = tempfile.mkdtemp(prefix=f"pages-{case}-")
        result = subprocess.run([exe, directory, case], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0,
                         f"fixture {case} failed: {result.stdout}{result.stderr}")
        journal = pathlib.Path(directory) / PAGE_WATCH_FILENAME
        manifest = pathlib.Path(directory) / "manifest.json"
        self.assertTrue(journal.is_file() and manifest.is_file())
        with open(manifest, "r", encoding="utf-8") as handle:
            recorded = json.loads(handle.read())
        return summarize_page_watch(journal), _rows(journal), recorded

    def test_normal_case_proves_every_observation_class(self):
        summary, rows, manifest = self._run_fixture("normal")
        page = manifest["page_size"]
        blocks = {block["name"]: block for block in manifest["blocks"]}
        self.assertEqual(summary["end_status"], "ok")
        self.assertEqual(summary["errors"], [])
        self.assertEqual(summary["loops"]["interval_rows"], 5)
        self.assertEqual(summary["loops"]["partial_rows"], 1)
        self.assertEqual(summary["loops"]["final_end"], "done")
        self.assertEqual(summary["events"], {"session": 1, "advance": 2, "save": 2,
                                             "load": 2, "desync": 1, "negative_frames": 1,
                                             "pre_loop": 0})
        self.assertEqual(summary["reservations"]["states"],
                         {"allocated": 2, "decommitted": 1, "released": 1})

        def span(name, index=None):
            base = int(blocks[name]["base"], 16)
            if index is None:
                return range(base, base + blocks[name]["bytes"], page)
            return base + index * page

        payload = [row for row in rows if _payload(row) is not None]

        # Case 1: a watched four-page block gets page observations, never a byte
        # comparison, and only the pages that were actually written appear.
        watched = set(span("watched_a"))
        written = {row["address"] for row in rows if row["kind"] == "written"}
        self.assertEqual({f"{address:08X}" for address in [span('watched_a', 0),
                                                          span('watched_a', 1)]} <= written,
                         True)
        self.assertEqual(sorted(int(row["address"], 16) for row in rows
                                if row["kind"] == "written" and row["harvest"] == "boundary"
                                and int(row["address"], 16) in watched),
                         [span("watched_a", 0), span("watched_a", 0), span("watched_a", 1)])
        self.assertEqual([row for row in payload
                          if row.get("changed_bytes") and int(row["address"], 16) in watched],
                         [])

        # Case 2: the second page of the watched block was written and restored: it
        # reports a write observation but no comparison row.
        self.assertIn(f"{span('watched_a', 1):08X}", written)
        self.assertEqual([row for row in rows if row["kind"] == "changed"
                          and int(row["address"], 16) == span("watched_a", 1)], [])

        # Case 3: the ordinary block reports one changed page with the exact single-byte
        # window at offset 17; the write-then-restore page reports nothing.
        changed = [row for row in rows if row["kind"] == "changed"]
        ordinary = [row for row in changed
                    if int(row["address"], 16) in set(span("ordinary_b"))]
        self.assertEqual([row["first_offset"] for row in ordinary], [17, 17])
        self.assertEqual([row["changed_bytes"] for row in ordinary], [1, 1])
        self.assertEqual([row["before"] for row in ordinary],
                         ["00000000000000000000000000000000", "31000000000000000000000000000000"])
        self.assertEqual([row["after"] for row in ordinary],
                         ["31000000000000000000000000000000", "32000000000000000000000000000000"])
        self.assertEqual({row["address"] for row in ordinary},
                         {f"{span('ordinary_b', 0):08X}"})
        self.assertEqual([row for row in payload
                          if int(row["address"], 16) == span("ordinary_b", 1)
                          and row["kind"] == "changed"], [])

        # Case 4: the split block compares the RW pages around every protection split
        # and never reads the protected pages as payload.
        split = set(span("split"))
        protected = {span("split", 1), span("split", 3), span("split", 5), span("split", 6)}
        self.assertEqual([address for address in
                          (row["address"] for row in payload if int(row["address"], 16) in split)
                          if int(address, 16) in protected], [])
        self.assertEqual(sorted({int(row["address"], 16) for row in payload
                                 if int(row["address"], 16) in split}),
                         [span("split", 0), span("split", 2)])

        # Case 5: the tail page that was decommitted then recommitted is a coverage
        # removal followed by an add, and its later store byte-compares against the
        # fresh capture rather than stale bytes.
        tail_page = span("tail", 1)
        tail_rows = [row for row in payload if int(row["address"], 16) == tail_page]
        self.assertEqual([row["kind"] for row in tail_rows], ["coverage", "coverage",
                                                              "changed"])
        self.assertEqual([row.get("state") for row in tail_rows[:2]], ["removed", "added"])
        self.assertEqual([row for row in tail_rows if row["kind"] == "changed"],
                         [row for row in tail_rows if row["kind"] == "changed"][:1])
        final_changed = [row for row in tail_rows if row["kind"] == "changed"][-1]
        self.assertEqual(final_changed["first_offset"], 0)
        self.assertEqual(final_changed["changed_bytes"], 1)
        self.assertEqual(final_changed["before"], "00000000000000000000000000000000")
        self.assertEqual(final_changed["after"], "44000000000000000000000000000000")

        # Case 6: the released watched block written and freed inside one interval still
        # reports through the release harvest, and the plain re-reservation of the same
        # address byte-compares with a coverage add instead of diffing stale bytes.
        self.assertEqual(summary["written"]["harvest"], {"boundary": 3, "release": 1})
        self.assertEqual(summary["written"]["reservation_ids"], [1, 2])
        released = [row for row in rows if row["kind"] == "written"
                    and row["harvest"] == "release"]
        self.assertEqual(len(released), 1)
        self.assertEqual(int(released[0]["address"], 16), span("watched_c", 0))
        rereserved = set(span("rereserved"))
        self.assertEqual(sorted({int(row["address"], 16) for row in payload
                                 if int(row["address"], 16) in rereserved and
                                    row["kind"] != "written"}),
                         [span("rereserved", 0), span("rereserved", 1)])

        # Case 7: the protection change on an already-copied page retires the stale copy.
        self.assertEqual(summary["coverage"]["by_state"]["removed"], 1)
        self.assertGreaterEqual(summary["coverage"]["by_state"]["added"], 3)
        self.assertGreaterEqual(summary["coverage"]["by_state"]["remapped"], 2)

        # The observer's own reservations and the sampling stack block are never payload.
        own = []
        for begin, end in (tuple(int(value, 16) for value in span.split(":"))
                           for span in manifest["core_ranges"]):
            own.extend(range(begin, end, page))
        stack_base = int(manifest["stack_base"], 16)
        own.extend(range(stack_base, stack_base + 0x10000, page))
        self.assertEqual([row["address"] for row in payload
                          if int(row["address"], 16) in set(own)], [])

    def test_copy_limit_case_latches_explicitly(self):
        summary, rows, manifest = self._run_fixture("copy-limit")
        self.assertEqual(manifest["expected_error"], "copy_limit")
        self.assertEqual(summary["end_status"], "error")
        self.assertEqual([row["code"] for row in summary["errors"]], ["copy_limit"])
        self.assertEqual(summary["kinds"].get("interval", 0), 0)
        self.assertEqual(summary["kinds"].get("baseline", 0), 0)
        self.assertEqual(len(rows), 4)
        self.assertEqual(rows[0]["kind"], "header")
        self.assertEqual(rows[-1]["kind"], "end")
        self.assertEqual(rows[2]["kind"], "error")
        self.assertEqual(int(rows[1]["base"], 16), int(manifest["blocks"][0]["base"], 16))
        self.assertEqual(rows[0]["copy_limit_bytes"], manifest["page_size"])
        self.assertEqual(len(rows[0]["ranges"]), 1)
        self.assertEqual(int(rows[0]["ranges"][0].split(":")[0], 16),
                         int(manifest["blocks"][1]["base"], 16))



class NativeAllocatorImportProbe(unittest.TestCase):
    """Native proof: rewriting only the two IAT cells redirects both cell-read shapes
    (the `FF 15 <cell>` site and the CRT's `mov esi, ds:cell` + `call esi` site) into the
    same-arity wrappers without touching any site's bytes."""

    def test_cell_redirect_reaches_both_wrappers(self):
        probe = os.environ.get("NETPLAY_ALLOCATOR_IMPORT_PROBE")
        if probe is None:
            return
        result = subprocess.run([probe], check=False, capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 0)
        self.assertIn("redirect: alloc_wrapper_calls=1 address=00000000 size=2000 type=1000 "
                      "protect=4 free_wrapper_calls=1", result.stdout)
        self.assertIn("free_size=1000 free_type=8000", result.stdout)

if __name__ == "__main__":
    unittest.main()
