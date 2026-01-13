import tkinter as tk
from tkinter import ttk, messagebox
import paho.mqtt.client as mqtt
import json
import threading

# --- CONFIGURAÇÕES ---
BROKER_IP = "192.168.1.6"  # IP da sua Raspberry Pi
BROKER_PORT = 1883
TOPIC_CMD = "bomba/comando"
TOPIC_STATUS = "bomba/status"

class InfusionPumpGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Monitor de Infusão IoT")
        self.root.geometry("600x500")
        
        # Variáveis de Interface
        self.var_status = tk.StringVar(value="Desconectado")
        self.var_vol_infused = tk.StringVar(value="0.0 ml")
        self.var_pressure = tk.StringVar(value="0 mmHg")
        self.var_flow_curr = tk.StringVar(value="0 ml/h")
        self.var_alarm = tk.StringVar(value="OK")
        
        # Variáveis de Configuração
        self.entry_vol = tk.DoubleVar(value=100.0)
        self.entry_rate = tk.DoubleVar(value=125.0)
        self.combo_mode = tk.StringVar(value="Contínuo")

        # --- LAYOUT ---
        self.create_widgets()
        
        # --- MQTT ---
        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        
        # Inicia conexão em thread separada para não travar a GUI
        threading.Thread(target=self.mqtt_connect, daemon=True).start()

    def create_widgets(self):
        # 1. Painel de Monitoramento (Topo)
        frame_mon = tk.LabelFrame(self.root, text="Monitoramento em Tempo Real", padx=10, pady=10, font=("Arial", 12, "bold"))
        frame_mon.pack(fill="x", padx=10, pady=5)

        # Grid de Labels
        tk.Label(frame_mon, text="Estado:", font=("Arial", 10)).grid(row=0, column=0, sticky="e")
        lbl_state = tk.Label(frame_mon, textvariable=self.var_status, font=("Arial", 12, "bold"), fg="blue")
        lbl_state.grid(row=0, column=1, sticky="w", padx=10)

        tk.Label(frame_mon, text="Volume Infundido:", font=("Arial", 10)).grid(row=1, column=0, sticky="e")
        tk.Label(frame_mon, textvariable=self.var_vol_infused, font=("Arial", 12)).grid(row=1, column=1, sticky="w", padx=10)

        tk.Label(frame_mon, text="Vazão Atual:", font=("Arial", 10)).grid(row=0, column=2, sticky="e")
        tk.Label(frame_mon, textvariable=self.var_flow_curr, font=("Arial", 12)).grid(row=0, column=3, sticky="w", padx=10)

        tk.Label(frame_mon, text="Pressão:", font=("Arial", 10)).grid(row=1, column=2, sticky="e")
        tk.Label(frame_mon, textvariable=self.var_pressure, font=("Arial", 12)).grid(row=1, column=3, sticky="w", padx=10)

        tk.Label(frame_mon, text="Alarme:", font=("Arial", 10)).grid(row=2, column=0, sticky="e")
        self.lbl_alarm = tk.Label(frame_mon, textvariable=self.var_alarm, font=("Arial", 12, "bold"), fg="green")
        self.lbl_alarm.grid(row=2, column=1, sticky="w", padx=10)

        # 2. Painel de Configuração (Meio)
        frame_cfg = tk.LabelFrame(self.root, text="Configuração do Tratamento", padx=10, pady=10)
        frame_cfg.pack(fill="x", padx=10, pady=5)

        tk.Label(frame_cfg, text="Volume Total (ml):").grid(row=0, column=0)
        tk.Entry(frame_cfg, textvariable=self.entry_vol, width=10).grid(row=0, column=1, padx=5)

        tk.Label(frame_cfg, text="Vazão (ml/h):").grid(row=0, column=2)
        tk.Entry(frame_cfg, textvariable=self.entry_rate, width=10).grid(row=0, column=3, padx=5)

        tk.Label(frame_cfg, text="Modo:").grid(row=0, column=4)
        modes = ["Contínuo", "Intermitente"] # Bolus e Purge são ações diretas
        ttk.Combobox(frame_cfg, textvariable=self.combo_mode, values=modes, width=10, state="readonly").grid(row=0, column=5, padx=5)

        btn_send = tk.Button(frame_cfg, text="ENVIAR CONFIG", bg="#ddd", command=self.send_config)
        btn_send.grid(row=0, column=6, padx=15)

        # 3. Painel de Controle (Botões Grandes)
        frame_ctrl = tk.Frame(self.root, pady=20)
        frame_ctrl.pack()

        # Botões de Ação
        btn_style = {"font": ("Arial", 10, "bold"), "width": 12, "height": 2}
        
        tk.Button(frame_ctrl, text="START", bg="#4CAF50", fg="white", command=lambda: self.send_action("start"), **btn_style).grid(row=0, column=0, padx=5)
        tk.Button(frame_ctrl, text="PAUSE", bg="#FFC107", command=lambda: self.send_action("pause"), **btn_style).grid(row=0, column=1, padx=5)
        tk.Button(frame_ctrl, text="STOP", bg="#F44336", fg="white", command=lambda: self.send_action("stop"), **btn_style).grid(row=0, column=2, padx=5)
        
        # Botões Especiais
        frame_spec = tk.Frame(self.root)
        frame_spec.pack(pady=10)
        
        tk.Button(frame_spec, text="PURGAR (CEBAR)", bg="orange", command=lambda: self.send_action("purge"), width=20).pack(side="left", padx=10)
        tk.Button(frame_spec, text="BOLUS (Dose Rápida)", bg="lightblue", command=self.popup_bolus, width=20).pack(side="left", padx=10)

    # --- LÓGICA MQTT ---
    def mqtt_connect(self):
        try:
            self.client.connect(BROKER_IP, BROKER_PORT, 60)
            self.client.loop_forever()
        except Exception as e:
            messagebox.showerror("Erro de Conexão", f"Não foi possível conectar na RPi ({BROKER_IP}):\n{e}")

    def on_connect(self, client, userdata, flags, rc):
        print(f"Conectado ao Broker! Código: {rc}")
        client.subscribe(TOPIC_STATUS)

    def on_message(self, client, userdata, msg):
        try:
            payload = json.loads(msg.payload.decode())
            # Atualiza GUI na thread principal
            self.root.after(0, self.update_gui, payload)
        except Exception as e:
            print(f"Erro JSON: {e}")

    def update_gui(self, data):
        # Traduz estado numérico para texto
        states = {0: "Inativo", 1: "Parado/Pausado", 2: "INFUNDINDO", 3: "Bolus", 4: "Purge", 5: "FIM", 99: "ERRO"}
        state_txt = states.get(data.get("state", 0), "Desconhecido")
        
        self.var_status.set(state_txt)
        self.var_vol_infused.set(f"{data.get('volume_infused', 0)} ml")
        self.var_flow_curr.set(f"{data.get('flow_rate', 0)} ml/h")
        self.var_pressure.set(f"{data.get('pressure', 0)}")
        
        is_alarm = data.get("alarm", False)
        self.var_alarm.set("PERIGO!" if is_alarm else "Normal")
        self.lbl_alarm.config(fg="red" if is_alarm else "green")

    # --- ENVIO DE COMANDOS ---
    def send_json(self, payload):
        try:
            msg = json.dumps(payload)
            self.client.publish(TOPIC_CMD, msg)
            print(f"Enviado: {msg}")
        except Exception as e:
            messagebox.showerror("Erro", f"Falha ao enviar MQTT: {e}")

    def send_config(self):
        # Mapeia modo texto para ID
        mode_map = {"Contínuo": 0, "Intermitente": 1}
        mode_val = mode_map.get(self.combo_mode.get(), 0)
        
        payload = {
            "action": "config",
            "volume": int(self.entry_vol.get()),
            "rate": int(self.entry_rate.get()),
            "mode": mode_val
        }
        self.send_json(payload)

    def send_action(self, action_name):
        self.send_json({"action": action_name})

    def popup_bolus(self):
        # Janela simples para confirmar Bolus
        win = tk.Toplevel(self.root)
        win.title("Configurar Bolus")
        win.geometry("300x150")
        
        tk.Label(win, text="Volume Bolus (ml):").pack(pady=5)
        v = tk.Entry(win)
        v.insert(0, "5")
        v.pack()
        
        tk.Label(win, text="Vazão (ml/h):").pack(pady=5)
        r = tk.Entry(win)
        r.insert(0, "600")
        r.pack()
        
        def confirm():
            payload = {
                "action": "bolus",
                "volume": int(v.get()),
                "rate": int(r.get())
            }
            self.send_json(payload)
            win.destroy()
            
        tk.Button(win, text="INJETAR AGORA", bg="red", fg="white", command=confirm).pack(pady=10)

if __name__ == "__main__":
    root = tk.Tk()
    app = InfusionPumpGUI(root)
    root.mainloop()
