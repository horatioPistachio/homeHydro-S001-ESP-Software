# EC meter saturated saline calibration — notebook implementation prompt

This prompt specifies a reproducible Jupyter notebook to calibrate an EC (electrical conductivity) probe by titrating a known saturated NaCl solution into a known volume of water and fitting calibration parameters.

## Experiment setup
- Start with V0_ml = 500 mL of tap water, with an approximate initial EC E0_uScm ≈ 327 µS/cm.
- Place the EC probe in the container and record the device output.
- Add one drop of saturated saline (NaCl) solution per step. The saturated solution EC at 25°C is assumed to be E_sat_uScm = 292,000 µS/cm.
- After each drop, stir and record the probe output. Repeat until the mixture EC is about 20,000 µS/cm.

Notes:
- Record or supply the measurement temperature(s). EC depends on temperature; we will normalize to 25°C using a temperature coefficient.
- If possible, measure or specify the drop volume v_drop_ml (e.g., with a pipette). If unknown, use a reasonable default (0.05mL) and allow sensitivity analysis.

## Objectives
Create a Jupyter notebook that:
- Ingests the recorded measurements and known experiment parameters.
- Computes the theoretical “true” EC for each step based on volume mixing and temperature compensation.
- Fits calibration parameters mapping the probe’s reported value to the true EC.
- Visualizes fit quality and stability across the EC range.
- Exports calibration artifacts for firmware use and compares results against `EC_meter_calibration_calc.ipynb`.

## Inputs
Provide a CSV with at least one of the following columns:
- If your device reports a raw analog/voltage: `drop, raw_voltage_V`

Example (device reports EC in mS/cm):
```
drop,raw_voltage_V
0,0.327
1,2.92
2,5.85
3,8.67
4,11.5
5,14.3
6,17.1
7,19.9
```

Experiment parameters (provide via a small YAML/JSON block in the notebook or a params cell):
- `V0_ml` (float, default 500.0): Initial water volume in mL.
- `E0_uScm` (float, default 327.0): Initial water EC at 25°C in µS/cm.
- `E_sat_uScm` (float, default 292000.0): Saturated NaCl EC at 25°C in µS/cm.
- `v_drop_ml` (float, default 0.05): Volume per drop in mL.
- `alpha_per_C` (float, default 0.02): Temperature coefficient for EC normalization (typ. 2%/°C for NaCl solutions).
- `T_C` (float or list, default 25.0): Bath temperature in °C (single value or per-measurement list).
- `tds_factor` (float, optional, default 0.5): TDS-to-EC conversion factor if ppm outputs are desired (ppm ≈ tds_factor × EC(µS/cm)).

Assumptions:
- EC is approximately proportional to NaCl concentration over the target range; linear mixing by volume is acceptable for this calibration.
- All EC values are normalized to 25°C before comparing or fitting.

## Calculations
1) Temperature normalization to 25°C
For an EC measured at temperature T (°C):
```
EC_25 = EC_T / (1 + alpha_per_C × (T - 25))
```
Apply to `E0_uScm`, `E_sat_uScm` if they were recorded at T ≠ 25°C, and to any device-reported EC values if needed.

2) Mixture EC per drop (true/reference EC)
Let Vn = V0_ml + n × v_drop_ml. Using a linear-by-volume approximation for conductivity contribution:
```
numerator_uS_cm_ml = E0_uScm × V0_ml + n × (E_sat_uScm × v_drop_ml)
EC_true_uScm(n) = numerator_uS_cm_ml / Vn
EC_true_mScm(n) = EC_true_uScm(n) / 1000
```

3) Calibration model(s)

- If the device reports a raw voltage, fit a linear model to map voltage to EC:
```
EC_true_mScm ≈ a × raw_voltage_V + b
```
Estimate a, b via least squares. Optionally fit through origin (b=0) and compare residuals. Report confidence intervals and goodness-of-fit (R², RMSE).

4) Optional TDS/ppm
If `tds_factor` is provided, compute `ppm_true = tds_factor × EC_true_uScm` and produce analogous plots/tables.

## Notebook structure
Include the following cells/sections:
1. Title and overview.
2. Parameters cell with defaults (see Inputs/parameters).
3. Data ingestion: read CSV, basic validation (drop monotonicity, missing values, column presence).
4. Temperature normalization helper(s).
5. Compute EC_true per drop from mixing model.
6. Calibration:
  - EC-reporting device: compute K_cell per point; summary stats; trend analysis.
  - Voltage-reporting device: fit linear model(s) (with/without intercept); diagnostics.
7. Plots:
  - EC_true vs drop number.
  - Measured vs EC_true with 1:1 line, residuals vs EC_true.
  - If EC-reporting device: K_cell vs EC_true.
  - Optional: Bland–Altman plot.
8. Summary and recommendations: calibration constants, stability range, any nonlinearity.
9. Export artifacts:
  - JSON with metadata (date, parameters, fit coefficients, errors).
  - A small C/C++ header snippet suitable for firmware (e.g., constants `EC_SLOPE_A`, `EC_OFFSET_B` or `EC_CELL_K`).
10. Comparison against `EC_meter_calibration_calc.ipynb`:
  - Load or summarize prior results; overlay plots; diff key constants.

## Outputs
- The Jupyter notebook (.ipynb) containing:
  - Well-documented code cells for the above steps
  - Plots and tables
  - Summary statistics (means, standard deviations, R², RMSE)
  - JSON export of calibration data (written to a `ec_calibration/` folder)
  - Firmware header snippet printed in a cell for copy/paste
- A brief note on limitations (linear-mixing approximation, temp coefficient assumptions).

## Acceptance criteria
- The notebook runs end-to-end with a provided CSV and default parameters.
- Produces EC_true per step and fits calibration with diagnostics.
- Exports a JSON with the chosen calibration model and errors.
- Provides a small header snippet ready to embed in firmware.
- Includes a comparison section referencing `EC_meter_calibration_calc.ipynb`.

## Edge cases to handle
- Missing or irregular drop numbers (warn and sort).
- Outliers in measured values (flag and optionally exclude with a toggle).
- Unknown `v_drop_ml` (allow parameter sweep over a plausible range and show sensitivity of the fit).
- Non-25°C measurements (apply temperature normalization or warn if T is missing).

## Example header snippet (output)
For an EC-reporting device with stable K_cell:
```
// Auto-generated by EC calibration notebook (YYYY-MM-DD)
#pragma once

// Cell constant to map device-reported EC (mS/cm) to true EC (mS/cm)
// EC_true_mScm = EC_reported_mScm * EC_CELL_K
static constexpr float EC_CELL_K = 1.037f;  // example value
```

For a voltage-reporting device (linear model):
```
// EC_true_mScm ≈ EC_SLOPE_A * raw_voltage_V + EC_OFFSET_B
static constexpr float EC_SLOPE_A = 6.123f;  // example
static constexpr float EC_OFFSET_B = 0.015f; // example
```

## Notes
- If you ultimately need salinity in ppm, prefer computing ppm from EC using a stated `tds_factor` and report that factor explicitly alongside results.
- If your prior notebook used a different definition for “K”, document the mapping to `K_cell` used here to avoid confusion.
