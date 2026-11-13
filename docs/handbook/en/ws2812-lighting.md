# The 30-pixel status and fill-light strip

The installed strip is **WS2813, 30 pixels**, connected at RGB/J7, D13/GPIO40 through the Robot Board's buffer. The URL retains an older WS2812 name for compatibility; it is not a statement that every strip can use the same driver or supply arrangement.

## What the colours mean

- **Green:** XIAO network readiness and fresh Indicator updates. The phone and printer connect on demand and do not need to remain continuously linked.
- **White:** explicit fill-light mode, including a touch request from the screen.
- **Waiting indication:** the status layer indicates incomplete readiness according to the device state.
- **Effects:** supported manual effects provide atmosphere rather than sensor truth.

Turning fill light off may reveal the green status layer; it does not necessarily make every LED dark. Inspect the reported `display_mode` when diagnosing this distinction. The prototype's brightness ceiling is 24/255; this is not a full-power current rating or proof of horticultural lighting performance.

## Automatic light

Indicator Settings → Light settings provides on/off thresholds, adjustment controls and a “capture current value” action. Save both thresholds and the enabled flag. XIAO performs the decision independently; manual actions take priority. Separate thresholds provide hysteresis so a noisy sample does not repeatedly toggle the light around one boundary.

The light sensor measures an ADC signal, not calibrated lux. Establish thresholds in the actual desk position and confirm the direction by covering/illuminating the sensor. Changing sensor orientation or plant placement can require new thresholds.

The user physically confirmed **30 green → 30 white → 30 green** and working touch controls. Saved automatic threshold controls have also been checked. See [wiring](hardware-wiring.md) for power and [API](system-api.md) for voice/control commands.

