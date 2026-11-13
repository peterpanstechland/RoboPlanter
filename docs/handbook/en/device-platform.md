# Bring your own hardware with Device Kit

The existing XIAO and Indicator implementation is the hardware reference. Device Kit explores how another controller could describe its identity, measurements and supported operations without disguising every board as the original prototype.

The current kit is **offline**. It validates capability descriptions and contracts; it does not discover devices, create accounts, connect to a planter or send actuator commands.

## Try the contract checks

Download the [Device Kit ZIP](/roboplanter/downloads/roboplanter-device-kit.zip), or use the repository:

```sh
python integrations/device-kit/device_kit.py validate integrations/device-kit/profiles/*.json
python -m unittest discover -s tests -p test_device_kit.py
```

On a shell that does not expand the wildcard, pass individual profile files. Consult the [kit README](../../../integrations/device-kit/README.md) for the supported schema and exact options.

A new hardware adapter must establish its own pin/power mapping, authorization, units, valid/invalid state, action limits and result semantics. Passing a JSON validator is not evidence that a physical pump stops or that a mobile BLE stack is compatible.

MQTT/MCP experiments remain optional auxiliary integrations, not prerequisites for YOYO's local BLE route. A2A and an online device-management platform are not implemented. Use the [system API](system-api.md) as a concrete reference, then test the new adapter rather than copying the original device's private configuration.

