# A small ticket for an ordinary day

The thermal printer is an output medium, not merely a debugging peripheral. A ticket gives a fleeting desk interaction a physical ending: something to write on, fold or keep.

## The layout

1. **Plant-computer logo and scene title.** A recognisable, one-colour identity suited to thermal printing.
2. **Current plant state.** Species and valid readings; unavailable data is stated honestly.
3. **Three small goals.** Empty checkboxes and ruled lines for handwriting.
4. **A tarot reflection.** A random card from the complete 78-card deck, with upright or reversed orientation.
5. **A brief closing line.** A reminder to care for the plant and take a break yourself.

The full deck includes 22 Major Arcana and 56 Minor Arcana. The random draw is playful, not a forecast, medical judgement or decision system.

## One preview, one ticket

The Indicator splits the full ticket into seven preview pages. Opening a new preview can draw a new card. Once a print request is created, its text, orientation, card and state snapshot are fixed. Reopening an action result must not quietly draw another card.

Paper layout uses a 384-dot bitmap. The tested EM5826H requires the project's **128-row raster-band mode** to preserve proportions; a historical 32-row attempt flattened circles and text. This behavior was established with a printed square/circle comparison, not inferred from the ESC/POS command name alone.

![Printed A/B raster-band comparison from the prototype bench](/roboplanter/assets/printer-aspect-bench.jpg)

*Historical diagnostic print: A was flattened; B preserved the intended aspect ratio. This is a bench photo, not a finished enclosure photograph.*

## From screen to paper

Indicator sends the confirmed ticket request to XIAO over authenticated HTTP. XIAO formats and sends ESC/POS data to the printer over BLE, then sends the configured cut sequence. Printer USB and TTL are disconnected in this configuration. The observed printout matched the preview, had normal proportions and cut successfully.

“Bytes sent” is not a paper-presence or cutter-position sensor. A disconnected or busy printer needs an explicit result and recovery path, not repeated automatic prints. [Printer setup](uart-printer.md) and the [system API](system-api.md) explain the transport and request lifecycle.

Printr informed the idea of small, useful printed rituals. See the [original Printr project](https://xninety9.github.io/Printr/); RoboPlanter's plant, screen and BLE workflow is its own implementation.

