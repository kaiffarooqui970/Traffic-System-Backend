import requests
import json
from django.http import JsonResponse
from django.views.decorators.csrf import csrf_exempt

@csrf_exempt
def register_vehicle_gateway(request):
    """
    Gateway endpoint that forwards the vehicle registration payload
    to the C++ Crow backend.
    """
    if request.method == 'POST':
        try:
            client_data = json.loads(request.body)
            crow_url = 'http://localhost:8080/api/register_vehicle'
            crow_response = requests.post(crow_url, json=client_data)
            return JsonResponse(crow_response.json(), status=crow_response.status_code)
            
        except requests.exceptions.ConnectionError:
            return JsonResponse({
                'error': 'Could not connect to the C++ Backend. Is it running on port 8080?'
            }, status=503)
        except Exception as e:
            return JsonResponse({'error': str(e)}, status=500)
            
    return JsonResponse({'error': 'Only POST methods are allowed'}, status=405)