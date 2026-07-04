import json
from unittest.mock import patch, MagicMock

import requests
from django.test import TestCase, Client


class GatewayPassThroughTests(TestCase):
    """
    These tests only check that each gateway route forwards requests to the
    correct Crow endpoint and relays the response/status code back unchanged.
    They mock requests.post/get, since the gateway itself has no business
    logic to test - that lives in the Crow backend.
    """

    def setUp(self):
        self.client = Client()

    def _mock_response(self, status_code=200, json_body=None):
        mock_resp = MagicMock()
        mock_resp.status_code = status_code
        mock_resp.json.return_value = json_body or {"status": "success"}
        return mock_resp

    @patch('api.views.requests.post')
    def test_register_vehicle_forwards_to_crow(self, mock_post):
        mock_post.return_value = self._mock_response(200, {"status": "success", "registered_plate": "L-AB-1234"})
        payload = {"plate_number": "L-AB-1234", "owner_name": "Kaif", "email": "kaif@example.com"}

        response = self.client.post('/gateway/register_vehicle', data=json.dumps(payload), content_type='application/json')

        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.json()['registered_plate'], 'L-AB-1234')
        called_url = mock_post.call_args[0][0]
        self.assertTrue(called_url.endswith('/api/register_vehicle'))

    @patch('api.views.requests.post')
    def test_log_plate_forwards_to_crow(self, mock_post):
        mock_post.return_value = self._mock_response(200, {"status": "success", "registered": True})
        payload = {"plate_number": "L-AB-1234", "junction_id": 1, "direction": "entry"}

        response = self.client.post('/gateway/log_plate', data=json.dumps(payload), content_type='application/json')

        self.assertEqual(response.status_code, 200)
        self.assertTrue(mock_post.call_args[0][0].endswith('/api/log_plate'))

    @patch('api.views.requests.post')
    def test_report_violation_relays_error_status(self, mock_post):
        mock_post.return_value = self._mock_response(404, {"status": "error", "message": "No registered vehicle found"})
        payload = {"plate_number": "Z-ZZ-0000", "violation_type": "speeding", "severity": "low"}

        response = self.client.post('/gateway/report_violation', data=json.dumps(payload), content_type='application/json')

        self.assertEqual(response.status_code, 404)
        self.assertEqual(response.json()['status'], 'error')

    @patch('api.views.requests.post')
    def test_log_traffic_count_forwards_to_crow(self, mock_post):
        mock_post.return_value = self._mock_response(200, {"status": "success"})
        payload = {"junction_id": 1, "vehicle_count": 10}

        response = self.client.post('/gateway/log_traffic_count', data=json.dumps(payload), content_type='application/json')

        self.assertEqual(response.status_code, 200)
        self.assertTrue(mock_post.call_args[0][0].endswith('/api/log_traffic_count'))

    @patch('api.views.requests.get')
    def test_congestion_report_forwards_query_params(self, mock_get):
        mock_get.return_value = self._mock_response(200, {"status": "success", "junctions": []})

        response = self.client.get('/gateway/congestion_report', {'junction_id': '1'})

        self.assertEqual(response.status_code, 200)
        self.assertTrue(mock_get.call_args[0][0].endswith('/api/congestion_report'))

    @patch('api.views.requests.post')
    def test_emergency_alert_forwards_to_crow(self, mock_post):
        mock_post.return_value = self._mock_response(200, {"status": "success", "drivers_notified": 2})
        payload = {"plate_number": "E-AM-0001", "junction_id": 1}

        response = self.client.post('/gateway/emergency_alert', data=json.dumps(payload), content_type='application/json')

        self.assertEqual(response.status_code, 200)
        self.assertTrue(mock_post.call_args[0][0].endswith('/api/emergency_alert'))

    @patch('api.views.requests.post', side_effect=requests.exceptions.ConnectionError)
    def test_connection_error_returns_503(self, mock_post):
        payload = {"plate_number": "L-AB-1234", "owner_name": "Kaif"}

        response = self.client.post('/gateway/register_vehicle', data=json.dumps(payload), content_type='application/json')

        self.assertEqual(response.status_code, 503)

    def test_get_method_rejected_on_post_only_route(self):
        response = self.client.get('/gateway/register_vehicle')
        self.assertEqual(response.status_code, 405)
