import test
from flask import Flask, Response, render_template
import threading, time, random

app = Flask(__name__)

# Biến dùng chung chứa số float (cập nhật bởi background thread)
current_value = {"v": 0.0}

def updater():
    """Background thread: cập nhật giá trị mỗi 5s (ví dụ: random float)."""
    while True:
        # update logic: ở đây dùng random để mô phỏng; thay bằng logic thật nếu cần
        #current_value["v"] = round(random.uniform(0, 100), 3)
        current_value["v"] = test.value
        print("Updated value to:", current_value["v"])
        time.sleep(1)

@app.route('/')
def index():
    return render_template('web.html')

def event_stream():
    """Generator trả về dữ liệu SSE. Gửi value hiện tại mỗi 5s."""
    while True:
        # Gửi theo định dạng SSE: data: <text>\n\n
        yield f"data: {current_value['v']}\n\n"
        time.sleep(1)

@app.route('/stream')
def stream():
    return Response(event_stream(), mimetype='text/event-stream')

if __name__ == '__main__':
    # start background updater thread
    
    if not hasattr(test, 'value'):
        test.value = 0.0  # Giá trị mặc định, tùy chỉnh nếu cần
    
    # Chạy MQTT ở thread riêng để không chặn Flask
    mqtt_thread = threading.Thread(target=test.start_mqtt)
    mqtt_thread.daemon = True  # Thread sẽ tự tắt khi app chính tắt
    mqtt_thread.start()

    t1 = threading.Thread(target=updater, daemon=True)
    t1.start()
    app.run(debug=True, threaded=True, host='0.0.0.0', port=5000)



