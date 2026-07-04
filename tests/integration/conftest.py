"""
Shared fixtures for the end-to-end integration suite.

These tests exercise the full stack: Django gateway -> Crow backend ->
PostgreSQL. They assume the stack is already running, either via
`docker compose up` or the native setup described in the root README.

Connection settings are read from environment variables so the same tests
run against either setup:

  GATEWAY_URL   Django gateway base URL   (default: http://localhost:8000)
  CROW_URL      Crow backend base URL     (default: http://localhost:8080)
  PGHOST        Postgres host             (default: localhost)
  PGPORT        Postgres port             (default: 5433, matching
                                            docker-compose.yml's host mapping;
                                            use 5432 for the native setup)
  PGDATABASE    Postgres database         (default: traffic_system_db)
  PGUSER        Postgres user             (default: traffic_user)
  PGPASSWORD    Postgres password         (default: traffic_pass)

All rows created by these tests use a "TST-" plate prefix and a dedicated
"TST Integration Junction" junction, so cleanup can find and remove exactly
what it created without touching real data.
"""
import os
import uuid

import psycopg2
import pytest
import requests

GATEWAY_URL = os.environ.get("GATEWAY_URL", "http://localhost:8000")
CROW_URL = os.environ.get("CROW_URL", "http://localhost:8080")

PG_CONFIG = {
    "host": os.environ.get("PGHOST", "localhost"),
    "port": os.environ.get("PGPORT", "5433"),
    "dbname": os.environ.get("PGDATABASE", "traffic_system_db"),
    "user": os.environ.get("PGUSER", "traffic_user"),
    "password": os.environ.get("PGPASSWORD", "traffic_pass"),
}

TEST_JUNCTION_NAME = "TST Integration Junction"


def _services_reachable():
    try:
        requests.get(f"{GATEWAY_URL}/gateway/congestion_report", timeout=2)
    except requests.exceptions.RequestException:
        return False
    try:
        requests.get(f"{CROW_URL}/api/congestion_report", timeout=2)
    except requests.exceptions.RequestException:
        return False
    return True


def _db_reachable():
    try:
        conn = psycopg2.connect(**PG_CONFIG)
        conn.close()
        return True
    except psycopg2.OperationalError:
        return False


@pytest.fixture(scope="session", autouse=True)
def require_full_stack():
    """
    Skips the whole integration suite (instead of erroring out) when the
    stack isn't running, so `pytest tests/` from a clean checkout doesn't
    fail just because nobody started Docker/Postgres/Crow/Django.
    """
    if not _services_reachable():
        pytest.skip(
            f"Gateway ({GATEWAY_URL}) or Crow backend ({CROW_URL}) is not "
            f"reachable. Start the stack first, e.g. `docker compose up`."
        )
    if not _db_reachable():
        pytest.skip(
            f"Could not connect to Postgres at {PG_CONFIG['host']}:{PG_CONFIG['port']}. "
            f"Set PGHOST/PGPORT/PGUSER/PGPASSWORD/PGDATABASE if your setup differs."
        )


@pytest.fixture(scope="session")
def db_conn():
    conn = psycopg2.connect(**PG_CONFIG)
    conn.autocommit = True
    yield conn
    conn.close()


@pytest.fixture(scope="session")
def test_junction_id(db_conn, require_full_stack):
    """Creates one dedicated junction for the whole test session and tears
    down every row that references it (in FK-safe order) afterwards."""
    with db_conn.cursor() as cur:
        cur.execute(
            "INSERT INTO junctions (name, location) VALUES (%s, %s) "
            "ON CONFLICT (name) DO UPDATE SET location = EXCLUDED.location "
            "RETURNING id",
            (TEST_JUNCTION_NAME, "Integration test fixture"),
        )
        junction_id = cur.fetchone()[0]

    yield junction_id

    with db_conn.cursor() as cur:
        cur.execute("DELETE FROM violations WHERE junction_id = %s", (junction_id,))
        cur.execute("DELETE FROM plate_logs WHERE junction_id = %s", (junction_id,))
        cur.execute("DELETE FROM traffic_counts WHERE junction_id = %s", (junction_id,))
        cur.execute("DELETE FROM violations WHERE plate_number LIKE 'TST-%%'")
        cur.execute("DELETE FROM plate_logs WHERE plate_number LIKE 'TST-%%'")
        cur.execute("DELETE FROM vehicles WHERE plate_number LIKE 'TST-%%'")
        cur.execute("DELETE FROM junctions WHERE id = %s", (junction_id,))


def unique_plate(prefix="TST"):
    """Generates a plate matching ^[A-Z]{1,3}-[A-Z]{1,2}-[0-9]{1,4}$ that's
    unique enough per test run to avoid colliding with a previous run's
    leftovers (cleanup should remove those anyway, but belt-and-braces)."""
    digits = str(uuid.uuid4().int)[:4]
    return f"{prefix}-ZZ-{digits}"
