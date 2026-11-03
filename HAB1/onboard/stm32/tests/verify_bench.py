"""Validate exported synthetic CSV and sync timing from C bench captures."""
import csv
import io
import re
import sys
from pathlib import Path

previous = {}
for filename in sys.argv[1:]:
    text = Path(filename).read_text(encoding="utf-8-sig")
    assert "PASS: closed/reopened" in text, filename
    payload = text.split("CSV_BEGIN\n", 1)[1].split("CSV_END", 1)[0]
    rows = list(csv.DictReader(io.StringIO(payload)))
    assert len(rows) == 12
    by_seq = {}
    for i, row in enumerate(rows):
        assert len(row) == 17 and None not in row
        assert int(row["sequence_number"]) == i
        assert row["log_format_version"] == row["packet_version"] == "1"
        assert float(row["temperature_c"]) == 20 + i * .25
        assert float(row["pressure_pa"]) == 101325 - i
        assert int(row["valid_fields"]) == 96
        assert row["latitude_deg"] == row["gps_fix_quality"] == ""
        by_seq[i] = int(row["mission_time_ms"])
    synced = -1
    ages = []
    for seq, _, done in re.findall(r"SYNC seq=(\d+) sample_ms=(\d+) done_ms=(\d+)", text):
        seq, done = int(seq), int(done)
        for i in range(synced + 1, seq + 1):
            ages.append(done - by_seq[i])
        synced = seq
    assert ages and max(ages) <= 5000
    for name, size, digest, count in re.findall(
        r"AUDIT (LOG\d+\.CSV) bytes=(\d+) hash=(\w+) rows=(\d+)", text
    ):
        if name in previous:
            assert previous[name] == (size, digest), f"earlier session changed: {name}"
        if int(count):
            previous[name] = size, digest
    print(f"PASS: {filename}: 12 exact records; maximum observed synced age {max(ages)} ms")
