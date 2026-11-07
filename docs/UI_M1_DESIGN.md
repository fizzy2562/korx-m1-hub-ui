# KORX-M1 LCD design

The KORX screen is an original, device-oriented interpretation of the late-1980s workstation LCD: a pale blue-green phosphor field inside a dark bezel, a faint pixel grid, compact page tabs, inverse-video selection, and value bars. It uses no Korg artwork, ROM font, logo, or trademark. The on-screen brand is KORX.

The framebuffer is 480x320 RGB565. Text is a 1-bit, custom 5x7 bitmap font with an integer pixel scale and a one-pixel LCD dot gap. Small text uses scale 1; the program name uses scale 3. The four parameter slots remain in the Hub's four 115-pixel columns, and the six soft-key fields remain at y=282..319.

The selected parameter is an inverse-video cell. Page navigation is represented by a compact title strip and `group/nGroups`; values have horizontal slider bars. Play pages emphasize the bank/number and name, with voice LEDs and audio meters. Combination play adds eight live timbre rows from the model's 11-byte timbre records. VDF and EG pages add integer-only response/envelope sketches.

`KORX_DARK_MODE` in `src/m1ui_view.h` is the alternate dark LCD palette switch. It is off by default for the classic backlit look. Curves use integer line drawing; there is no floating-point drawing, heap allocation, STL, RTTI, or exception use in the view.

Future work: Write confirmation dialog, Global page layout, Disk, Sequencer, and numeric keypad entry.
