

P1.0 - Display Segment A      (open-drain)
P1.1 - Display Segment B      (open-drain)
P1.2 - Display Segment C      (open-drain)
P1.3 - Display Segment D      (open-drain)
P1.4 - Display Segment E      (open-drain)
P1.5 - Display Segment F      (open-drain)
P1.6 - Display Segment G      (open-drain)
P1.7 - Display Decimal Point  (open-drain)

P3.0 - Mosfet Gate Control (open-drain)
P3.1 - Display Digit 1 (push-pull)
P3.2 - ADC Input Voltage Measurement (Divider 200k/39k)
P3.3 - Display Digit 2 (push-pull)
P3.4 - Display Digit 3 (push-pull)
P3.5 - Buzzer (open-drain)
P3.6 - Button 1 (internal pull-up)
P3.7 - Button 2 (internal pull-up)

P5.4 - Pedal Switch / Electrode Contact Detection (pull-up disbled)
P5.5 - +Vref (ADC Reference Voltage)

When pedal is not connected to jack, P5.4 detects short circuit between welding electrode and ground. When pedal is connected to jack, P5.4 detects pedal switch pressed or not.

P5.5 provides reference voltage for ADC measurement of input voltage.


Button functions:
- Button 1 short press: decrease pulse width by 1 point
- Button 1 long press: select trigger mode:
    - Mode A-0: Manual
    - Mode A-1: Auto
- Button 2 short press: increase pulse width by 1 point
- Button 2 long press: select pulse count:
    - Step P-1: 1 pulse
    - Step P-2: 2 pulses
    - Step P-3: 3 pulses
- Button 2 wery long press: go to sleep mode (display off, all outputs off, wake up by button press or pedal press)
