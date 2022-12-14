# Drumatic Affect

The circuit and program described in this project are used to add features to the 
[Digitech SDRUM](https://www.digitech.com/band-creator/SDRUM.html)
pedal, which is a drum machine that has a lot of cool features but seems to be missing some
key functionality for my purposes. The main thing that it is missing is a way to specify
a specific beats per minute. The use cases for the design of the pedal seem to be to use tap
tempo to set the tempo, or to set the tempo when a song is programmed.

I intend to use the pedal to drive the tempo of the song, but I don't program the song slots
with specific songs. Each song slot merely has various beats programmed that can be used in 
various songs. I simply select the set of verse / chorus / bridge beats that work the best for
a song, then set the proper tempo for the song, and start it. I want to be able to set a 
specific tempo that I've determined ahead of time so that the SDRUM unit can keep that exact
tempo rather than setting one on-the-fly with the tap tempo.

Unfortunately, this is the one thing that the SDRUM unit can't do. There is no way to know or set
the exact tempo. You can get in-the-ballpark by using a metronome to set a tempo then trying to
tap that tempo, but it's somewhat in-exact.

The idea came about to have a hardware device send the tap signal rather than a person. This would
give a very exact tempo that could be set simply be dialing in the correct tempo on the device
and having it send emulated taps to the footswitch input. That device is the **Drumatic Affect**.
I came across [another project](https://www.instructables.com/Arduino-Metronome/) that had some 
overlap, started with that and continued to modify it until it fit the new purpose.

## Requirements

I had a pretty long list of requirements for a drum machine that would fit my needs. I tried out
many different solutions and nothing existed that could do everything I wanted. Here is a 
comparison table of many possible solutions.

| Feature                               | SDRUM + Drumatic Affect | SDRUM | Rock Drummer App | Beat Buddy | Alesis SR-16     | Drum Tracks |
|---------------------------------------|-------------------------|-------|------------------|------------|------------------|-------------|
| Battery powered                       | yes*                    | yes*  | yes              | yes*       | yes*             | yes         |
| Song selector in arm's reach          | yes (with stand)        | no*   | yes (with stand) | no         | yes (with stand) | yes         |
| Able to set specific tempo            | yes                     | no    | yes              | yes        | yes              | yes         |
| Direct access to song parts           | yes                     | yes*  | yes/no           | no         | yes (only 2)     | no          |
| Start and stop with footswitch        | yes                     | no*   | yes              | yes        | yes              | yes         |
| Fills                                 | yes                     | yes   | yes              | yes        | yes              | no          |
| Easily programmable                   | yes                     | yes   | no               | yes        | no               | no          |
| Programs beat patterns not just songs | yes                     | yes   | yes              | no         | yes              | no          |
| Fast setup at gigs                    | yes                     | yes   | no               | yes        | yes              | yes         |
| Count-in                              | yes                     | yes   | yes              | yes        | no               | yes         |
| Mute on-demand                        | yes                     | yes   | no               | no         | no               | no          |

Some of the requirements are marked with exceptions. Such as the battery powered requirement can be filled 
with a 9v guitar effect pedal battery pack on most devices. The SDRUM does have many of the requirements, 
but some can not be filled at the same time depending on which footswitch mode you use.

## Schematic

The schematic for the Drumatic Affect device is shown below.

![Schematic diagram](DrumaticAffect_bb.png)

This circuit includes two relays driven by an Arduino Nano and the code in this repository.
The first relay is used to send the tap signal to the FSX3 footswitch input. A 2P3T switch can
be used to select which song part to select. This requires the pedal to be in **Direct Part**
mode which allows you to select the verse, chorus, and bridge directly using each of the three
buttons on the FSX3 switch. You can also select the chorus pole of the switch and run the 
pedal in the **Table Top** mode.

The second relay is trickier to use because it requires modifying the pedal (and voiding the
warranty). It is connected to the footswitch on the pedal itself. This requires you to open
the pedal and add leads to the two solder points on the footswitch sub-board and run those
leads to the second relay in this circuit. This relay is used to auto-start a song as soon
as the tempo is set, so it is optional if you can start the song manually after setting the
tempo.

The rest of the circuit is fairly straightforward. There is a rotary encoder with a push button 
switch (shown as two separate components in this diagram). The rotary encoder is used to select
the tempo, then pushing the switch sends three "taps" to the FSX3 input to select the song
part and set the tempo. The second relay then sends a signal to the pedal to start the song.

I did find that cheap rotary encoders are terribly noisy and you'll get stray signals on both
the rotary switch as well as the push button switch; and even worse, you'll get stray signals
on the push button switch while using the rotary switch. I ended up buying [rotary encoders
with built-in debouncing](https://www.tindie.com/products/fabteck/24-steps-rotary-encoder-and-debouncing-circuit/)
(as well as using debouncing in the software).

### Parts List

* Arduino Nano
* Rotary encoder with push-button switch
* 128X64 OLED LCD Display
* 2 - 5V relays
* 2 - 1N4001 diodes
* 2P3T rotary switch
* SPST toggle switch

## What's up with the name?

The word "drumatic" is a play on "automatic drum". The word "affect" is used because it's 
effect pedal, but this device causes the change, it doesn't respond to the change, so "affect"
seems like a better description of it's behavior than "effect". And, of course, the whole
name is a play on the phrase "dramatic effect".
