# Automated Traffic Management System - Backend API

A backend system for managing city traffic: vehicle registration, junction
plate logging, violation/fine issuance, congestion analysis, and emergency
vehicle prioritisation. Built for LZPMPC005L Programming Clinic (weeks 12-15).

## Architecture

```
Client → Django Gateway (pass-through only) → C++ Crow Backend (all business logic + DB access) → PostgreSQL
```

- **Django Gateway** (`django-gateway/`, port 8000): pure HTTP pass-through.
  Each route forwards the client's JSON to the matching Crow endpoint and
  relays the response/status code back unchanged. No validation or business
  logic lives here.
- **C++ Crow Backend** (`crow-backend/`, port 8080): all validation, business
  logic, and database access (via `libpqxx`).
- **PostgreSQL**: persists vehicles, junctions, plate logs, violations, and
  traffic counts. Schema in `database-scripts/schema.sql`.

See `docs/` for UML diagrams (use case, sequence, class).

## What's implemented

| Week | Feature | Status |
|---|---|---|
| 12 | Vehicle registration (`register_vehicle`) | Done |
| 12 | Junction plate logging (`log_plate`) | Done |
| 13 | Violation reporting + configurable fines (`report_violation`) | Done |
| 13 | Email notification stub (console/file logged, SMTP-ready) | Done |
| 14 | Traffic count logging (`log_traffic_count`) | Done |
| 14 | Congestion report + hour-of-day prediction (`congestion_report`) | Done |
| 15 | Emergency vehicle registration flag (`is_emergency`) | Done |
| 15 | Emergency alert + "clear the way" notifications (`emergency_alert`) | Done |

**Known simplifications (by design, not oversights):**
- Plate recognition is simulated: `log_plate` accepts plate data directly in
  the request body rather than doing real image/OCR recognition.
- Congestion "prediction" is a simple historical average per hour-of-day
  compared against a fixed threshold - not a trained ML model.
- Email notifications are logged to stdout and `crow-backend/notifications.log`
  instead of actually being sent, since no SMTP server is configured for this
  project. `send_email_notification()` in `main.cpp` is the single place a
  real SMTP/API call would be dropped in.

## Technology Stack

