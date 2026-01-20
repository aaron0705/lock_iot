from django.urls import path, include
from . import views
app_name = 'device_control_app'

urlpatterns = [
    path('', views.device_control, name='device_control'),
    path('add', views.add_device, name='add'),
    path('remove', views.remove_device_view, name='remove'),
    path('detail/<int:device_id>/', views.device_detail, name='detail')
]