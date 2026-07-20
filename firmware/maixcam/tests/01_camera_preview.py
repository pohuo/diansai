"""MaixCAM Pro test 01: camera and display smoke test."""

from maix import app, camera, display, image


WIDTH = 320
HEIGHT = 240


cam = camera.Camera(WIDTH, HEIGHT)
disp = display.Display()

print("[TEST01] camera preview started")
print("[TEST01] expected: live image remains stable for 10 minutes")

while not app.need_exit():
    img = cam.read()
    img.draw_string(4, 4, "TEST01 CAMERA OK", image.COLOR_GREEN)
    disp.show(img)

print("[TEST01] stopped normally")
