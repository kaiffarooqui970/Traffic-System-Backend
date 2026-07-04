# UAT-01: Vehicle Registration (Week 12)

Manual sign-off sheet for the vehicle registration feature. No coding
knowledge required - use `curl` or a tool like Postman/Insomnia against
`http://localhost:8000/gateway/register_vehicle` (Django gateway) or
`http://localhost:8080/api/register_vehicle` (Crow backend directly).

## Scenario 1: Register a valid vehicle

**Preconditions:**
- The full stack is running (`docker compose up`, or the native setup from
  the README).
- The plate `L-AB-1234` is not already registered (fresh database, or pick a
  plate you haven't used before).

**Steps:**
1. Given a vehicle owner with plate `L-AB-1234`, name `Kaif`, and email
   `kaif@example.com`.
2. When you `POST` to `/gateway/register_vehicle` with:
   ```json
   {"plate_number": "L-AB-1234", "owner_name": "Kaif", "email": "kaif@example.com"}
   ```
3. Then the response should be `HTTP 200` with
   `{"status": "success", ..., "registered_plate": "L-AB-1234"}`.

**Expected result:** Vehicle is registered; a subsequent lookup (e.g. via
`log_plate`, see UAT-02) reports it as registered.

- [ ] Pass  [ ] Fail   Tested by: ______________  Date: ______________

---

## Scenario 2: Reject an invalid plate format

**Preconditions:** Stack running.

**Steps:**
1. Given a plate that does not match `LETTERS-LETTERS-DIGITS`
   (e.g. `12345`).
2. When you `POST` to `/gateway/register_vehicle` with:
   ```json
   {"plate_number": "12345", "owner_name": "Bad Plate Test"}
   ```
3. Then the response should be `HTTP 400` with
   `{"status": "error", "message": "Registration failed: Illogical license plate."}`.

**Expected result:** Registration is rejected; no row is added to the
`vehicles` table.

- [ ] Pass  [ ] Fail   Tested by: ______________  Date: ______________

---

## Scenario 3: Reject a missing owner name

**Preconditions:** Stack running.

**Steps:**
1. Given a valid plate but no `owner_name` field.
2. When you `POST` to `/gateway/register_vehicle` with:
   ```json
   {"plate_number": "M-CD-5678"}
   ```
3. Then the response should be `HTTP 400` with a message indicating
   `owner_name is required`.

**Expected result:** Registration is rejected.

- [ ] Pass  [ ] Fail   Tested by: ______________  Date: ______________

---

## Scenario 4: Register an emergency vehicle

**Preconditions:** Stack running.

**Steps:**
1. Given a vehicle with `is_emergency: true`.
2. When you `POST` to `/gateway/register_vehicle` with:
   ```json
   {"plate_number": "E-AM-0001", "owner_name": "City Ambulance Service", "is_emergency": true}
   ```
3. Then the response should be `HTTP 200` success.

**Expected result:** Vehicle is registered with the emergency flag set,
enabling it to later pass UAT-05 (emergency alert acceptance).

- [ ] Pass  [ ] Fail   Tested by: ______________  Date: ______________
