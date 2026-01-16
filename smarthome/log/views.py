from django.shortcuts import render, redirect
from django.contrib.auth.models import User
from django.contrib.auth import authenticate, login, logout
from django.contrib.auth.decorators import login_required

# Create your views here.
def sign_up(request):
    if request.method == "POST":
        username = request.POST.get("username")
        password1 = request.POST.get("password1")
        password2 = request.POST.get("password2")

        if password1 != password2:
            return render(request, "sign_up.html", {"error": "Mật khẩu không trùng khớp"})

        # Kiểm tra user đã tồn tại chưa
        if User.objects.filter(username=username).exists():
            return render(request, "sign_up.html", {"error": "Tài khoản đã tồn tại"})

        # Tạo user mới
        new_user = User.objects.create_user(username=username, password=password1)
        login(request, new_user)  
        # đăng nhập ngay sau khi đăng ký
        return redirect("log_app:log_in")

    return render(request, "sign_up.html")

# đăng nhập
def log_in(request):
    if request.method == "POST":
        username = request.POST.get("username")
        password = request.POST.get("password")

        user = authenticate(request, username=username, password=password)

        if user is not None:
            login(request, user)
            return redirect("dashboard_app:dashboard")
        else:
            return render(request, "log_in.html", {"error": "Sai tài khoản hoặc mật khẩu"})
        
    return render(request, "log_in.html")

# đăng xuất
@login_required
def log_out(request):
    logout(request)
    return redirect("log_app:log_in")