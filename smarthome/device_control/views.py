<<<<<<< HEAD
from django.shortcuts import render
from django.contrib.auth.decorators import login_required
from datetime import datetime
from .models import SmartDevice
from django.db.models import Q
=======
from django.shortcuts import render, redirect
from django.contrib.auth.decorators import login_required
from django.contrib import messages
from datetime import datetime
from .models import smart_devices
from django.db.models import Q
from django.views.decorators.csrf import csrf_protect
import paho.mqtt.client as mqtt
import base64

key = b"1234567890123456"
iv = b"1234567890123456"

broker = "piserver"
>>>>>>> a02b74048eed94e2825a54c58349d894719e27c2

# Create your views here.
@login_required
def device_control(request):
<<<<<<< HEAD
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

=======
    devices = smart_devices.objects.only('id', 'name')
    return render(request, 'device_control.html', {'devices': devices})

@login_required
def device_detail(request, device_id):
    device = smart_devices.objects.get(id=device_id)
    if request.method == "POST":
        command = request.POST.get('action')
        topic = device.mqtt_topic
        mqtt_client = mqtt_init()
        try:
            if mqtt_client and mqtt_client.is_connected():
                success = mqtt_client.publish(topic, command, qos=1)
                if success:
                    messages.success(request, f"Lệnh {command.upper()} đã được gửi tới thiết bị thành công!")
                else:
                    messages.error(request, "Gửi lệnh thất bại. Vui lòng kiểm tra lại kết nối.")
            else:
                messages.error(request, "Lỗi: MQTT Server chưa kết nối!")
        except Exception as e:
            messages.error(request, f"Lỗi hệ thống: {str(e)}")

    return render(request, 'device_detail.html', {
        'device': device
    })

@login_required
@csrf_protect
def add_device(request):
    alert_msg = None
    if request.method == "POST":
        new_device = smart_devices.objects.create(
            name = request.POST.get('name'),
            current_status = 'OFF',
            device_type = request.POST.get('type'),
            location = request.POST.get('location'),
            is_connected = 1,
            first_used =  datetime.now(),
            last_used = datetime.now(),
            last_maintenance = datetime.now(),
            description = request.POST.get('description')
        )
        new_device.save()
        alert_msg = "Thêm thiết bị thành công!"
        return redirect('device_control_app:device_control')
    return render(request, 'add.html', {'alert_msg': alert_msg})

@login_required
def remove_device_view(request):
    alert_msg = None  
    found_devices = None
    count = 0
>>>>>>> a02b74048eed94e2825a54c58349d894719e27c2
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
<<<<<<< HEAD
    return render(request, 'remove_device.html', 
                {'alert_message': alert_msg, 
                'found_devices': found_devices, 
                'count': count})
=======
    return render(request, 'remove.html', 
                {'alert_message': alert_msg, 
                'found_devices': found_devices, 
                'count': count})

def aes_encrypt(plain_text):
    cipher = AES.new(key, AES.MODE_CBC, iv)
    try:
        # 1. Chuyển plain_text sang bytes (nếu nó đang là chuỗi)
        if isinstance(plain_text, str):
            plain_bytes = plain_text.encode('utf-8')
        else:
            plain_bytes = plain_text

        padded_data = pad(plain_bytes, AES.block_size, style='pkcs7')

        cipher_bytes = cipher.encrypt(padded_data)

        return base64.b64encode(cipher_bytes).decode('utf-8')

    except ValueError as e:
        print(f"Lỗi giá trị: {e}")
    except Exception as e:
        print(f"Lỗi không xác định: {e}")

def aes_decrypt(cipher_text):
    cipher = AES.new(key, AES.MODE_CBC, iv)
    try:
        cipher_bytes = base64.b64decode(cipher_text)
        plain_bytes = cipher.decrypt(cipher_bytes)
        plain_text = unpad(plain_bytes, AES.block_size).decode('utf-8')
        return plain_text
    except ValueError:
        print("Unvalidate")

def on_connect(client, userdate, flags, rc):
    print("Connected")
    client.subscribe(topic)

def on_message(client, userdata, message):
    print("Message received: ", message.payload)
    plain_text = aes_decrypt(message.payload)
    if plain_text:
        print(f"Plain text is {plain_text}")
        if (is_json(message.payload)):
            save_event(message.payload)
    else:
        print("")


def pub_message(command, topic):
    try:
        message = {}
        message["command"] = command
        message["value"] = 100 if message["command"]=="adjust" else None
        json_message = loads(message)
        encrypted_json_message = aes_encrypt(json_message)
        client.publish(topic, encrypted_json_message)
        return True
    except Exception:
        return False


def mqtt_init():
    # 1. Khởi tạo đối tượng Client (Thêm protocol để ổn định hơn)
    client = mqtt.Client(client_id="Django_Backend_Server", protocol=mqtt.MQTTv311)
    
    # 2. Gán các hàm xử lý
    client.on_message = on_message
    client.on_connect = on_connect
    
    # 3. Bảo mật
    client.username_pw_set("user1", "123456")
    
    try:
        # 4. Kết nối
        client.connect(broker, 1883, 60)
        
        # 5. ĐIỂM CẢI TIẾN QUAN TRỌNG: 
        # Thay loop_forever() bằng loop_start()
        client.loop_start() 
        
        print("MQTT Loop started in background...")
        return client
    except Exception as e:
        print(f"Lỗi kết nối MQTT: {e}")
        return None
>>>>>>> a02b74048eed94e2825a54c58349d894719e27c2
