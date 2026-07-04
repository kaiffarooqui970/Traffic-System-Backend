# Bug and Issue Tracking Log

This is the **single source of truth** for known bugs/issues on this
project. **GitHub Issues is not in use** — this repository is not currently
hosted with issue tracking enabled, so there are no issue numbers to
cross-reference. If GitHub Issues is enabled in the future, new entries
should include the issue number in the "Found In" or "Resolution/Notes"
column and this file should link to it rather than duplicate its content.

## Process — how to log a new bug

1. **Who logs it:** whoever finds it — either author while building/testing
   a feature, or the grader/lecturer during review (see `tests/uat/` for the
   manual sign-off sheets that often surface these).
2. **Required fields:** every row below must be filled in — a bug without a
   severity or a reproduction context ("Found In") isn't actionable. Use the
   next sequential `ISS-0xx` ID.
3. **Severity:**
   - **Critical** — breaks the build or a core endpoint entirely.
   - **Major** — a feature works incorrectly or a documented guarantee
     doesn't hold, but the system still runs.
   - **Minor** — edge case, cosmetic, or quality-of-life issue.
4. **Status changes:** update the `Status` column in place (don't delete old
   rows) and add a note in `Resolution/Notes` explaining what changed and
   why, e.g. "Fixed in commit adding traffic_logic.h — see ISS-001." Status
   values: `Open`, `In Progress`, `Fixed`, `Won't Fix`.
5. Bugs found while writing `tests/unit/`, `tests/integration/`, or
   `tests/uat/` should be logged here even if fixed immediately — the point
   is to keep a record of what testing actually surfaced, not just what's
   currently broken.

## Log

| ID | Title | Severity | Status | Found In | Description | Resolution/Notes |
|---|---|---|---|---|---|---|
| ISS-001 | Docker build broke after adding a CMake unit-test target | Critical | Fixed | `src/crow-backend/CMakeLists.txt` (Deliverable 2, C++ unit tests) | Adding `add_executable(traffic_logic_tests tests/unit/cpp/test_traffic_logic.cpp)` referenced a path outside the Crow Dockerfile's build context (`./src/crow-backend` only, per `docker-compose.yml`), so `docker compose up --build` failed with "Cannot find source file" before the production image could even be built. | Guarded the test target with `if(EXISTS ${UNIT_TEST_DIR}/test_traffic_logic.cpp)` so the production build silently skips it when `tests/` isn't present, and changed the Dockerfile to build the `server` target explicitly (`cmake --build build --target server`) rather than "all". Verified with a full `docker compose up --build` afterward — all three containers start and a real request round-trips through gateway → Crow → Postgres. |
| ISS-002 | Fine/plate/severity validation logic duplicated between `main.cpp` and would-be test code | Major | Fixed | `src/crow-backend/main.cpp` (Deliverable 2, C++ unit tests) | The plate/email regexes, `FINE_TABLE`, and validation helpers lived only inline in `main.cpp`. Unit-testing them without duplicating the logic (which would silently drift from the real behavior over time) required a refactor. | Extracted into `src/crow-backend/traffic_logic.h` (pure, DB-free), included by both `main.cpp` and `tests/unit/cpp/test_traffic_logic.cpp`, so the tests exercise the exact code path the server uses. |
| ISS-003 | Plate numbers are not normalized before validation | Minor | Open | `is_valid_plate()` / `register_vehicle`, `log_plate`, etc. in `src/crow-backend/main.cpp` | The plate regex `^[A-Z]{1,3}-[A-Z]{1,2}-[0-9]{1,4}$` requires uppercase with no surrounding whitespace. A client sending `"l-ab-1234"` or `" L-AB-1234 "` gets a generic 400 rather than being auto-corrected, which could confuse a non-technical operator entering plates by hand. | Not fixed — flagging as a known edge case. A fix would call `.toupper()`/trim in `is_valid_plate` (or just before it) in `traffic_logic.h`, which is now unit-testable in isolation thanks to ISS-002's refactor. |
| ISS-004 | Notifications aren't persisted anywhere queryable | Major | Open | `send_email_notification()` in `src/crow-backend/main.cpp`; surfaced while writing `tests/integration/test_integration.py` | The spec/course brief implies verifying "a notification log entry is created" for violations, congestion alerts, and emergency alerts. In this codebase, notifications are only written to stdout and `src/crow-backend/notifications.log` (a plain file next to the running process) — there's no `notifications` table and no API to query past notifications. Integration tests can only assert the `owner_notified` / `drivers_notified` flags in the API response, not an actual stored log entry, and UAT scenarios that want to verify a notification require someone to read the Crow process's console/log file by hand. | Not fixed — this is a known simplification (also called out in the main README as "by design, not oversight" for the lack of real SMTP). If persisted notification history becomes a requirement, add a `notifications` table (recipient, subject, body, related entity, timestamp) and insert into it from `send_email_notification()` alongside the existing file/console log. |
| ISS-005 | Duplicate vehicle registration returns a generic 500 instead of 409 Conflict | Minor | Open | `register_vehicle` endpoint in `src/crow-backend/main.cpp`; found manually while re-testing UAT-01 | `vehicles.plate_number` is the primary key. Registering the same plate twice throws a `pqxx` unique-violation exception, which is caught by the generic `catch (const std::exception&)` block and returned as `500 Failed to save to database. Is PostgreSQL running?` — a confusing message for what is actually a client error (plate already registered), not a server/DB availability problem. | Not fixed — would need to either pre-check for an existing plate before inserting, or catch `pqxx::unique_violation` specifically and return `409` with a clearer message. |
| ISS-006 | UAT checklists exist but have not been manually signed off | Minor | Open | `tests/uat/` (Deliverable 2) | The five UAT markdown files describe accurate, verified-working scenarios (their happy paths are proven by the automated `tests/integration/` suite), but nobody has actually run through them by hand and ticked the pass/fail checkboxes — that's a manual step for a grader/reviewer, not something this restructuring work could do on its own. | Not applicable to fix in code — logged so `tests/README.md` doesn't overclaim "UAT passing" when what's true is "UAT scenarios exist and their underlying behavior is proven, but sign-off is pending." |
| ISS-007 | docker-compose default host ports can collide with unrelated local services | Minor | Won't Fix | `docker-compose.yml`; hit firsthand while verifying Deliverable 1/2 against a real `docker compose up` | Ports 8080/8000/5433 are common defaults; on the machine used to verify this work, an unrelated container (`pfims_gateway`) already had port 8080 bound, causing `docker compose up` to fail with "port is already allocated" until the ports were remapped. | Won't Fix as a code change — already documented in the README's Docker troubleshooting section (change the left-hand side of the `ports:` mapping in `docker-compose.yml` if this happens). No code change needed; noting it here since it's a real failure mode hit during this work, not a hypothetical. |
