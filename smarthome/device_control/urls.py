from django.urls import path, include
from . import views
app_name = 'device_control_app'

urlpatterns = [
    path('device_control/', views.device_control, name="device_control")
]