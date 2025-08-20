import tkinter as tk
from tkinter import messagebox, ttk
import serial
import threading
import time
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

SERIAL_PORT = 'COM4'
BAUD_RATE = 9600

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
except Exception as e:
    ser = None
    print(f"Failed to open serial port. Check the COM port and try again.\nError: {e}")

root = tk.Tk()
root.title("8051 Sensor + Actuator Interface via NodeMCU")
root.geometry("900x700")
root.config(bg="#1e293b")

label = ttk.Label(root, text='8051 Sensor + Actuator Interface',
                 font=('Helvetica', 18, 'bold'), background="#1e293b",
                 foreground="#fbbf24")
label.pack(pady=20)

paned = ttk.Panedwindow(root, orient='horizontal')
paned.pack(fill='both', expand=1)

left_frame = ttk.Frame(paned)
paned.add(left_frame, weight=1)

right_frame = ttk.Frame(paned)
paned.add(right_frame, weight=2)

left_label = ttk.Label(left_frame, text='Select Sensor',
                       font=('Helvetica', 14, 'bold'))
left_label.pack(pady=10)

temperature_data = []
humidity_data = []
index_data = []

sensor_plot_data = {
    "Touch": [], "IR": [], "Rain": [], "Gas": [], "Flame": [],
    "Soil": [], "Sound": [], "Vibration": [], "Proximity": []
}

sensor_index_data = {k: [] for k in sensor_plot_data.keys()}

fig, ax = plt.subplots(figsize=(5, 3), dpi=100)
canvas = FigureCanvasTkAgg(fig, master=None)

def update_plot(sensor_name="DHT11"):
    ax.clear()
    if sensor_name == "DHT11":
        if index_data:
            ax.plot(index_data, temperature_data, label='Temp (°C)', color='red', marker='o')
            ax.plot(index_data, humidity_data, label='Humidity (%)', color='blue', marker='x')
            ax.set_ylim(0, 100)
            ax.set_xlim(1, max(index_data[-1], 10))
    else:
        data = sensor_plot_data[sensor_name]
        idx = sensor_index_data[sensor_name]
        if data:
            ax.step(idx, data, label=f'{sensor_name} (0/1)', color='green', where='mid')
            ax.set_ylim(-0.5, 1.5)
            ax.set_xlim(1, max(idx[-1], 10))
    ax.set_title(f"{sensor_name} Plot")
    ax.set_xlabel("Readings")
    ax.set_ylabel("Value")
    ax.legend()
    ax.grid(True)
    canvas.draw()

def parse_sensor_value(sensor, text):
    binary_map = {
        "Touched": 1, "Not": 0,
        "Object": 1, "Clear": 0,
        "Detected": 1, "None": 0,
        "Moist": 1, "Dry": 0,
        "Det": 1, "ON": 1, "OFF": 0
    }
    try:
        if ":" in text:
            key, val = text.split(":", 1)
            val = val.strip().split()[0]
            if val in binary_map:
                return binary_map[val]
            return float(val)
    except:
        pass
    return None

def send_command(cmd, sensor_name):
    if ser and ser.is_open:
        try:
            ser.write(cmd.encode())
        except Exception as e:
            return f"Error sending command: {e}\n"
    else:
        return "Serial port not open\n"

    response_lines = []
    start_time = time.time()
    timeout = 5
    hum = temp = None
    try:
        while time.time() - start_time < timeout:
            try:
                line = ser.readline()
                if line:
                    text = line.decode(errors='replace').strip()
                    response_lines.append(text)
                    if sensor_name == "DHT11":
                        if "Hum:" in text:
                            hum = float(text.split(":")[1].replace("%", "").strip())
                        elif "Temp:" in text:
                            temp = float(text.split(":")[1].replace("C", "").strip())
                        if hum is not None and temp is not None:
                            temperature_data.append(temp)
                            humidity_data.append(hum)
                            index_data.append(len(index_data) + 1)
                            update_plot("DHT11")
                            hum = temp = None
                    elif sensor_name in sensor_plot_data:
                        val = parse_sensor_value(sensor_name, text)
                        if val is not None:
                            sensor_plot_data[sensor_name].append(val)
                            sensor_index_data[sensor_name].append(len(sensor_index_data[sensor_name]) + 1)
                            update_plot(sensor_name)
                    if text == "DONE":
                        break
            except Exception as e:
                response_lines.append(f"Error reading: {e}")
                break
    except Exception as e:
        response_lines.append(f"Error while reading: {e}")
    return '\n'.join(response_lines)

