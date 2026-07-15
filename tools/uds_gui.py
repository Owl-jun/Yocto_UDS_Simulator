#!/usr/bin/env python3
import socket
import threading
import tkinter as tk
from tkinter import messagebox
from tkinter import ttk


DID_NAMES = {
    "F190": "VIN",
    "F187": "Software Version",
    "F188": "Hardware Version",
    "F18C": "Serial Number",
}

NRC_NAMES = {
    "11": "Service not supported",
    "12": "Sub-function not supported",
    "13": "Incorrect message length or invalid format",
    "22": "Conditions not correct",
    "24": "Request sequence error",
    "31": "Request out of range",
    "35": "Invalid key",
}

REQUESTS = [
    ("Session", "Enter Default Session", "10 01"),
    ("Session", "Enter Extended Session", "10 03"),
    ("DID", "Read VIN", "22 F1 90"),
    ("DID", "Read Software Version", "22 F1 87"),
    ("DID", "Read Hardware Version", "22 F1 88"),
    ("DID", "Read Serial Number", "22 F1 8C"),
    ("DTC", "Read All DTCs", "19 02 FF"),
    ("DTC", "Read confirmedDTC", "19 02 08"),
    ("DTC", "Clear All DTCs", "14 FF FF FF"),
    ("Security", "Request Seed", "27 01"),
    ("Security", "Send Key", "27 02 ED CB"),
    ("Routine", "Start Self-test", "31 01 FF 00"),
    ("Routine", "Read Self-test Result", "31 03 FF 00"),
    ("Routine", "Stop Self-test", "31 02 FF 00"),
    ("Reset", "Hard Reset", "11 01"),
    ("Reset", "Soft Reset", "11 03"),
]


def bytes_from_hex(line):
    return [int(token.removeprefix("0x"), 16) for token in line.split()]


def ascii_from_bytes(values):
    return "".join(chr(value) if 32 <= value <= 126 else "." for value in values)


def decode_request(request):
    parts = request.split()
    if not parts:
        return "Empty request"

    sid = parts[0].upper()
    if sid == "10" and len(parts) >= 2:
        return "Diagnostic Session Control: " + ("Default Session" if parts[1] == "01" else "Extended Session")
    if sid == "11" and len(parts) >= 2:
        return "ECU Reset: " + ("Hard Reset" if parts[1] == "01" else "Soft Reset")
    if sid == "14":
        return "Clear Diagnostic Information: all DTC group"
    if sid == "19" and len(parts) >= 3:
        return f"Read DTC Information: status mask 0x{parts[2].upper()}"
    if sid == "22" and len(parts) >= 3:
        did = parts[1].upper() + parts[2].upper()
        return "Read Data By Identifier: " + DID_NAMES.get(did, "DID 0x" + did)
    if sid == "27" and len(parts) >= 2:
        return "Security Access: " + ("request seed" if parts[1] == "01" else "send key")
    if sid == "2E" and len(parts) >= 3:
        did = parts[1].upper() + parts[2].upper()
        return "Write Data By Identifier: " + DID_NAMES.get(did, "DID 0x" + did)
    if sid == "31" and len(parts) >= 4:
        actions = {"01": "start", "02": "stop", "03": "request result"}
        routine = parts[2].upper() + parts[3].upper()
        return f"Routine Control: {actions.get(parts[1], 'unknown')} routine 0x{routine}"

    return "Raw UDS request"


