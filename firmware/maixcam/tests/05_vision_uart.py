"""MaixCAM Pro test 05: colored-target detection + UART1 output."""

from maix import app, camera, display, err, image, pinmap, time, uart


WIDTH = 320
HEIGHT = 240
RED_THRESHOLD = [0, 80, 40, 80, 10, 80]
PIXELS_THRESHOLD = 300
AREA_THRESHOLD = 300
UART_DEVICE = "/dev/ttyS1"
BAUDRATE = 115200
SEND_INTERVAL_MS = 50  # 20 Hz


def largest_blob(blobs):
    if not blobs:
        return None
    return max(blobs, key=lambda b: b[2] * b[3])


def checksum(body):
    return sum(body.encode("ascii")) % 255


def make_frame(seq, x, y, valid):
    body = "V,{},{},{},{}".format(seq, valid, x, y)
    return "${},{}*\n".format(body, checksum(body))


err.check_raise(pinmap.set_pin_function("A19", "UART1_TX"), "set A19 UART1_TX failed")
err.check_raise(pinmap.set_pin_function("A18", "UART1_RX"), "set A18 UART1_RX failed")
serial = uart.UART(UART_DEVICE, BAUDRATE)
cam = camera.Camera(WIDTH, HEIGHT)
disp = display.Display()

print("[TEST05] vision + UART started")

seq = 0
last_send_ms = time.ticks_ms()

while not app.need_exit():
    img = cam.read()
    blobs = img.find_blobs(
        [RED_THRESHOLD],
        pixels_threshold=PIXELS_THRESHOLD,
        area_threshold=AREA_THRESHOLD,
    )
    target = largest_blob(blobs)

    if target is None:
        x_out, y_out, valid = 0, 0, 0
        img.draw_string(4, 4, "TARGET LOST", image.COLOR_RED)
    else:
        x, y, w, h = target[0], target[1], target[2], target[3]
        x_out = x + w // 2
        y_out = y + h // 2
        valid = 1
        img.draw_rect(x, y, w, h, image.COLOR_GREEN)
        img.draw_string(4, 4, "X:{} Y:{}".format(x_out, y_out), image.COLOR_GREEN)

    now_ms = time.ticks_ms()
    if now_ms - last_send_ms >= SEND_INTERVAL_MS:
        frame = make_frame(seq, x_out, y_out, valid)
        serial.write_str(frame)
        print("TX", frame.strip())
        seq = (seq + 1) % 256
        last_send_ms = now_ms

    disp.show(img)

print("[TEST05] stopped normally")
