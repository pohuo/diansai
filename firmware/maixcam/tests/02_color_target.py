"""MaixCAM Pro test 02: detect the largest colored target.

Default threshold is the red LAB example from the MaixPy v4 documentation.
Tune RED_THRESHOLD on the real screen/printed target when necessary.
"""

from maix import app, camera, display, image


WIDTH = 320
HEIGHT = 240
RED_THRESHOLD = [0, 80, 40, 80, 10, 80]
PIXELS_THRESHOLD = 300
AREA_THRESHOLD = 300


def largest_blob(blobs):
    if not blobs:
        return None
    return max(blobs, key=lambda b: b[2] * b[3])


cam = camera.Camera(WIDTH, HEIGHT)
disp = display.Display()

print("[TEST02] largest red target detection started")
print("[TEST02] threshold:", RED_THRESHOLD)

while not app.need_exit():
    img = cam.read()
    blobs = img.find_blobs(
        [RED_THRESHOLD],
        pixels_threshold=PIXELS_THRESHOLD,
        area_threshold=AREA_THRESHOLD,
    )
    target = largest_blob(blobs)

    if target is None:
        img.draw_string(4, 4, "TARGET LOST", image.COLOR_RED)
        print("TARGET_LOST")
    else:
        x, y, w, h = target[0], target[1], target[2], target[3]
        cx = x + w // 2
        cy = y + h // 2
        area = w * h
        img.draw_rect(x, y, w, h, image.COLOR_GREEN)
        img.draw_string(4, 4, "X:{} Y:{} A:{}".format(cx, cy, area), image.COLOR_GREEN)
        print("TARGET x={} y={} area={}".format(cx, cy, area))

    disp.show(img)

print("[TEST02] stopped normally")
