# UAT-04: Traffic Flow Analysis & Congestion Detection (Week 14)

Manual sign-off sheet for traffic count logging and the congestion report.
Congestion threshold is 20 vehicles within a 15-minute window (see
`CONGESTION_THRESHOLD` / `CONGESTION_WINDOW_MINUTES` in
`src/crow-backend/traffic_logic.h`).

## Scenario 1: Log a traffic count

**Preconditions:**
- Stack running.
- At least one junction exists.

**Steps:**
1. Given junction ID `1`.
2. When you `POST` to `/gateway/log_traffic_count` with:
   ```json
   {"junction_id": 1, "vehicle_count": 25}
   ```
3. Then the response should be `HTTP 200` echoing back the junction and
   count.

**Expected result:** A row is added to `traffic_counts`.

- [ ] Pass  [ ] Fail   Tested by: ______________  Date: ______________

---

## Scenario 2: Congestion report flags a congested junction

**Preconditions:** Stack running; junction ID `1` has had at least one
`log_traffic_count` call within the last 15 minutes totalling more than 20
vehicles (e.g. the 25 from Scenario 1, or several smaller calls that add
up).

**Steps:**
1. When you `GET /gateway/congestion_report?junction_id=1`.
2. Then the response should be `HTTP 200` with that junction's
   `"is_congested": true` and `"recent_count"` greater than 20.

**Expected result:** The junction is correctly flagged as congested.

- [ ] Pass  [ ] Fail   Tested by: ______________  Date: ______________

---

## Scenario 3: Congestion report does not flag a quiet junction

**Preconditions:** Stack running; a junction with no recent traffic counts
(or a total under 20 in the last 15 minutes).

**Steps:**
1. When you `GET /gateway/congestion_report?junction_id=<quiet junction id>`.
2. Then `"is_congested"` should be `false`.

**Expected result:** Junctions under the threshold are not flagged.

- [ ] Pass  [ ] Fail   Tested by: ______________  Date: ______________

---

## Scenario 4: Congested drivers receive a re-routing notification

**Preconditions:** Stack running; junction `1` is congested (Scenario 2);
at least one vehicle with an email address was logged (via `log_plate`) at
junction `1` within the last 15 minutes; at least one other junction
exists with a lower recent count to serve as the suggested alternative.

**Steps:**
1. Given the setup above, When you `GET /gateway/congestion_report`.
2. Then `"drivers_notified"` in the response should be greater than 0.
3. Check the Crow backend's console output / `notifications.log` for a
   "Traffic Congestion Alert" entry addressed to that driver, suggesting
   the alternative junction.

**Expected result:** At least one notification is generated for a driver
recently logged at the congested junction.

- [ ] Pass  [ ] Fail   Tested by: ______________  Date: ______________

---

## Scenario 5: Reject an out-of-range hour parameter

**Preconditions:** Stack running.

**Steps:**
1. When you `GET /gateway/congestion_report?hour=25`.
2. Then the response should be `HTTP 400` stating `hour` must be between 0
   and 23.

**Expected result:** Request rejected.

- [ ] Pass  [ ] Fail   Tested by: ______________  Date: ______________