- **Gateway:** Python 3, Django
- **Backend:** C++17, [Crow](https://crowcpp.org/), `libpqxx`, CMake
- **Database:** PostgreSQL
- **Containerization:** Docker, docker-compose

## Project Structure

```
.
├── crow-backend/              # C++ Crow API - all business logic + DB access
│   ├── main.cpp                # every /api/* endpoint
│   ├── crow_all.h               # vendored single-header Crow framework
│   ├── CMakeLists.txt
│   └── Dockerfile
├── django-gateway/            # Django API Gateway - pure HTTP pass-through
│   ├── api/
│   │   ├── views.py             # one forwarding view per Crow endpoint
│   │   ├── urls.py
│   │   └── tests.py             # gateway pass-through tests
│   ├── traffic_gateway/settings.py
│   ├── requirements.txt
│   └── Dockerfile
├── database-scripts/
│   └── schema.sql              # full Postgres schema (5 tables)
├── docs/                      # Mermaid UML diagrams (use case, sequence, class)
├── docker-compose.yml
└── README.md
```

## How to Run This Project

You have two options: **Docker Compose** (fastest, no local Postgres/CMake
setup needed) or a **native setup** (build the C++ binary and run Django
directly on your machine). Both talk to the same schema and expose the same
API.

### Option A: Docker Compose (recommended)

**Prerequisites:** Docker Desktop (or Docker Engine + Compose plugin)
installed and running.

**1. From the repo root, build and start everything:**
```bash
docker compose up --build
```
This builds and starts three containers, in dependency order:
- `db` - `postgres:16`, with `database-scripts/schema.sql` automatically
  executed against a fresh volume on first boot (Postgres's
  `docker-entrypoint-initdb.d` mechanism - it only runs once, the first time
  the `pgdata` volume is created)
- `crow-backend` - built from `crow-backend/Dockerfile`, listening on host
  port `8080`, waits for `db`'s healthcheck before starting
- `django-gateway` - built from `django-gateway/Dockerfile`, listening on
  host port `8000`, waits for `crow-backend`

Leave this running in its terminal (or add `-d` to run detached:
`docker compose up --build -d`).

**2. Verify it's up** (in a second terminal):
```bash
docker compose ps
```
All three services should show as `running`/`healthy`.

**3. Smoke-test it:**
```bash
curl -X POST http://localhost:8000/gateway/register_vehicle \
  -d '{"plate_number":"L-AB-1234","owner_name":"Kaif","email":"kaif@example.com"}'
```
Expected: `{"status": "success", "message": "Vehicle registered and saved to database!", "registered_plate": "L-AB-1234"}`

**4. Stopping:**
```bash
docker compose down          # stops and removes containers, keeps the pgdata volume (your data persists)
docker compose down -v       # also deletes the pgdata volume (full reset, re-runs schema.sql next time)
```

**5. Rebuilding after code changes:**
```bash
docker compose up --build    # rebuilds any service whose Dockerfile/context changed
```

**Docker troubleshooting:**
- *"port is already allocated"* - something else on your machine is already
  using `8080`, `8000`, or `5432`. Either stop that process, or edit the
  `ports:` mappings in `docker-compose.yml` (left side is the host port,
  right side is the port inside the container - e.g. change `"8080:8080"`
  to `"18080:8080"` and use `http://localhost:18080` instead).
- *`crow-backend` keeps restarting / can't connect to DB* - run
  `docker compose logs db` and `docker compose logs crow-backend` to see
  why; usually means `db`'s healthcheck hasn't passed yet (wait a few
  seconds) or the `pgdata` volume is in a bad state (try `docker compose
  down -v` for a clean reset).
- *Schema changes aren't showing up* - `schema.sql` only runs on a **fresh**
  volume. If you edited it after the first run, do `docker compose down -v`
  then `docker compose up --build` again.

### Option B: Native setup (macOS)

**1. Install prerequisites via Homebrew:**
```bash
brew install postgresql libpq libpqxx cmake asio python3
```

**2. Start Postgres** (if not already running):
```bash
brew services start postgresql@14   # or whichever version brew installed
pg_isready                          # should print "accepting connections"
```

**3. Create the database and load the schema:**
```bash
createdb traffic_system_db
psql -d traffic_system_db -f database-scripts/schema.sql
```
Re-running this is safe - every `CREATE TABLE`/`CREATE INDEX` uses
`IF NOT EXISTS`.

**4. Build and run the Crow backend:**
```bash
cd crow-backend
cmake -B build
cmake --build build
./build/server
```
You should see:
```
Starting C++ Crow Server on port 8080...
Crow/master server is running at http://0.0.0.0:8080 using 10 threads
```
Leave this running; it listens on `http://localhost:8080`.

Connection details are read from environment variables - `PGHOST`,
`PGPORT`, `PGUSER`, `PGPASSWORD`, `PGDATABASE` - each falling back to a
sensible local default (`dbname=traffic_system_db` via the local Unix
socket) when unset, so no extra configuration is needed for local
development. `PORT` overrides the listen port (default `8080`) if you need
to run on a different one, e.g. `PORT=8090 ./build/server`.

**5. In a new terminal, install and run the Django gateway:**
```bash
cd django-gateway
python3 -m venv venv && source venv/bin/activate   # optional but recommended
pip install -r requirements.txt
python manage.py runserver
```
You should see Django's usual `Starting development server at
http://127.0.0.1:8000/`. It forwards to the Crow backend via
`CROW_BACKEND_URL` (defaults to `http://localhost:8080`; set it if you ran
Crow on a different port, e.g. `CROW_BACKEND_URL=http://localhost:8090
python manage.py runserver`).

**6. Smoke-test it** (in a third terminal):
```bash
curl -X POST http://localhost:8000/gateway/register_vehicle \
  -d '{"plate_number":"L-AB-1234","owner_name":"Kaif","email":"kaif@example.com"}'
```

**Native setup troubleshooting:**
- *`Failed to bind to 0.0.0.0:8080 - Address already in use`* - another
  process is already on that port; find it with `lsof -i :8080` and either
  stop it or run Crow with `PORT=<other-port> ./build/server`.
- *`Failed to save to database. Is PostgreSQL running?`* - check
  `pg_isready`; if Postgres isn't running, `brew services start
  postgresql@14`. If it is running but the database doesn't exist yet, redo
  step 3.
- *`ld: library 'pqxx' not found` at build time* - `libpqxx`/`libpq` aren't
  installed, or CMake's cache is stale from a previous config. Re-run
  `brew install libpq libpqxx` then delete `crow-backend/build/` and re-run
  `cmake -B build && cmake --build build`.
- *`asio.hpp file not found`* - `brew install asio` (Crow is header-only but
  depends on standalone ASIO for networking).

### Environment variables reference

| Variable | Used by | Default | Purpose |
|---|---|---|---|
| `PGHOST` | Crow backend | *(unset → local Unix socket)* | Postgres host |
| `PGPORT` | Crow backend | *(unset → default port)* | Postgres port |
| `PGUSER` | Crow backend | *(unset → current OS user)* | Postgres user |
| `PGPASSWORD` | Crow backend | *(unset → none)* | Postgres password |
| `PGDATABASE` | Crow backend | `traffic_system_db` | Postgres database name |
| `PORT` | Crow backend | `8080` | Port the Crow server listens on |
| `CROW_BACKEND_URL` | Django gateway | `http://localhost:8080` | Base URL the gateway forwards requests to |
| `DJANGO_SECRET_KEY` | Django gateway | insecure dev fallback | Django's `SECRET_KEY` |
| `DJANGO_DEBUG` | Django gateway | `True` | Django debug mode |
| `DJANGO_ALLOWED_HOSTS` | Django gateway | `localhost,127.0.0.1` | Comma-separated allowed hosts |

In `docker-compose.yml`, all of these are already wired up between services
(e.g. `PGHOST=db`, `CROW_BACKEND_URL=http://crow-backend:8080`) - you only
need this table for the native setup or if you're customizing the compose
file.

### Running tests

```bash
cd django-gateway
python manage.py test api
```
This runs the gateway pass-through tests (mocked HTTP calls to Crow - no
running Crow backend or database required). Expect `Ran 8 tests ... OK`.

There is no separate C++ test suite; the Crow backend is verified by
building it (`cmake --build build`) and exercising each endpoint with
`curl` against a real Postgres instance, as shown in the smoke-test steps
above.

## API Documentation

All responses follow `{"status": "success"|"error", ...}`. Errors return
`{"status": "error", "message": "..."}` with `400` (bad input), `404`
(not-found lookup), or `500` (DB error).

Every endpoint below is available at `POST /gateway/<name>` (Django) or
directly at `POST /api/<name>` on the Crow backend (port 8080).

---

### `POST register_vehicle`

Registers a vehicle. Plate must match `^[A-Z]{1,3}-[A-Z]{1,2}-[0-9]{1,4}$`.

Request:
```json
{
    "plate_number": "L-AB-1234",
    "owner_name": "Kaif",
    "email": "kaif@example.com",
    "vehicle_type": "SUV",
    "is_emergency": false
}
```

Success (200):
```json
{"status": "success", "message": "Vehicle registered and saved to database!", "registered_plate": "L-AB-1234"}
```

Error (400):
```json
{"status": "error", "message": "Registration failed: Illogical license plate."}
```

---

### `POST log_plate`

Logs a plate sighting at a junction and reports whether it's registered.

Request:
```json
{"plate_number": "L-AB-1234", "junction_id": 1, "direction": "entry"}
```

Success (200):
```json
{"status": "success", "plate_number": "L-AB-1234", "registered": true, "owner_name": "Kaif"}
```

`direction` must be `"entry"` or `"exit"`. Unregistered plates still get
logged; `registered` will be `false` and `owner_name` omitted.

---

### `POST report_violation`

Looks up the vehicle, computes a fine from a configurable
type/severity table, records the violation, and emails the owner.

Request:
```json
{"plate_number": "L-AB-1234", "violation_type": "speeding", "severity": "high", "junction_id": 1}
```

Success (200):
```json
{
    "status": "success",
    "violation_id": 1,
    "plate_number": "L-AB-1234",
    "violation_type": "speeding",
    "severity": "high",
    "fine_amount": 250,
    "owner_notified": true
}
```

`violation_type` must be one of `speeding`, `illegal_parking`, `red_light`.
`severity` must be one of `low`, `medium`, `high`. Fine amounts are looked up
from a `violation_type -> severity -> amount` table in `main.cpp` (not
hardcoded per request), so each type/severity combination can be tuned
independently. `junction_id` is optional. 404 if the plate isn't registered.

---

### `POST log_traffic_count`

Records vehicle throughput at a junction.

Request:
```json
{"junction_id": 1, "vehicle_count": 25}
```

Success (200):
```json
{"status": "success", "junction_id": 1, "vehicle_count": 25}
```

`vehicle_count` must be a non-negative integer.

---

### `GET congestion_report`

Aggregates recent `plate_logs`/`traffic_counts` per junction, flags
congestion above a configurable threshold, and (for congested junctions)
emails recently-logged drivers suggesting the least-congested alternative
junction. Optional query params: `junction_id` (filter to one junction),
`hour` (0-23, checks historical average for that hour instead of "now").

Success (200):
```json
{
    "status": "success",
    "window_minutes": 15,
    "threshold": 20,
    "hour_checked": 17,
    "junctions": [
        {
            "junction_id": 1,
            "junction_name": "Market Square",
            "recent_count": 27,
            "is_congested": true,
            "historical_avg_for_hour": 25,
            "predicted_congested": true
        }
    ],
    "drivers_notified": 1
}
```

---

### `POST emergency_alert`

Given a plate and junction, verifies the vehicle is a registered emergency
vehicle (`is_emergency = true`), logs a priority event, and emails drivers
recently logged at that junction to clear the way. Non-emergency or unknown
plates are rejected.

Request:
```json
{"plate_number": "E-AM-0001", "junction_id": 1}
```

Success (200):
```json
{"status": "success", "message": "Emergency priority event logged.", "plate_number": "E-AM-0001", "junction_id": 1, "drivers_notified": 2}
```

Error (403) if the vehicle isn't a registered emergency vehicle:
```json
{"status": "error", "message": "Vehicle is not a registered emergency vehicle; alert rejected."}
```

---

Developed by Kaif Ahmed Farooqui, ALI
