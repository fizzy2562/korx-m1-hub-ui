# Controls

HOME opens live help. LEFT/RIGHT switch its two pages; HOME or RETURN closes it without changing the edit position. Other controls dismiss help and retain their normal action. HOME held for one second and SHIFT+HOME remain Hub responsibilities.

All button actions use type 3, their ID, down=1, value=0 (release with down=0). All encoders use type 4, their ID, down=0, value=+1/-1. Changes are temporary: selecting another patch reloads its data; there is no save command.

| Control (ID) | Program Play | Combination Play | Every edit page |
|---|---|---|---|
| F1/F2 (0/1) | Previous/next available program | Previous/next available combination | Select previous/next patch in current mode |
| F3 (2) | Combination Play | Program Play | Switch mode, return to Play |
| F4 (3) | Compare original/edited | Same | Same; next edit exits compare |
| F5/F6 (4/5) | Enter previous/next edit group | Same | Previous/next edit group, wrapping |
| SHIFT (6) | Hold modifier | Same | E1-E4 parameter step x4 |
| OCT-/OCT+ (7/8) | Unused | Unused | Unused |
| ENTER (9) | Edit current group (initially Name) | Same | Return to Play |
| K1 (10) | Play | Play | Play and reset selected group to Name |
| K2 (11) | OSC Basic | Combi Type | Same mode-specific jump |
| K3 (12) | Effects common | Effects common | Effects common |
| K4/K5 (13/14) | FX1/FX2 parameters | Same | Same (blank if No Effect) |
| K6 (15) | Name | Timbre 1 | Same mode-specific jump |
| RETURN (16) | Play | Play | Return to Play; closes help when open |
| HOME (17) | Program help | Combination help | Current group/part help |
| LEFT/RIGHT (18/20) | Enter previous/next parameter part | Same | Previous/next 4 parameters; wraps across groups |
| MENU (19), REC/STOP/PLAY (21/22/23), DATA_PUSH (24) | Unused | Unused | Unused |
| E1-E4 (encoder 0-3) | Unused | Unused | Edit the named parameter in each of the four columns |
| A / DATA (encoder 4/6) | Previous/next available program | Previous/next available combination | Select patch in current mode; SHIFT does not skip four patches |
| B (encoder 5) | Switch to Combination | Switch to Program | Switch mode, return to Play |
| On-screen keys (type 2) | MIDI 48+ID; first key C3 (MIDI 48) | Same | Same |
| Pads (type 1) | Unused | Unused | Unused |
| Pedal (type 6) | Sustain | Same | Same |
| MIDI (type 5) | Existing engine MIDI handling | Same | Same |

Keyboard velocity comes from value (0.8 is about 102); down=0 releases the note. OCT buttons do not transpose. MIDI octave naming can differ between hosts; note number 48 is authoritative.

## Edit-page columns

The common control table above applies to every row below. Each semicolon separates a part of up to four parameters in E1-E4 order. F5/F6 traverse these groups in order; LEFT/RIGHT traverse their parts. OSC2 groups appear only with Osc Mode=Double. Play is separate from the Name edit group.

