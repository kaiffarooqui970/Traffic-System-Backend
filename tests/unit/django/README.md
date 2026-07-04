# Django gateway unit tests

The gateway's unit tests live at
[`src/django-gateway/api/tests.py`](../../../src/django-gateway/api/tests.py),
next to the app they test, which is Django's standard convention
(`manage.py test` auto-discovers `tests.py`/`tests/` inside each app).

They use Django's built-in `TestCase` + `unittest.mock.patch` to mock
`requests.post`/`requests.get`, so they run without a live Crow backend or
database - only the gateway's forwarding logic is under test (the gateway
has no business logic of its own; that lives in the Crow backend, covered by
[`tests/unit/cpp/`](../cpp/)).

## Run

```bash
cd src/django-gateway
pip install -r requirements.txt
python manage.py test api
```

Expect `Ran 8 tests ... OK`.

## What's covered

- Each gateway route forwards to the correct Crow endpoint (`/api/...`).
- Response body and status code are relayed back unchanged, including error
  statuses (e.g. a 404 from Crow becomes a 404 from the gateway).
- `requests.exceptions.ConnectionError` (Crow backend not running) is caught
  and turned into a `503` with a descriptive message.
- Non-POST requests to POST-only routes are rejected with `405`.
