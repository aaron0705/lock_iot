from django.db import models
from django.utils import timezone

class SmartDevice(models.Model):

    # --- Thông tin cơ bản ---
    name = models.CharField(max_length=100, verbose_name="Tên thiết bị")
    device_type = models.CharField(max_length=50, default='light')
    location = models.CharField(max_length=100, default="Phòng khách")
    
    # --- Kết nối MQTT ---
    mqtt_topic = models.CharField(max_length=255, unique=True, help_text="Topic MQTT duy nhất")
    
    # --- Trạng thái (Yêu cầu của bạn) ---
    status = models.CharField(max_length=50, default="OFF", verbose_name="Trạng thái hiện tại")
    is_online = models.BooleanField(default=False, verbose_name="Đang kết nối?")
    
    # --- Thời gian (Yêu cầu của bạn) ---
    first_used = models.DateTimeField(auto_now_add=True, verbose_name="Lần đầu sử dụng")
    last_used = models.DateTimeField(null=True, blank=True, verbose_name="Lần sử dụng cuối")
    last_maintenance = models.DateTimeField(null=True, blank=True, verbose_name="Lần bảo trì cuối")

    def __str__(self):
        return f"{self.name} ({self.location})"

    def save(self, *args, **kwargs):
        # Bước 1: Kiểm tra xem đây có phải là tạo mới không (chưa có ID)
        is_new = self.pk is None 
        
        # Bước 2: Lưu dữ liệu xuống DB trước để lấy ID
        super().save(*args, **kwargs) 
        
        # Bước 3: Nếu là tạo mới và chưa có topic, thì cập nhật lại topic
        if is_new and not self.mqtt_topic:
            # Lúc này self.pk (ID) đã tồn tại
            self.mqtt_topic = f"iot/smarthome/{self.location}/{self.name}/{self.pk}"
            
            # Bước 4: Cập nhật lại cột mqtt_topic vào DB
            # Dùng update() để tránh gọi hàm save() đệ quy vô tận
            SmartDevice.objects.filter(pk=self.pk).update(mqtt_topic=self.mqtt_topic)

    class Meta:
        verbose_name = "Thiết bị thông minh"
        verbose_name_plural = "Quản lý thiết bị"