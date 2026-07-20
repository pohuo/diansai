"""MaixCAM Pro test 04: UART1 local loopback.

Power off first, then connect MaixCAM A19 (TX) directly to A18 (RX)
with one jumper wire. No MSPM0 is used in this test.
"""

from maix import app, err, pinmap, time, uart


UART_DEVICE = "/dev/ttyS1"
BAUDRATE = 115200

err.check_raise(pinmap.set_pin_function("A19", "UART1_TX"), "set A19 UART1_TX failed")
err.check_raise(pinmap.set_pin_function("A18", "UART1_RX"), "set A18 UART1_RX failed")
serial = uart.UART(UART_DEVICE, BAUDRATE)

print("[TEST04] connect A19(TX) to A18(RX)")

seq = 0
passed = 0
failed = 0

while not app.need_exit():
    message = "LOOPBACK,{:03d}\n".format(seq)

    # Clear any old bytes before sending a new test message.
    serial.read()
    serial.write_str(message)
    time.sleep_ms(100)
    received = serial.read()

    if received == message.encode("ascii"):
        passed += 1
        print("PASS seq={} total_pass={} total_fail={}".format(seq, passed, failed))
    else:
        failed += 1
        print("FAIL seq={} expected={!r} received={!r}".format(seq, message, received))

    seq = (seq + 1) % 1000
    time.sleep_ms(400)

print("[TEST04] stopped: pass={}, fail={}".format(passed, failed))
