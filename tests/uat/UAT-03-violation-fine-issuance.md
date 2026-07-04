# UAT-03: Violation Reporting & Fine Issuance (Week 13)

Manual sign-off sheet for violation reporting, fine calculation, and owner
notification.

**Fine table (for reference while checking results):**

| Violation type    | Low  | Medium | High |
|--------------------|------|--------|------|
| speeding           | $50  | $120   | $250 |
| illegal_parking    | $20  | $50    | $100 |
| red_light          | $80  | $150   | $300 |

## Scenario 1: Report a violation for a registered vehicle

**Preconditions:**
- Stack running.
- Plate `L-AB-1234` is registered with an email address (see UAT-01).

**Steps:**
1. Given the registered plate `L-AB-1234`.
2. When you `POST` to `/gateway/report_violation` with:
   ```json
   {"plate_number": "L-AB-1234", "violation_type": "speeding", "severity": "high"}
   ```
3. Then the response should be `HTTP 200` with `"fine_amount": 250` and
   `"owner_notified": true`.

**Expected result:** A row is added to `violations` with `fine_amount =
250.00`, matching the fine table above. A notification is written to
stdout / `notifications.log` (check the Crow backend's console output or
`src/crow-backend/notifications.log` for native runs; `docker compose logs
crow-backend` for Docker runs).

- [ ] Pass  [ ] Fail   Tested by: ______________  Date: ______________

---

## Scenario 2: Fine amount matches severity for every type

**Preconditions:** Stack running; a registered plate available.

**Steps:**
1. Repeat Scenario 1's request for each combination of
   `violation_type` in `{speeding, illegal_parking, red_light}` and
   `severity` in `{low, medium, high}` (9 requests total).
2. For each, note the returned `fine_amount`.

**Expected result:** Every returned amount matches the fine table above
exactly.

- [ ] Pass  [ ] Fail   Tested by: ______________  Date: ______________

---

## Scenario 3: Reject an unregistered plate

**Preconditions:** Stack running; plate `Q-QQ-0000` is not registered.

**Steps:**
1. Given an unregistered plate `Q-QQ-0000`.
2. When you `POST` to `/gateway/report_violation` with:
   ```json
   {"plate_number": "Q-QQ-0000", "violation_type": "speeding", "severity": "low"}
   ```
3. Then the response should be `HTTP 404` with a message that no
   registered vehicle was found.

**Expected result:** No violation is recorded.

- [ ] Pass  [ ] Fail   Tested by: ______________  Date: ______________

---

## Scenario 4: Reject an invalid violation type or severity

**Preconditions:** Stack running; a registered plate available.

**Steps:**
1. Given a registered plate.
2. When you `POST` with `"violation_type": "jaywalking"` (not in the
   table).
3. Then the response should be `HTTP 400` listing the valid types.
4. Repeat with a valid type but `"severity": "extreme"` (not in the table)
   and confirm `HTTP 400` listing the valid severities.

**Expected result:** Both requests are rejected; no violation recorded.

- [ ] Pass  [ ] Fail   Tested by: ______________  Date: ______________
