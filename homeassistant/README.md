# Home Assistant: laundry room motion lights

`laundry_lights_motion.yaml` is a Home Assistant automation that turns the
laundry lights on when motion is detected and off after the room has been
clear for 5 minutes.

## Setup

1. Find your entity IDs in Home Assistant under **Settings -> Devices & services -> Entities**
   (search for "laundry"). You need one motion or occupancy `binary_sensor`
   and one `light` (a light group works too).
2. Open `laundry_lights_motion.yaml` and replace:
   - `binary_sensor.laundry_motion` with your motion sensor
   - `light.laundry_lights` with your light
3. In Home Assistant go to **Settings -> Automations & Scenes -> Create Automation**,
   open the three-dot menu, choose **Edit in YAML**, paste the file contents
   (everything below the comment block), and save.

## Tuning

- **Off delay**: change `minutes: 5` under the `room_clear` trigger. Most
  motion sensors have their own "clear" timeout (often 30 to 90 seconds),
  so the lights go off at sensor-clear time plus this delay.
- **Only when dark**: add a condition to the `motion_detected` branch, for
  example `condition: sun`, `after: sunset` / `before: sunrise`, or a
  `numeric_state` condition on an illuminance sensor.
- **Manual override**: if you want the lights to stay on when you switched
  them on by hand, add a condition to the `room_clear` branch that checks a
  helper toggle you set from the wall switch.

## Alternative: the built-in blueprint

Home Assistant ships a "Motion-activated Light" blueprint that does the same
thing with no YAML. Go to **Settings -> Automations & Scenes -> Blueprints ->
Motion-activated Light -> Create Automation**, pick the motion sensor and
light, and set the wait time. Use the YAML file here if you want to customise
the behaviour beyond what the blueprint offers.
