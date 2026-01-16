
import base64
import paho.mqtt.client as mqtt
from Crypto.Cipher import AES
from Crypto.Util.Padding import unpad
import mysql.connector

key = b"1234567890123456"
iv = b"1234567890123456"
topic = "pi/smarthome"

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

def save_event(json_message):
    cnx = mysql.connector.connect(
    host="127.0.0.1",
    port=3306,
    user="root",
    password="")
    cur = cnx.cursor()
    # Sử dụng 3 dấu ngoặc kép để tránh lỗi xung đột dấu ngoặc bên trong
    sql_query = """
        INSERT INTO history ("Date and Time", "Command", "Device", "Callback", "User")
        VALUES (%s, %s, %s, %s, %s)
    """

    # Gom dữ liệu vào 1 tuple riêng cho dễ nhìn và dễ debug
    data_values = (
        json_message["datetime"], 
        json_message["command"], 
        json_message["device"],   # Đã sửa lỗi chính tả json_mesage
        json_message["callback"], # Bổ sung callback cho khớp với cột thứ 4
        json_message["user"]      # User là cột thứ 5
    )

    # Thực thi
    cur.execute(sql_query, data_values)  
    cnx.commit()
    cursor.close()
    cnx.close()

def is_json(my_str): 
    try: 
        json.loads(my_str) # thử parse chuỗi 
        return True 
    except ValueError: 
        return False

def on_message(client, userdata, message):
    print("Message received: ", message.payload)
    plain_text = aes_decrypt(message.payload)
    if plain_text:
        print(f"Plain text is {plain_text}")
        if (is_json(message.payload)):
            save_event(message.payload)
    else:
        print("")

def main():
    broker = "piserver"
    port = 1883
    client = mqtt.Client()
    client.on_message = on_message
    client.on_connect = on_connect
    client.username_pw_set("user1", "123456")
    client.connect('localhost', 1883, 60)
    client.loop_forever()

if __name__ == "__main__":
    main()
