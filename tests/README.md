# Testing and Validation

Three layers of testing, from fastest/most isolated to slowest/most
end-to-end:

```
tests/
├── unit/
│   ├── cpp/       # Crow backend pure-logic tests (doctest, no DB/HTTP)
│   └── django/    # pointer to src/django-gateway/api/tests.py (mocked HTTP)
├── integration/   # full-stack tests against a running Django+Crow+Postgres
└── uat/           # Given/When/Then manual sign-off sheets, one per feature
```

See [`docs/ISSUES.md`](../docs/ISSUES.md) for the bug/issue tracking log
kept alongside this test work.

## 1. Unit tests

### C++ (Crow backend) — `tests/unit/cpp/`

Tests pure, DB-free logic extracted into
[`src/crow-backend/traffic_logic.h`](../src/crow-backend/traffic_logic.h):
plate/email regex validation, violation-type/severity validation, fine
amount lookup, congestion-threshold logic, and emergency-vehicle-flag
checks. Uses [doctest](https://github.com/doctest/doctest) (single header,
vendored at `tests/unit/cpp/third_party/doctest.h`), wired into
`src/crow-backend/CMakeLists.txt` as a separate `traffic_logic_tests`
target so it builds and runs without Postgres, ASIO, or libpqxx.

```bash
cd src/crow-backend
cmake -B build
cmake --build build --target traffic_logic_tests
./build/traffic_logic_tests
# or: cd build && ctest --output-on-failure
```

**Status: 11 test cases / 49 assertions, all passing** (verified
2026-07-03).

### Django gateway — `src/django-gateway/api/tests.py`

Django `TestCase`s that mock `requests.post`/`requests.get`, so they run
without a live Crow backend. See
[`tests/unit/django/README.md`](unit/django/README.md) for what's covered.

```bash
cd src/django-gateway
pip install -r requirements.txt
python manage.py test api
```

**Status: 8 tests, all passing** (verified 2026-07-03).

## 2. Integration tests — `tests/integration/`

End-to-end tests using `pytest` + `requests` against the real Django
gateway, which forwards to a real Crow backend, which writes to a real
Postgres database. Requires the full stack running.

```bash
docker compose up -d
pip install -r tests/integration/requirements.txt
pytest tests/integration -v
```

If the stack isn't reachable, the suite **skips** (doesn't fail) via the
`require_full_stack` fixture in `conftest.py` — see that file for how to
point the tests at a non-default host/port setup (e.g. the native
setup, which uses Postgres port 5432 instead of docker-compose's 5433).

Covers the full round trip for every Week 12-15 feature:
- Register a vehicle → row appears in `vehicles`.
- Log a plate at a junction → row appears in `plate_logs`, registration
  status reported correctly.
- Report a violation → row appears in `violations` with the correct fine
  amount; `owner_notified` reflects an email being sent (the notification
  itself is file/console-logged, not DB-stored — see `docs/ISSUES.md`
  ISS-004).
- Log traffic counts → congestion report reflects them and flags the
  junction as congested once over threshold.
- Emergency alert accepted for a flagged vehicle, rejected (403) for a
  non-emergency one.

All test data uses a `TST-` plate prefix and a dedicated "TST Integration
Junction" junction; `test_junction_id`'s teardown deletes every row it
created (in FK-safe order) at the end of the session, so re-running the
suite doesn't accumulate junk data.

**Status: 6 tests, all passing against a live `docker compose` stack**
(verified 2026-07-03).

## 3. User Acceptance Tests — `tests/uat/`

One markdown file per Week 12-15 feature, written as Given/When/Then
scenarios with preconditions, steps, expected results, and a `- [ ] Pass /
- [ ] Fail` checkbox — usable as a manual sign-off sheet by a non-technical
grader with just `curl`/Postman and the running stack.

| File | Feature |
|---|---|
| `UAT-01-vehicle-registration.md` | Week 12: vehicle registration |
| `UAT-02-plate-logging.md` | Week 12: junction plate logging/recognition |
| `UAT-03-violation-fine-issuance.md` | Week 13: violation reporting + fines |
| `UAT-04-congestion-detection.md` | Week 14: traffic flow + congestion |
| `UAT-05-emergency-vehicle-prioritisation.md` | Week 15: emergency alerts |

These are **not automated** — they're a manual checklist. The scenarios
they describe are exercised automatically by `tests/integration/`, but
sign-off requires a human to actually run through them against the running
stack and check the boxes.

**Status: not yet signed off** — the checklists exist and their happy-path
scenarios are known to work (proven by the automated integration suite
above), but nobody has manually run through and ticked every box yet. Do
not report these as "passed" until someone actually has.

## Honesty note

Every "passing" status above was verified by actually running the suite
against this repository on 2026-07-03, not assumed. If you change
`src/crow-backend/main.cpp`, `traffic_logic.h`, or the Django gateway,
re-run the relevant suite before updating this file — a stale "passing"
claim here is worse than no claim at all.
