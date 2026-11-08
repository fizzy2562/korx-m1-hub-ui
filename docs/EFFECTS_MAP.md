# Effects map

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

## All 33 effects

Select a type on K3 using E1/E2, then K4/K5. Parameters below are compacted from `m1::effectParams(type)` (unused bytes omitted), in E1-E4 order. RIGHT reaches the remaining parameters.

| Type | Name | Part 1 (E1-E4) | Part 2 (E1-E4) |
|---|---|---|---|
| 0 | Hall | Rev Time, High Damp, Pre Delay, E/R Level | EQ High, EQ Low |
| 1 | Ens Hall | Rev Time, High Damp, Pre Delay, E/R Level | EQ High, EQ Low |
| 2 | ConcHall | Rev Time, High Damp, Pre Delay, E/R Level | EQ High, EQ Low |
| 3 | Room | Rev Time, High Damp, Pre Delay, E/R Level | EQ High, EQ Low |
| 4 | LargeRoom | Rev Time, High Damp, Pre Delay, E/R Level | EQ High, EQ Low |
| 5 | LiveStage | Rev Time, High Damp, Pre Delay, E/R Level | EQ High, EQ Low |
| 6 | E.Ref 1 | E/R Time, Pre Delay, EQ High, EQ Low | none |
| 7 | E.Ref 2 | E/R Time, Pre Delay, EQ High, EQ Low | none |
| 8 | E.Ref 3 | E/R Time, Pre Delay, EQ High, EQ Low | none |
| 9 | St Delay | Delay L, Feedback, High Damp, Delay R | EQ High, EQ Low |
| 10 | CrossDly | Delay L, Feedback, High Damp, Delay R | EQ High, EQ Low |
| 11 | Chorus 1 | Depth, Speed, Triangle, Delay | EQ High, EQ Low |
| 12 | Chorus 2 | Depth, Speed, Triangle, Delay | EQ High, EQ Low |
| 13 | Flanger 1 | Depth, Speed, MG Status, Feedback | Delay, EQ High, EQ Low |
| 14 | CrossFlng | Depth, Speed, MG Status, Feedback | Delay, EQ High, EQ Low |
| 15 | Phaser 1 | Depth, Speed, MG Status, Feedback | Manual |
| 16 | Phaser 2 | Depth, Speed, MG Status, Feedback | Manual |
| 17 | Tremolo 1 | Depth, Speed, MG Status, Shape | EQ High, EQ Low |
| 18 | Tremolo 2 | Depth, Speed, MG Status, Shape | EQ High, EQ Low |
| 19 | Equalizer | High fc, Low fc, High Gain, Low Gain | none |
| 20 | OverDrive | Drive, Level, EQ High, EQ Low | none |
| 21 | Distortion | Distortion, Level, EQ Low | none |
| 22 | Exciter | Blend, Emphasis, EQ High, EQ Low | none |
| 23 | Symph Ens | Depth, EQ High, EQ Low | none |
| 24 | Rotary SP | Depth, Speed Rate, Fast | none |
| 25 | Dly/Hall | Delay, Feedback, High Damp, Rev Time | Rev Damp, Pre Delay |
| 26 | Dly/Room | Delay, Feedback, High Damp, Rev Time | Rev Damp, Pre Delay |
| 27 | Dly/E.Ref | Delay, Feedback, High Damp, E/R Time | Pre Delay |
| 28 | Dly/Dly | Delay, Feedback, High Damp, Delay R | Feedback R, Damp R |
| 29 | Dly/Chorus | Delay, Feedback, High Damp, Depth | Speed, Triangle |
| 30 | Dly/Flngr | Delay, Feedback, High Damp, Depth | Speed, Triangle, Feedback |
| 31 | Dly/Phasr | Delay, Feedback, High Damp, Depth | Speed, Feedback, Manual |
| 32 | Dly/Treml | Delay, Feedback, High Damp, Depth | Speed, Shape |

To audition, choose an effect program (indices 8-18), enable the FX L/R switches and set nonzero balances on K3. Play notes, then compare changes. Placement selects serial/parallel routing. Out3/Out4 pan controls remain exposed; the Hub audio API supplies stereo output, so independent physical outputs 3/4 are not available. This change preserves the existing audio engine and effect implementations.
