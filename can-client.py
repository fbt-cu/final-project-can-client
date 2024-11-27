import tkinter as tk
import tkinter.ttk as ttk
import socket
import threading

import time

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
close_app = False

def connect():
    global server_socket
    print("Attempting connection...")
    server_socket.connect((ent_server_ip.get(), 9000))

def disconnect():
    global server_socket
    print("Disconnecting...")
    server_socket.sendall(b"close")
    server_socket.shutdown(socket.SHUT_RDWR)
    server_socket.close()
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# can1 123 8 01 02 03 04 05 06 07 08

def can0_send():
    global server_socket
    print("Sending CAN0 frame...")
    server_socket.sendall(b"can0 " + ent_can0_id.get().encode('utf-8') + b" 8 " + ent_can0_payload.get().encode('utf-8'))

def can1_send():
    global server_socket
    print("Sending CAN1 frame...")
    server_socket.sendall(b"can1 " + ent_can1_id.get().encode('utf-8') + b" 8 " + ent_can1_payload.get().encode('utf-8'))

def receive_messages():
    global server_socket
    data = ""
    print("Thread 'receive_messages' launched.")
    time.sleep(5)
    while(close_app == False):
        try:
            data = (server_socket.recv(40))
        except OSError:
            print("OSError")
            time.sleep(1)
        else:
            if len(data) > 0:
                txt_server_messages.configure(state="normal")
                txt_server_messages.insert(tk.END, data.decode())
                txt_server_messages.configure(state="disabled")
                txt_server_messages.see(tk.END)
    print("Thread 'receive_messages' ending.")

window = tk.Tk()
window.title("Can-Client")
window.resizable(False, False)

# Connection
frm_connection = ttk.Frame()

ent_server_ip = ttk.Entry(master=frm_connection)
ent_server_ip.insert(0, "10.72.1.106")
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
ent_can0_id.insert(0, "123")
ent_can0_id.pack(side="left")
ent_can0_payload = ttk.Entry(master=frm_can0, width=30)
ent_can0_payload.insert(0, "00 11 22 33 44 55 66 77")
ent_can0_payload.pack(side="left")
btn_can0_send = ttk.Button(master=frm_can0, text="Send", command=can0_send)
btn_can0_send.pack(side="left")

# CAN1
frm_can1 = ttk.Frame()

ttk.Label(master=frm_can1, text="CAN1").pack(side="left")
ent_can1_id = ttk.Entry(master=frm_can1,  width=5)
ent_can1_id.insert(0, "124")
ent_can1_id.pack(side="left")
ent_can1_payload = ttk.Entry(master=frm_can1, width=30)
ent_can1_payload.insert(0, "FF EE DD CC BB AA 99 88")
ent_can1_payload.pack(side="left")
btn_can1_send = ttk.Button(master=frm_can1, text="Send", command=can1_send)
btn_can1_send.pack(side="left")

frm_connection.pack()
frm_server_messages.pack()
frm_can0.pack()
frm_can1.pack()

# Launch receive_messages thread
receive_messages_thread = threading.Thread(target=receive_messages)
receive_messages_thread.start()

window.mainloop()

disconnect()
close_app = True
receive_messages_thread.join()
print("Exiting app...")
