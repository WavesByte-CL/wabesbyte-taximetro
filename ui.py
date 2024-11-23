import tkinter as tk
from tkinter import ttk, messagebox
import queue
from esptool_exec import FirebaseLogin
import os
import sys

def resource_path(relative_path):
    base_path = getattr(sys, '_MEIPASS', os.path.dirname(os.path.abspath(__file__)))
    return os.path.join(base_path, relative_path)

class TaximeterMonitorApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Wavesbyte Taximeter Monitor")
        self.geometry("400x300")
        self.resizable(False, False)
        icon_path = resource_path('taximetro.ico')
        self.iconbitmap(icon_path)

        self.username = tk.StringVar()
        self.password = tk.StringVar()
        self.queue = queue.Queue()

        self.login_frame = ttk.Frame(self)
        self.login_frame.pack(pady=20)
        self.create_login_widgets()

        self.device_frame = ttk.Frame(self)
        self.device_list = None

    def create_login_widgets(self):
        ttk.Label(self.login_frame, text="Email:").pack()
        username_entry = ttk.Entry(self.login_frame, textvariable=self.username)
        username_entry.pack()

        ttk.Label(self.login_frame, text="Password:").pack()
        password_entry = ttk.Entry(self.login_frame, textvariable=self.password, show="*")
        password_entry.pack()

        ttk.Button(self.login_frame, text="Login", command=self.login).pack(pady=10)

    def create_device_list(self):
        self.device_list = ttk.Treeview(self.device_frame, columns=('ID', 'Status'), show='headings')
        self.device_list.heading('ID', text='ID del Taxímetro')
        self.device_list.heading('Status', text='Estado')
        self.device_list.pack(expand=True, fill='both')

    def update_device_list(self):
        try:
            while True:
                mac_address, status = self.queue.get_nowait()
                found = False
                for child in self.device_list.get_children():
                    if self.device_list.item(child, 'values')[0] == mac_address:
                        self.device_list.item(child, values=(mac_address, status))
                        found = True
                        break
                if not found:
                    self.device_list.insert('', 'end', values=(mac_address, status))
        except queue.Empty:
            pass
        self.after(1000, self.update_device_list)

    def login(self):
        email = self.username.get()
        password = self.password.get()
        if email and password:
            login_success = FirebaseLogin(email, password)
            if login_success:
                self.login_frame.pack_forget()
                self.device_frame.pack(expand=True, fill='both')
                self.create_device_list()
                self.update_device_list()
            else:
                messagebox.showerror("Error de Login", "Credenciales incorrectas, intente nuevamente.")
        else:
            messagebox.showerror("Error de Login", "Por favor, ingrese un email y contraseña válidos.")

def main():
    app = TaximeterMonitorApp()
    app.mainloop()

if __name__ == '__main__':
    main()
