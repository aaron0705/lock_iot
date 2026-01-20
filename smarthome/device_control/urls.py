from django.urls import path, include
from . import views
app_name = 'device_control_app'

urlpatterns = [
<<<<<<< HEAD
    path('device_control/', views.device_control, name="device_control")
=======
    path('', views.device_control, name='device_control'),
    path('add', views.add_device, name='add'),
    path('remove', views.remove_device_view, name='remove'),
    path('detail/<int:device_id>/', views.device_detail, name='detail')
>>>>>>> a02b74048eed94e2825a54c58349d894719e27c2
]