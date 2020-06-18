import pyb
import time
pin = pyb.Pin(2, pyb.Pin.OUT)
for i in range(4):
    print('LED ON2')
    pin.value(0)
    time.sleep(1)
    print('LED OFF2')
    pin.value(1)
    time.sleep(1)
    print('2iteration done.')
print("All 2 done.")