def decode_response(response):
    parts = response.split()
    if not parts:
        return "No response"

    sid = parts[0].upper()
    if sid == "7F" and len(parts) >= 3:
        nrc = parts[2].upper()
        return f"Negative Response for SID 0x{parts[1].upper()}: {NRC_NAMES.get(nrc, 'NRC 0x' + nrc)}"
    if sid == "50" and len(parts) >= 2:
        return "Session changed: " + ("Default Session" if parts[1] == "01" else "Extended Session")
    if sid == "51" and len(parts) >= 2:
        return "ECU reset accepted"
    if sid == "54":
        return "DTCs cleared"
    if sid == "59" and len(parts) >= 3:
        count = max(0, (len(parts) - 3) // 4)
        if count == 0:
            return "No DTC matched the requested status mask"
        return f"{count} DTC record(s) returned"
    if sid == "62" and len(parts) >= 3:
        did = parts[1].upper() + parts[2].upper()
        value = ascii_from_bytes(bytes_from_hex(" ".join(parts[3:])))
        return f"{DID_NAMES.get(did, 'DID 0x' + did)} = {value}"
    if sid == "67" and len(parts) >= 2:
        if parts[1] == "01" and len(parts) >= 4:
            return f"Seed received: 0x{parts[2].upper()}{parts[3].upper()}"
        return "Security unlocked"
    if sid == "6E" and len(parts) >= 3:
        did = parts[1].upper() + parts[2].upper()
        return f"{DID_NAMES.get(did, 'DID 0x' + did)} write accepted"
    if sid == "71" and len(parts) >= 5:
        statuses = {"00": "OK/stopped", "01": "running"}
        return f"Routine response: {statuses.get(parts[4], 'status 0x' + parts[4].upper())}"

    return "Positive response"


class UdsGui:
    def __init__(self, root):
        self.root = root
        self.root.title("UDS Diagnostic Tester")
        self.root.geometry("1040x680")
        self.sock = None
        self.sock_file = None
        self.lock = threading.Lock()

        self.host_var = tk.StringVar(value="127.0.0.1")
        self.port_var = tk.StringVar(value="5000")
        self.status_var = tk.StringVar(value="Disconnected")
        self.custom_var = tk.StringVar()
        self.serial_var = tk.StringVar(value="SIM-0001")

        self.build_ui()

    def build_ui(self):
        root = self.root
        root.columnconfigure(0, weight=0)
        root.columnconfigure(1, weight=1)
        root.rowconfigure(1, weight=1)

        connection = ttk.Frame(root, padding=10)
        connection.grid(row=0, column=0, columnspan=2, sticky="ew")
        connection.columnconfigure(6, weight=1)

        ttk.Label(connection, text="Host").grid(row=0, column=0, sticky="w")
        ttk.Entry(connection, textvariable=self.host_var, width=18).grid(row=0, column=1, padx=(6, 12))
        ttk.Label(connection, text="Port").grid(row=0, column=2, sticky="w")
        ttk.Entry(connection, textvariable=self.port_var, width=8).grid(row=0, column=3, padx=(6, 12))
        ttk.Button(connection, text="Connect", command=self.connect).grid(row=0, column=4, padx=(0, 6))
        ttk.Button(connection, text="Disconnect", command=self.disconnect).grid(row=0, column=5)
        ttk.Label(connection, textvariable=self.status_var).grid(row=0, column=6, sticky="e")

        left_outer = ttk.Frame(root, padding=(10, 0, 6, 10))
        left_outer.grid(row=1, column=0, sticky="ns")
        left_outer.rowconfigure(0, weight=1)
        left_outer.columnconfigure(0, weight=1)

        left_canvas = tk.Canvas(left_outer, width=260, highlightthickness=0)
        left_canvas.grid(row=0, column=0, sticky="ns")

        left_scroll = ttk.Scrollbar(left_outer, orient="vertical", command=left_canvas.yview)
        left_scroll.grid(row=0, column=1, sticky="ns")
        left_canvas.configure(yscrollcommand=left_scroll.set)

        left = ttk.Frame(left_canvas)
        left.columnconfigure(0, weight=1)
        left_window = left_canvas.create_window((0, 0), window=left, anchor="nw")

        def update_left_scroll_region(_event):
            left_canvas.configure(scrollregion=left_canvas.bbox("all"))

        def update_left_width(event):
            left_canvas.itemconfigure(left_window, width=event.width)

        def scroll_left(event):
            left_canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")

        left.bind("<Configure>", update_left_scroll_region)
        left_canvas.bind("<Configure>", update_left_width)
        left_canvas.bind("<Enter>", lambda _event: left_canvas.bind_all("<MouseWheel>", scroll_left))
        left_canvas.bind("<Leave>", lambda _event: left_canvas.unbind_all("<MouseWheel>"))

        grouped = {}
        for group, label, request in REQUESTS:
            grouped.setdefault(group, []).append((label, request))

        row = 0
        for group, items in grouped.items():
            frame = ttk.LabelFrame(left, text=group, padding=8)
            frame.grid(row=row, column=0, sticky="ew", pady=(0, 8))
            frame.columnconfigure(0, weight=1)
            row += 1
            for index, (label, request) in enumerate(items):
                ttk.Button(frame, text=label, command=lambda req=request: self.send(req)).grid(
                    row=index, column=0, sticky="ew", pady=2
                )

        write_frame = ttk.LabelFrame(left, text="Write Serial Number", padding=8)
        write_frame.grid(row=row, column=0, sticky="ew", pady=(0, 8))
        write_frame.columnconfigure(0, weight=1)
        ttk.Entry(write_frame, textvariable=self.serial_var).grid(row=0, column=0, sticky="ew", pady=(0, 6))
        ttk.Button(write_frame, text="Write Serial Number", command=self.write_serial).grid(row=1, column=0, sticky="ew")
        row += 1

        custom_frame = ttk.LabelFrame(left, text="Custom Request", padding=8)
        custom_frame.grid(row=row, column=0, sticky="ew")
        custom_frame.columnconfigure(0, weight=1)
        ttk.Entry(custom_frame, textvariable=self.custom_var).grid(row=0, column=0, sticky="ew", pady=(0, 6))
        ttk.Button(custom_frame, text="Send Raw Hex", command=self.send_custom).grid(row=1, column=0, sticky="ew")

        right = ttk.Frame(root, padding=(6, 0, 10, 10))
        right.grid(row=1, column=1, sticky="nsew")
        right.rowconfigure(0, weight=1)
        right.columnconfigure(0, weight=1)

        columns = ("direction", "meaning", "hex")
        self.log = ttk.Treeview(right, columns=columns, show="headings")
        self.log.heading("direction", text="Direction")
        self.log.heading("meaning", text="Meaning")
        self.log.heading("hex", text="Raw Hex")
        self.log.column("direction", width=90, stretch=False)
        self.log.column("meaning", width=520)
        self.log.column("hex", width=300)
        self.log.grid(row=0, column=0, sticky="nsew")

        scroll = ttk.Scrollbar(right, orient="vertical", command=self.log.yview)
        scroll.grid(row=0, column=1, sticky="ns")
        self.log.configure(yscrollcommand=scroll.set)

        controls = ttk.Frame(right)
        controls.grid(row=1, column=0, sticky="ew", pady=(8, 0))
        ttk.Button(controls, text="Clear Log", command=self.clear_log).pack(side="right")

    def connect(self):
        if self.sock:
            return
        try:
            port = int(self.port_var.get())
            self.sock = socket.create_connection((self.host_var.get(), port), timeout=5.0)
            self.sock_file = self.sock.makefile("rb")
            self.status_var.set(f"Connected to {self.host_var.get()}:{port}")
        except Exception as error:
            self.sock = None
            self.sock_file = None
            messagebox.showerror("Connection failed", str(error))

    def disconnect(self):
        if self.sock_file:
            self.sock_file.close()
        if self.sock:
            self.sock.close()
        self.sock = None
        self.sock_file = None
        self.status_var.set("Disconnected")

    def send_custom(self):
        self.send(self.custom_var.get())

    def write_serial(self):
        value = " ".join(f"{ord(char):02X}" for char in self.serial_var.get())
        self.send("2E F1 8C " + value)

    def send(self, request):
        request = " ".join(request.strip().split()).upper()
        if not request:
            return
        if not self.sock or not self.sock_file:
            self.connect()
            if not self.sock:
                return
        threading.Thread(target=self.send_worker, args=(request,), daemon=True).start()

    def send_worker(self, request):
        try:
            with self.lock:
                self.sock.sendall((request + "\n").encode("ascii"))
                response = self.sock_file.readline().decode("ascii").strip()
                if not response:
                    raise ConnectionError("server closed the connection")
            self.root.after(0, self.add_exchange, request, response)
        except Exception as error:
            self.root.after(0, self.handle_send_error, error)

    def add_exchange(self, request, response):
        self.log.insert("", "end", values=("Request", decode_request(request), request))
        self.log.insert("", "end", values=("Response", decode_response(response), response))
        self.log.yview_moveto(1.0)

    def handle_send_error(self, error):
        self.disconnect()
        messagebox.showerror("Send failed", str(error))

    def clear_log(self):
        for item in self.log.get_children():
            self.log.delete(item)


def main():
    root = tk.Tk()
    UdsGui(root)
    root.mainloop()


if __name__ == "__main__":
    main()
