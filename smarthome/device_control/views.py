from django.shortcuts import render
from django.contrib.auth.decorators import login_required
from datetime import datetime
from .models import SmartDevice
from django.db.models import Q

# Create your views here.
@login_required
def device_control(request):
    return render(request, 'device_control.html')

def add_device(request):
    if request.method == "POST":
        new_device = SmartDevice.objects.create(
            name = request.POST.get("name")
            current_status = 
            device_type = request.POST.get("type")
            location = request.POST.get("location")
            first_used =  datetime.now()
            last_used = first_used
            last_maintenance = first_used
            description = request.POST.get("description")
        )
        new_device.save()
    return render(request, 'add.html', {'alert_msg': alert_msg})

def remove_device_view(request):
    alert_msg = None  # Biến để chứa nội dung thông báo sẽ gửi ra HTML

    if request.method == "POST":
        search_key = request.POST.get('search_key', '').strip()

        if not search_key:
            alert_msg = "Vui lòng nhập thông tin để tìm kiếm!"
        else:
            # 1. Tạo bộ lọc (như cũ)
            criteria = Q(name__icontains=search_key) | Q(location__icontains=search_key)
            if search_key.isdigit():
                criteria |= Q(id=search_key)
            
            # 2. Tìm kiếm
            found_devices = SmartDevice.objects.filter(criteria)
            count = found_devices.count()

            # 3. Xử lý kết quả
            if count == 0:
                alert_msg = f"Không tìm thấy thiết bị nào khớp với: '{search_key}'"

            elif count == 1:
                device = found_devices.first()
                device.is_connected = False
                device.save()
                alert_msg = f"THÀNH CÔNG: Đã gỡ bỏ thiết bị '{device.name}' (ID: {device.id})."
            else:
                message = f"Tìm thấy {count} thiết bị trùng khớp. Vui lòng chọn đúng dữ liệu để xóa"

    # Trả về trang HTML kèm theo biến 'alert_message'
    return render(request, 'remove_device.html', 
                {'alert_message': alert_msg, 
                'found_devices': found_devices, 
                'count': count})
