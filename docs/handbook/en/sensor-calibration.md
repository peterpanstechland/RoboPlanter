# Calibrate soil and tank readings

Calibration belongs to XIAO's nonvolatile storage. The screen edits the reference points; YOYO reads the converted result. Rebooting should not require entering the same calibration again.

## Soil: two real reference points

1. Open Settings → Planter connection → Sensor calibration on the Indicator.
2. Put the probe in the actual substrate's agreed dry-reference condition. Wait for stable samples and capture **dry**.
3. Establish the wet-reference condition for that substrate, let readings settle, then capture **wet**.
4. Check the displayed values and save. Reboot, then verify that the reference points persist.

Never copy reference numbers from an article. The scale is relative wet/dry moisture, **not laboratory volumetric water content** and not a universal watering recommendation. Probe placement, substrate and supply voltage affect readings.

The phone's normal soil query returns a result such as `Soil:65% calibrated`. `uncalibrated` and `unavailable` are distinct states. A valid 0% reading must not be confused with missing data. Raw ADC remains available explicitly for diagnostics.

## Tank: full, low-water stop and empty

Mount the dry ultrasonic head above the water, away from wall echoes, splashes and condensation. Measure distances from the actual installed sensing face:

- **Full:** normal highest water surface.
- **Stop:** the lowest safe operating water surface for the pump intake.
- **Empty:** the chosen empty-tank reference surface.

The supported relationship is `20 <= full < stop < empty <= 3500` in millimetres, with at least 10 mm between adjacent points. This validation does not establish that your tank geometry produces reliable echoes. The calculated percentage describes relative water height, not millilitres.

After saving, move a target and test the installed water levels. Stale/invalid distance, uncalibrated tank and low water prevent normal watering; invalid water data during an action must stop it. Moving the probe or changing the tank requires rechecking calibration.

## Concurrent edits

The HTTP calibration API uses a revision to reject stale writes. If saving times out, read the state back before trying another write. A lost response is not evidence that the device did not save. See the [API guide](system-api.md).

