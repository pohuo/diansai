// Parametric front mount for an 8-channel gray sensor board.
// First version based on board size: 90.3 x 37.0 mm.
// Print in PETG or ABS, 0.4 mm nozzle, 4 perimeters, 30% infill.

$fn = 48;

// Board data from the image
board_len = 90.3;
board_w   = 37.0;
board_t   = 1.6;

// Clearances
clr = 0.6;
wall = 3.0;

// Frame sizes
frame_len = 112;
frame_w   = board_w + 2 * wall + 6;
frame_h   = 18;
top_t     = 4;

// Chassis mounting slots
slot_len = 16;
slot_w   = 3.6;
slot_x1  = 26;
slot_x2  = 78;

// Board retention slots for zip ties
tie_slot_len = 10;
tie_slot_w   = 3.8;
tie_z1 = 7;
tie_z2 = 27;
tie_z3 = 53;
tie_z4 = 79;

module oblong_cut(len, wid, h) {
    linear_extrude(height = h)
        hull() {
            translate([-len / 2, 0]) circle(d = wid);
            translate([ len / 2, 0]) circle(d = wid);
        }
}

module slot_at(x, y, z, len, wid, h, rot = 0) {
    translate([x, y, z])
        rotate([0, 0, rot])
            oblong_cut(len, wid, h);
}

module rail(x, y, h, t, l) {
    translate([x, y, top_t])
        cube([l, t, h]);
}

module sensor_mount() {
    difference() {
        union() {
            // rear chassis plate
            cube([24, frame_w, top_t]);

            // left and right side rails, leaving the bottom open
            rail(8, wall, frame_h - top_t, wall, frame_len - 16);
            rail(8, frame_w - 2 * wall, frame_h - top_t, wall, frame_len - 16);

            // front and rear stops for the board
            translate([8, wall, top_t])
                cube([4, frame_w - 2 * wall, 7]);
            translate([frame_len - 12, wall, top_t])
                cube([4, frame_w - 2 * wall, 7]);
        }

        // chassis slots
        slot_at(slot_x1, frame_w / 2, -0.1, slot_len, slot_w, top_t + 0.2);
        slot_at(slot_x2, frame_w / 2, -0.1, slot_len, slot_w, top_t + 0.2);

        // board tie slots, through both side rails
        slot_at(tie_z1, wall + wall / 2, top_t + 3, tie_slot_len, tie_slot_w, frame_h, 0);
        slot_at(tie_z1, frame_w - wall - wall / 2, top_t + 3, tie_slot_len, tie_slot_w, frame_h, 0);
        slot_at(tie_z2, wall + wall / 2, top_t + 3, tie_slot_len, tie_slot_w, frame_h, 0);
        slot_at(tie_z2, frame_w - wall - wall / 2, top_t + 3, tie_slot_len, tie_slot_w, frame_h, 0);
        slot_at(tie_z3, wall + wall / 2, top_t + 3, tie_slot_len, tie_slot_w, frame_h, 0);
        slot_at(tie_z3, frame_w - wall - wall / 2, top_t + 3, tie_slot_len, tie_slot_w, frame_h, 0);
        slot_at(tie_z4, wall + wall / 2, top_t + 3, tie_slot_len, tie_slot_w, frame_h, 0);
        slot_at(tie_z4, frame_w - wall - wall / 2, top_t + 3, tie_slot_len, tie_slot_w, frame_h, 0);

    }
}

sensor_mount();