def show_output(sensor_name, cmd_key):
    output_text.delete(1.0, tk.END)
    output_text.insert(tk.END, f"Fetching {sensor_name} data...\n")

    def read():
        response = send_command(cmd_key, sensor_name)
        output_text.delete(1.0, tk.END)
        output_text.insert(tk.END, response)
        output_text.see(tk.END)
        canvas.get_tk_widget().pack(expand=1, fill='both')

    threading.Thread(target=read).start()

sensors = [
    ("DHT11", "R"), ("Touch", "T"), ("IR", "I"), ("Rain", "N"),
    ("Gas", "G"), ("Flame", "F"), ("Soil", "S"),
    ("Sound", "A"), ("Vibration", "V"), ("Proximity", "P")
]

style = ttk.Style()
style.theme_use("clam")
sensor_color = "#90caf9"
style.configure("TButton",
                 font=('Helvetica', 12, 'bold'),
                 padding=20,
                 background=sensor_color,
                 foreground='#000022',
                 borderwidth=0)
style.map("TButton",
          background=[("active", sensor_color), ("disabled", sensor_color)])

grid_frame = ttk.Frame(left_frame)
grid_frame.pack(pady=10, padx=20, fill='both', expand=1)
columns = 2
for index, (sensor_name, cmd_key) in enumerate(sensors):
    row = index // columns
    col = index % columns
    btn = ttk.Button(grid_frame, text=sensor_name,
                     command=lambda n=sensor_name, k=cmd_key: show_output(n, k),
                     style='TButton')
    btn.grid(row=row, column=col, pady=5, padx=5, sticky='ew')
for col in range(columns):
    grid_frame.columnconfigure(col, weight=1)

# === Actuator Control ===
actuator_label = ttk.Label(left_frame, text='Control Actuators',
                           font=('Helvetica', 14, 'bold'))
actuator_label.pack(pady=10)

actuators = [
    ("Fan ON", "1"), ("Fan OFF", "2"),
    ("Buzzer ON", "3"), ("Buzzer OFF", "4"),
    ("Pump ON", "5"), ("Pump OFF", "6")
]

actuator_frame = ttk.Frame(left_frame)
actuator_frame.pack(pady=5, padx=20, fill='both', expand=1)

for index, (name, cmd) in enumerate(actuators):
    row = index // 2
    col = index % 2
    btn = ttk.Button(actuator_frame, text=name,
                     command=lambda k=cmd, n=name: show_output(n, k),
                     style='TButton')
    btn.grid(row=row, column=col, pady=5, padx=5, sticky='ew')
for col in range(2):
    actuator_frame.columnconfigure(col, weight=1)

right_label = ttk.Label(right_frame, text='Output',
                         font=('Helvetica', 14, 'bold'))
right_label.pack(pady=10)

vertical_pane = ttk.Panedwindow(right_frame, orient='vertical')
vertical_pane.pack(fill='both', expand=1, padx=20, pady=10)

output_frame = ttk.Frame(vertical_pane)
output_text = tk.Text(output_frame, wrap='word',
                      font=('Courier', 14, 'bold'),
                      bg="#fff7cc", fg="#000022")
output_text.pack(expand=1, fill='both')
vertical_pane.add(output_frame, weight=3)

graph_frame = ttk.Frame(vertical_pane)
canvas.get_tk_widget().pack(in_=graph_frame, expand=1, fill='both')
vertical_pane.add(graph_frame, weight=2)

root.mainloop()
