# OSC Catalog

OSC is a public Synaptome app-facing control surface. The first public repo documents and validates OSC messages that the openFrameworks runtime can receive or emit without requiring firmware, helper source, generated radio headers, or deployment netmaps.

## App Parameter Routes

The default app OSC map lives at:

```text
synaptome/bin/data/config/osc-map.json
```

Example route fixture:

```text
docs/examples/osc_map_example.json
```

Validate with:

```powershell
python tools\validate_configs.py --public-app
python tools\validate_parameter_targets.py --strict --contract-fixtures
```

## Sensor-Like App Inputs

Synaptome treats several incoming OSC paths as sensor or modulation sources. Public examples should target stable parameter IDs or documented sensor IDs.

Typical app-facing families:

```text
/parameter/<parameter-id>
/sensor/host/localmic/<metric>
/sensor/bioamp/<metric>
/control/<action>
```

The exact accepted routes are owned by `synaptome/src/io/OscParameterRouter.*` and the committed OSC map fixtures.

## Public Boundary

Included in first public Synaptome:

- app OSC map schema and examples
- parameter-target validation
- BrowserFlow OSC ingest regression
- host-local audio/sensor style routes when represented as app inputs

Not included in first public Synaptome:

- firmware TLV decode implementation
- helper ESP-NOW bridge source
- generated radio config headers
- private deployment netmaps
- embedded UI catalog exchange fixtures

Those belong to future helper or radio-contract packages. If a future package emits OSC into Synaptome, it should publish sample app-facing OSC captures that do not require private firmware artifacts.
