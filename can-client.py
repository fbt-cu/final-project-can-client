import tkinter as tk
import tkinter.ttk as ttk

def connect():
    print("Attempting connection...")

def disconnect():
    print("Disconnecting...")

def can0_send():
    print("Sending CAN0 frame...")

def can1_send():
    print("Sending CAN1 frame...")


window = tk.Tk()
window.title("Can-Client")
window.resizable(False, False)

# Connection
frm_connection = ttk.Frame()

ent_server_ip = ttk.Entry(master=frm_connection)
ent_server_ip.pack(side="left")

btn_connect = ttk.Button(master=frm_connection, text="Connect", command=connect)
btn_connect.pack(side="left")

btn_disconnect = ttk.Button(master=frm_connection, text="Disconnect", command=disconnect)
btn_disconnect.pack(side="left")

# Server messages
frm_server_messages = ttk.Frame()

txt_server_messages = tk.Text(master=frm_server_messages)
# txt_server_messages.insert("0.0", "This is a test")
txt_server_messages.configure(state="disabled")
txt_server_messages.pack()

# Outgoing messages
# CAN0
frm_can0 = ttk.Frame()

ttk.Label(master=frm_can0, text="CAN0").pack(side="left")
ent_can0_id = ttk.Entry(master=frm_can0, width=5)
ent_can0_id.insert(0, "ID")
ent_can0_id.pack(side="left")
ent_can0_payload = ttk.Entry(master=frm_can0, width=20)
ent_can0_payload.insert(0, "Payload")
ent_can0_payload.pack(side="left")
btn_can0_send = ttk.Button(master=frm_can0, text="Send", command=can0_send)
btn_can0_send.pack(side="left")

# CAN1
frm_can1 = ttk.Frame()

ttk.Label(master=frm_can1, text="CAN1").pack(side="left")
ent_can1_id = ttk.Entry(master=frm_can1,  width=5)
ent_can1_id.insert(0, "ID")
ent_can1_id.pack(side="left")
ent_can1_payload = ttk.Entry(master=frm_can1, width=20)
ent_can1_payload.insert(0, "Payload")
ent_can1_payload.pack(side="left")
btn_can1_send = ttk.Button(master=frm_can1, text="Send", command=can1_send)
btn_can1_send.pack(side="left")

frm_connection.pack()
frm_server_messages.pack()
frm_can0.pack()
frm_can1.pack()

window.mainloop()

