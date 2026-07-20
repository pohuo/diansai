"""MaixCAM Pro test 03: send simulated coordinates on UART1.

Wiring for cross-board test:
  MaixCAM A19/UART1_TX -> MSPM0 A9/UART1_RX
  MaixCAM A18/UART1_RX <- MSPM0 A8/UART1_TX
  MaixCAM GND          -- MSPM0 GND
Both boards are powered separately. Do not connect 3V3/VBUS/5V.
"""

from maix import app, err, pinmap, time, uart


UART_DEVICE = "/dev/ttyS1"
BAUDRATE = 115200

# x, y, valid; 320 x 240 test positions
TEST_POINTS = [
    (160, 120, 1),
    (20, 120, 1),
    (300, 120, 1),
    (160, 20, 1),
    (160, 220, 1),
    (0, 0, 0),
]


def checksum(body):
    return sum(body.encode("ascii")) % 255


def make_frame(seq, x, y, valid):
    # Human-readable diagnostic protocol: $V,seq,valid,x,y,checksum*\n
    body = "V,{},{},{},{}".format(seq, valid, x, y)
    return "${},{}*\n".format(body, checksum(body))


err.check_raise(pinmap.set_pin_function("A19", "UART1_TX"), "set A19 UART1_TX failed")
err.check_raise(pinmap.set_pin_function("A18", "UART1_RX"), "set A18 UART1_RX failed")
serial = uart.UART(UART_DEVICE, BAUDRATE)

print("[TEST03] UART1 fixed-coordinate transmitter started")
print("[TEST03] device={}, baud={}".format(UART_DEVICE, BAUDRATE))

seq = 0
index = 0
while not app.need_exit():
    x, y, valid = TEST_POINTS[index]
    frame = make_frame(seq, x, y, valid)
    serial.write_str(frame)
    print("TX", frame.strip())

    seq = (seq + 1) % 256
    index = (index + 1) % len(TEST_POINTS)
    time.sleep_ms(500)

print("[TEST03] stopped normally")
