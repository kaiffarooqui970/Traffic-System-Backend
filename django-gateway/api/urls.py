from django.urls import path
from . import views

urlpatterns = [
    # The client will call: http://localhost:8000/gateway/register_vehicle
    path('register_vehicle', views.register_vehicle_gateway, name='register_vehicle'),
]