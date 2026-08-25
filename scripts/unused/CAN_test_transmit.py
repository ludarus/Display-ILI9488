import can
import time

bus = can.interface.Bus(channel="can0", interface="socketcan")

# brightness fade
for i in range(10):
    msg = can.Message(arbitration_id=0x448, data=[2], is_extended_id=False)
    print(f"sending message {i} now")
    bus.send(msg)
    time.sleep(10 / 1000)

bus.shutdown()
