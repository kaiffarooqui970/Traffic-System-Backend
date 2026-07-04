"""
End-to-end integration tests against the full running stack (Django gateway
+ Crow backend + PostgreSQL). See conftest.py for how the stack is located
and how test data is cleaned up.

Run with the stack already up:
    docker compose up -d
    pip install -r tests/integration/requirements.txt
    pytest tests/integration -v
"""
from conftest import GATEWAY_URL, unique_plate


def _post(path, payload):
    import requests
    return requests.post(f"{GATEWAY_URL}/gateway/{path}", json=payload, timeout=5)


def _get(path, params=None):
    import requests
    return requests.get(f"{GATEWAY_URL}/gateway/{path}", params=params, timeout=5)


def test_register_vehicle_persists_to_db(db_conn):
    plate = unique_plate()
    resp = _post("register_vehicle", {
        "plate_number": plate,
        "owner_name": "Integration Test Owner",
        "email": "integration-test@example.com",
    })
    assert resp.status_code == 200
    assert resp.json()["status"] == "success"

    with db_conn.cursor() as cur:
        cur.execute("SELECT owner_name, email FROM vehicles WHERE plate_number = %s", (plate,))
        row = cur.fetchone()
    assert row is not None
    assert row[0] == "Integration Test Owner"
    assert row[1] == "integration-test@example.com"


def test_log_plate_at_junction_recorded_and_registration_reported(db_conn, test_junction_id):
    plate = unique_plate()
    _post("register_vehicle", {"plate_number": plate, "owner_name": "Plate Logger"})

    resp = _post("log_plate", {
        "plate_number": plate,
        "junction_id": test_junction_id,
        "direction": "entry",
    })
    assert resp.status_code == 200
    body = resp.json()
    assert body["registered"] is True
    assert body["owner_name"] == "Plate Logger"

    with db_conn.cursor() as cur:
        cur.execute(
            "SELECT direction FROM plate_logs WHERE plate_number = %s AND junction_id = %s",
            (plate, test_junction_id),
        )
        row = cur.fetchone()
    assert row is not None
    assert row[0] == "entry"


def test_report_violation_creates_fine_and_notifies_owner(db_conn, test_junction_id):
    plate = unique_plate()
    _post("register_vehicle", {
        "plate_number": plate,
        "owner_name": "Violator",
        "email": "violator@example.com",
    })

    resp = _post("report_violation", {
        "plate_number": plate,
        "violation_type": "speeding",
        "severity": "high",
        "junction_id": test_junction_id,
    })
    assert resp.status_code == 200
    body = resp.json()
    assert body["fine_amount"] == 250
    # Fine amount must match the documented type/severity table - a
    # mismatch here would mean main.cpp's FINE_TABLE drifted from the docs.
    assert body["owner_notified"] is True

    with db_conn.cursor() as cur:
        cur.execute(
            "SELECT violation_type, severity, fine_amount FROM violations WHERE plate_number = %s",
            (plate,),
        )
        row = cur.fetchone()
    assert row is not None
    assert row[0] == "speeding"
    assert row[1] == "high"
    assert float(row[2]) == 250.0

    # Notifications aren't stored in the DB (see docs/ISSUES.md ISS-004) -
    # they're only logged to stdout/notifications.log by the Crow process,
    # so the strongest assertion available from outside that process is the
    # gateway response's owner_notified flag, already checked above.


def test_log_traffic_count_reflected_in_congestion_report(db_conn, test_junction_id):
    resp = _post("log_traffic_count", {
        "junction_id": test_junction_id,
        "vehicle_count": 999,  # comfortably above CONGESTION_THRESHOLD (20)
    })
    assert resp.status_code == 200

    resp = _get("congestion_report", {"junction_id": test_junction_id})
    assert resp.status_code == 200
    body = resp.json()
    junctions = [j for j in body["junctions"] if j["junction_id"] == test_junction_id]
    assert len(junctions) == 1
    assert junctions[0]["recent_count"] >= 999
    assert junctions[0]["is_congested"] is True


def test_emergency_alert_accepted_for_flagged_vehicle(db_conn, test_junction_id):
    plate = unique_plate()
    _post("register_vehicle", {
        "plate_number": plate,
        "owner_name": "Ambulance Driver",
        "is_emergency": True,
    })

    resp = _post("emergency_alert", {"plate_number": plate, "junction_id": test_junction_id})
    assert resp.status_code == 200
    assert resp.json()["status"] == "success"


def test_emergency_alert_rejected_for_non_emergency_vehicle(db_conn, test_junction_id):
    plate = unique_plate()
    _post("register_vehicle", {
        "plate_number": plate,
        "owner_name": "Regular Driver",
        "is_emergency": False,
    })

    resp = _post("emergency_alert", {"plate_number": plate, "junction_id": test_junction_id})
    assert resp.status_code == 403
    assert resp.json()["status"] == "error"
