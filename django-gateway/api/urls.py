from django.urls import path
from . import views

urlpatterns = [
    # The client will call: http://localhost:8000/gateway/<name>
    path('register_vehicle', views.register_vehicle_gateway, name='register_vehicle'),
    path('log_plate', views.log_plate_gateway, name='log_plate'),
    path('report_violation', views.report_violation_gateway, name='report_violation'),
    path('log_traffic_count', views.log_traffic_count_gateway, name='log_traffic_count'),
    path('congestion_report', views.congestion_report_gateway, name='congestion_report'),
    path('emergency_alert', views.emergency_alert_gateway, name='emergency_alert'),
]
