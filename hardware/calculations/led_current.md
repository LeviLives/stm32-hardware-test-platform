# LED Load Calculation

### LED

Manufacturer: DFRobot
Product: FIT0242 5 mm LED assortment
Color used: Red
Forward-voltage specification: 1.8–2.2 V
Maximum forward current: 20 mA
Suggested forward current: 16–18 mA
Polarity: long lead = anode; short lead/flat edge = cathode

## Expected Current

Supply voltage: 5.0 V
Series resistance: 330 ohm

At $V_F = 1.8\ \mathrm{V}$:

$$
I = \frac{V_S - V_F}{R}
  = \frac{5.0\ \mathrm{V} - 1.8\ \mathrm{V}}{330\ \Omega}
  = 9.70\ \mathrm{mA}
$$

At $V_F = 2.2\ \mathrm{V}$:

$$
I = \frac{5.0\ \mathrm{V} - 2.2\ \mathrm{V}}{330\ \Omega}
  = 8.48\ \mathrm{mA}
$$

Ideal expected current range, neglecting the small MOSFET ON voltage: approximately 8.5–9.7 mA.

## Expected Current Using Measured Resistance

Nominal series resistance: 330 Ω  
Measured series resistance: 321 Ω

At $V_F = 1.8\ \mathrm{V}$:

$$
I = \frac{V_S - V_F}{R}
  = \frac{5.0\ \mathrm{V} - 1.8\ \mathrm{V}}{321\ \Omega}
  = 9.97\ \mathrm{mA}
$$

At $V_F = 2.2\ \mathrm{V}$:

$$
I = \frac{5.0\ \mathrm{V} - 2.2\ \mathrm{V}}{321\ \Omega}
  = 8.72\ \mathrm{mA}
$$

Ideal as-built expected current range, neglecting the small MOSFET
ON voltage: approximately 8.72–9.97 mA.
