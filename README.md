# Word-Clock

## An arduino word clock implementation for a gift

### Modes

- OFF: Turns off all LEDs
- WORD_MODE: Displays the time as a phrase, e.g., "It's twenty til five"
  - Accurate to within a 2 minute interval
  - With word mode on, if the date is July 4th, birthday mode activates
- DIGIT_MODE: Displays the time as it would appear on a digital clock
- CYCLE_MODE: Alternates between WORD_MODE and DIGIT_MODE every ten seconds
- ADJUST_COLOR: Brings up a color wheel to select from 28 colors
- ADJUST_BRIGHTNESS: Brings up a star that changes size depending upon the brightness selected
- ADJUST_HOUR: Allows user to adjust current hour in digit format
- ADJUST_MINUTE: Allows the user to adjust the current minute in digit format
- ADJUST_MONTH: Allows user to adjust the current month in digit format
- ADJUST_DAY: Allows user to adjust the current day in digit format
- ADJUST_YEAR: Allows user to adjust the current year in digit format

### Usage

- The mode is selected using pullup buttons on the side of the clock
  - Select button to cycle through modes
  - Up and down buttons to adjust mode when entered