| Mode | Group | E1-E4 parameters by part |
|---|---|---|
| Program | Name | Char 1, Char 2, Char 3, Char 4; Char 5, Char 6, Char 7, Char 8; Char 9, Char 10 |
| Program | OSC Basic | Osc Mode, Assign, Hold |
| Program | OSC 1 | Multisnd, Octave, Level |
| Program | OSC 2 | Multisnd, Octave, Level, Interval; Detune, Delay |
| Program | Pitch EG 1 | Start Lvl, Atk Time, Atk Level, Dcy Time; Rel Time, Rel Level, Time Vel, Level Vel |
| Program | Pitch EG 2 | Start Lvl, Atk Time, Atk Level, Dcy Time; Rel Time, Rel Level, Time Vel, Level Vel |
| Program | VDF 1 | Cutoff, EG Int, Kbd Center, Kbd Track; EG Int Vel, EG Tm Kbd, EG Tm Vel |
| Program | VDF 1 EG | Atk Time, Atk Level, Dcy Time, Break Pt; Slope Tm, Sustain, Rel Time, Rel Level |
| Program | VDF 1 Sw | Kbd Atk, Kbd Dcy, Kbd Slope, Kbd Rel; Vel Atk, Vel Dcy, Vel Slope, Vel Rel |
| Program | VDF 2 | Cutoff, EG Int, Kbd Center, Kbd Track; EG Int Vel, EG Tm Kbd, EG Tm Vel |
| Program | VDF 2 EG | Atk Time, Atk Level, Dcy Time, Break Pt; Slope Tm, Sustain, Rel Time, Rel Level |
| Program | VDF 2 Sw | Kbd Atk, Kbd Dcy, Kbd Slope, Kbd Rel; Vel Atk, Vel Dcy, Vel Slope, Vel Rel |
| Program | VDA 1 | Kbd Center, Kbd Track, Vel Sense, EG Tm Kbd; EG Tm Vel |
| Program | VDA 1 EG | Atk Time, Atk Level, Dcy Time, Break Pt; Slope Tm, Sustain, Rel Time |
| Program | VDA 1 Sw | Kbd Atk, Kbd Dcy, Kbd Slope, Kbd Rel; Vel Atk, Vel Dcy, Vel Slope, Vel Rel |
| Program | VDA 2 | Kbd Center, Kbd Track, Vel Sense, EG Tm Kbd; EG Tm Vel |
| Program | VDA 2 EG | Atk Time, Atk Level, Dcy Time, Break Pt; Slope Tm, Sustain, Rel Time |
| Program | VDA 2 Sw | Kbd Atk, Kbd Dcy, Kbd Slope, Kbd Rel; Vel Atk, Vel Dcy, Vel Slope, Vel Rel |
| Program | Pitch MG | Wave, Frequency, Delay, Intensity; OSC1, OSC2, Key Sync |
| Program | VDF MG | Wave, Frequency, Delay, Intensity; OSC1, OSC2, Key Sync |
| Program | AfterTouch | Pitch, Pitch MG, VDF Cutoff, VDF MG; VDA Amp |
| Program | Joystick | Bend, VDF Sweep, Pitch MG, PMG Freq; VDF MG, VMG Freq |
| Combination | Name | Char 1, Char 2, Char 3, Char 4; Char 5, Char 6, Char 7, Char 8; Char 9, Char 10 |
| Combination | Combi Type | Type |
| Combination | Timbre 1 | Program, Level, Transpose, Detune; Pan, Key Top, Key Bottom, Vel Top; Vel Bottom, MIDI Ch, Timbre, Prg Chg; Damper, AfterTch, Ctrl Chg |
| Combination | Timbre 2 | Program, Level, Transpose, Detune; Pan, Key Top, Key Bottom, Vel Top; Vel Bottom, MIDI Ch, Timbre, Prg Chg; Damper, AfterTch, Ctrl Chg |
| Combination | Timbre 3 | Program, Level, Transpose, Detune; Pan, Key Top, Key Bottom, Vel Top; Vel Bottom, MIDI Ch, Timbre, Prg Chg; Damper, AfterTch, Ctrl Chg |
| Combination | Timbre 4 | Program, Level, Transpose, Detune; Pan, Key Top, Key Bottom, Vel Top; Vel Bottom, MIDI Ch, Timbre, Prg Chg; Damper, AfterTch, Ctrl Chg |
| Combination | Timbre 5 | Program, Level, Transpose, Detune; Pan, Key Top, Key Bottom, Vel Top; Vel Bottom, MIDI Ch, Timbre, Prg Chg; Damper, AfterTch, Ctrl Chg |
| Combination | Timbre 6 | Program, Level, Transpose, Detune; Pan, Key Top, Key Bottom, Vel Top; Vel Bottom, MIDI Ch, Timbre, Prg Chg; Damper, AfterTch, Ctrl Chg |
| Combination | Timbre 7 | Program, Level, Transpose, Detune; Pan, Key Top, Key Bottom, Vel Top; Vel Bottom, MIDI Ch, Timbre, Prg Chg; Damper, AfterTch, Ctrl Chg |
| Combination | Timbre 8 | Program, Level, Transpose, Detune; Pan, Key Top, Key Bottom, Vel Top; Vel Bottom, MIDI Ch, Timbre, Prg Chg; Damper, AfterTch, Ctrl Chg |

## Effects in both modes

| Destination | Program event sequence | Combination event sequence |
|---|---|---|
| Effects common part 1 | K3: `e:3,12,1,0` | F3 from Program: `e:3,2,1,0`, then K3 |
| Common part 2 | K3, RIGHT `e:3,20,1,0` | Same after selecting Combination |
| Common part 3 | K3, RIGHT twice | Same |
| Common part 4 | K3, RIGHT three times | Same |
| FX1 parameters part 1 | K4: `e:3,13,1,0` | Same |
| FX1 parameters part 2 (if more than 4) | K4, RIGHT | Same |
| FX2 parameters part 1 | K5: `e:3,14,1,0` | Same |
| FX2 parameters part 2 (if more than 4) | K5, RIGHT | Same |

Button releases use the same type/id with down=0, value=0. Encoders use type=4, down=0, value=+1 or -1. On common part 1, E1 (id 0) selects FX1 and E2 (id 1) selects FX2. Types 0..32 are effects; 33 is No Effect (no parameter page). K4/K5 are blank when their effect is No Effect. F5/F6 also traverse every available group; LEFT/RIGHT traverse groups of four parameters and cross group boundaries.

| Common part | E1 | E2 | E3 | E4 |
|---|---|---|---|---|
| 1 | FX1 Type | FX2 Type | Placement | FX1 L On |
| 2 | FX1 R On | FX2 L On | FX2 R On | FX1 Bal L |
| 3 | FX1 Bal R | FX2 Bal L | FX2 Bal R | Out3 Pan |
| 4 | Out4 Pan | unused | unused | unused |

See [EFFECTS_MAP.md](EFFECTS_MAP.md) for every effect parameter.
