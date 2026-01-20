from django.db import models
from django.utils import timezone

class smart_devices(models.Model):

    name = models.CharField(max_length=100, verbose_name="Tên thiết bị")
    device_type = models.CharField(max_length=50, default='light')
    location = models.CharField(max_length=100, default="Phòng khách")
    mqtt_topic = models.CharField(max_length=255, unique=True, help_text="Topic MQTT duy nhất")
    current_status = models.CharField(max_length=50, default="OFF", verbose_name="Trạng thái hiện tại")
    is_connected = models.IntegerField(default=0, verbose_name="Trạng thái kết nối (0: Tắt, 1: Bật)")
    
    
    first_used = models.DateTimeField(auto_now_add=True, verbose_name="Lần đầu sử dụng")
    last_used = models.DateTimeField(null=True, blank=True, verbose_name="Lần sử dụng cuối")
    last_maintenance = models.DateTimeField(null=True, blank=True, verbose_name="Lần bảo trì cuối")
    description = models.TextField(null=True, blank=True, verbose_name="Mô tả chi tiết")
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
            self.mqtt_topic = f"iot/smarthome/{self.location}/{self.device_type}/{self.pk}"
            
            # Bước 4: Cập nhật lại cột mqtt_topic vào DB
            # Dùng update() để tránh gọi hàm save() đệ quy vô tận
            smart_devices.objects.filter(pk=self.pk).update(mqtt_topic=self.mqtt_topic)

    class Meta:
        db_table = 'smart_devices'
        verbose_name = "Thiết bị thông minh"
        verbose_name_plural = "Quản lý thiết bị"