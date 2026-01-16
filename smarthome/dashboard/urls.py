from django.urls import path, include
from . import views

app_name = 'dashboard_app'

urlpatterns = [
    path('dashboard/', views.dashboard, name='dashboard')
]