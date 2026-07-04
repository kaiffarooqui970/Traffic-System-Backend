import os
import json
import requests
from django.http import JsonResponse
from django.views.decorators.csrf import csrf_exempt

# Base URL of the C++ Crow backend. Configurable via env var so this works
# both for local dev (default) and inside docker-compose.
CROW_BASE_URL = os.environ.get('CROW_BACKEND_URL', 'http://localhost:8080')


def _forward_post(request, crow_path):
    """
    Pure pass-through: forwards the client's JSON body to the given Crow
    endpoint and relays the response back unchanged. No validation or
    business logic happens here - that all lives in the Crow backend.
    """
    if request.method != 'POST':
        return JsonResponse({'error': 'Only POST methods are allowed'}, status=405)
    try:
        client_data = json.loads(request.body)
        crow_response = requests.post(f'{CROW_BASE_URL}{crow_path}', json=client_data)
        return JsonResponse(crow_response.json(), status=crow_response.status_code)
    except requests.exceptions.ConnectionError:
        return JsonResponse({
            'error': f'Could not connect to the C++ Backend. Is it running on port 8080? ({crow_path})'
        }, status=503)
    except Exception as e:
        return JsonResponse({'error': str(e)}, status=500)


def _forward_get(request, crow_path):
    """Pure pass-through for GET endpoints, forwarding query params as-is."""
    if request.method != 'GET':
        return JsonResponse({'error': 'Only GET methods are allowed'}, status=405)
    try:
        crow_response = requests.get(f'{CROW_BASE_URL}{crow_path}', params=request.GET)
        return JsonResponse(crow_response.json(), status=crow_response.status_code)
    except requests.exceptions.ConnectionError:
        return JsonResponse({
            'error': f'Could not connect to the C++ Backend. Is it running on port 8080? ({crow_path})'
        }, status=503)
    except Exception as e:
        return JsonResponse({'error': str(e)}, status=500)


@csrf_exempt
def register_vehicle_gateway(request):
    """Forwards vehicle registration payloads to the C++ Crow backend."""
    return _forward_post(request, '/api/register_vehicle')


@csrf_exempt
def log_plate_gateway(request):
    """Forwards junction plate-recognition events to the C++ Crow backend."""
    return _forward_post(request, '/api/log_plate')


@csrf_exempt
def report_violation_gateway(request):
    """Forwards traffic violation reports to the C++ Crow backend."""
    return _forward_post(request, '/api/report_violation')


@csrf_exempt
def log_traffic_count_gateway(request):
    """Forwards junction traffic-count logs to the C++ Crow backend."""
    return _forward_post(request, '/api/log_traffic_count')


@csrf_exempt
def congestion_report_gateway(request):
    """Forwards congestion report requests to the C++ Crow backend."""
    return _forward_get(request, '/api/congestion_report')


@csrf_exempt
def emergency_alert_gateway(request):
    """Forwards emergency vehicle alerts to the C++ Crow backend."""
    return _forward_post(request, '/api/emergency_alert')
