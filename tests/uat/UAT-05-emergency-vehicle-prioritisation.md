# UAT-05: Emergency Vehicle Prioritisation (Week 15)

Manual sign-off sheet for emergency vehicle alerts and "clear the way"
notifications.

## Scenario 1: Emergency alert accepted for a flagged vehicle

**Preconditions:**
- Stack running.
- Plate `E-AM-0001` is registered with `is_emergency: true` (see UAT-01,
  Scenario 4).
- A junction exists (e.g. ID `1`).

**Steps:**
1. Given the flagged emergency vehicle `E-AM-0001`.
2. When you `POST` to `/gateway/emergency_alert` with:
   ```json
   {"plate_number": "E-AM-0001", "junction_id": 1}
   ```
3. Then the response should be `HTTP 200` with
   `{"status": "success", "message": "Emergency priority event logged.", ...}`.

**Expected result:** A priority "entry" event is logged in `plate_logs` for
that plate/junction.

- [ ] Pass  [ ] Fail   Tested by: ______________  Date: ______________

---

## Scenario 2: Emergency alert rejected for a non-emergency vehicle

**Preconditions:** Stack running; plate `L-AB-1234` is registered with
`is_emergency` unset/`false` (see UAT-01, Scenario 1).

**Steps:**
1. Given the non-emergency vehicle `L-AB-1234`.
2. When you `POST` to `/gateway/emergency_alert` with:
   ```json
   {"plate_number": "L-AB-1234", "junction_id": 1}
   ```
3. Then the response should be `HTTP 403` with
   `{"status": "error", "message": "Vehicle is not a registered emergency vehicle; alert rejected."}`.

**Expected result:** The alert is rejected; no priority event is logged.

- [ ] Pass  [ ] Fail   Tested by: ______________  Date: ______________

---

## Scenario 3: Emergency alert rejected for an unregistered plate

**Preconditions:** Stack running; plate `U-NR-0000` is not registered.

**Steps:**
1. When you `POST` to `/gateway/emergency_alert` with:
   ```json
   {"plate_number": "U-NR-0000", "junction_id": 1}
   ```
2. Then the response should be `HTTP 404` stating no registered vehicle was
   found.

**Expected result:** The alert is rejected.

- [ ] Pass  [ ] Fail   Tested by: ______________  Date: ______________

---

## Scenario 4: Nearby drivers are notified to clear the way

**Preconditions:** Stack running; a registered emergency vehicle; at least
one other vehicle with an email address was logged (via `log_plate`) at the
same junction within the last 10 minutes.

**Steps:**
1. Given the setup above, When you `POST /gateway/emergency_alert` for the
   emergency vehicle at that junction.
2. Then `"drivers_notified"` in the response should be greater than 0.
3. Check the Crow backend's console output / `notifications.log` for an
   "Emergency Vehicle Alert - Clear the Way" entry addressed to the other
   driver.

**Expected result:** Recently-logged drivers at that junction (excluding
the emergency vehicle itself) are notified.

- [ ] Pass  [ ] Fail   Tested by: ______________  Date: ______________
