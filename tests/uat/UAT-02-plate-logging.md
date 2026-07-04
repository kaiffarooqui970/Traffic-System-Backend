# UAT-02: Junction Plate Logging / Recognition (Week 12)

Manual sign-off sheet for junction plate-sighting logging. Plate
recognition is simulated - plate data is submitted directly rather than
via real image/OCR recognition (see README "Known simplifications").

## Scenario 1: Log a registered plate at a junction

**Preconditions:**
- Stack running.
- Plate `L-AB-1234` is already registered (see UAT-01, Scenario 1).
- At least one junction exists in the `junctions` table (`schema.sql` seeds
  three by default, so junction ID `1` = "Market Square" out of the box).

**Steps:**
1. Given the registered plate `L-AB-1234` and an existing junction ID
   (e.g. `1`).
2. When you `POST` to `/gateway/log_plate` with:
   ```json
   {"plate_number": "L-AB-1234", "junction_id": 1, "direction": "entry"}
   ```
3. Then the response should be `HTTP 200` with
   `{"status": "success", "registered": true, "owner_name": "Kaif", ...}`.

**Expected result:** A row is added to `plate_logs`; the response confirms
the plate is registered and shows the owner's name.

- [ ] Pass  [ ] Fail   Tested by: ______________  Date: ______________

---

## Scenario 2: Log an unregistered plate

**Preconditions:** Stack running; junction exists; plate `Z-ZZ-9999` is not
registered.

**Steps:**
1. Given an unregistered but validly-formatted plate `Z-ZZ-9999`.
2. When you `POST` to `/gateway/log_plate` with:
   ```json
   {"plate_number": "Z-ZZ-9999", "junction_id": 1, "direction": "entry"}
   ```
3. Then the response should be `HTTP 200` with `"registered": false` and no
   `owner_name` field.

**Expected result:** The sighting is still logged (cameras see every
plate, registered or not), but flagged as unregistered.

- [ ] Pass  [ ] Fail   Tested by: ______________  Date: ______________

---

## Scenario 3: Reject an invalid direction

**Preconditions:** Stack running; junction exists.

**Steps:**
1. Given a valid plate but `direction: "sideways"`.
2. When you `POST` to `/gateway/log_plate` with:
   ```json
   {"plate_number": "L-AB-1234", "junction_id": 1, "direction": "sideways"}
   ```
3. Then the response should be `HTTP 400` with a message that `direction`
   must be `entry` or `exit`.

**Expected result:** Request rejected; no row added.

- [ ] Pass  [ ] Fail   Tested by: ______________  Date: ______________

---

## Scenario 4: Reject a non-existent junction

**Preconditions:** Stack running.

**Steps:**
1. Given a valid plate and a `junction_id` that does not exist
   (e.g. `999999`).
2. When you `POST` to `/gateway/log_plate` with:
   ```json
   {"plate_number": "L-AB-1234", "junction_id": 999999, "direction": "entry"}
   ```
3. Then the response should be `HTTP 404` with a message that `junction_id`
   does not reference a known junction.

**Expected result:** Request rejected; no row added.

- [ ] Pass  [ ] Fail   Tested by: ______________  Date: ______________
