# Decision Rules — Currency Exchange Store

This document lists **inputs → actions** for each operation. No complex tables are used.

## Common Inputs

- **operation**: `exchange`, `add_tx`, `list_date`, `search_tx`, `eod_summary`
- **currencies valid**: `from` and `to` are within `[0..4]` (LOC, USD, EUR, GBP, JPY)
- **sufficient reserve**: enough currency is available to fulfill payout
- **partial flag**: `0` (no partial) or `1` (allow partial with remainder in LOC)
- **CSV format**: parser supports **legacy** rows (no `tx_id`, fewer fields) and **new** rows (with `tx_id`, `partial`, `remainder_loc`, `profit`)

## Operation: `exchange`

- If **currencies invalid** → **Reject**, print error, no state change.
- If **reserve insufficient** and **partial=0** → **Reject**, print error, no state change.
- If **reserve insufficient** and **partial=1** → **Accept partial**:
  - Convert the payable portion; compute **remainder in LOC** for the client.
  - **Update balances**, **log CSV**, **generate receipt**, **update profit**.
- If **currencies valid** and **reserve sufficient** → **Accept**:
  - **Update balances** (debit `from`, credit `to` via LOC path).
  - **Persist to CSV** (write header if needed, include `tx_id`).
  - **Generate receipt**; **profit** accumulates in LOC.

## Operation: `add_tx` (manual append)

- If **currencies valid** → **Append to CSV** using the new row format.
- `last_tx_id` may be advanced to keep IDs unique.
- No balances are auto-recalculated here unless explicitly designed in UI (current CLI treats as log append).

## Operation: `list_date`

- If **CSV for date exists** → **Read & print** every row.
  - Malformed rows are **skipped** with a warning.
  - Supports both **legacy** and **new** formats.
- If **no file** → print “no transactions.”

## Operation: `search_tx`

- If **tx_id found** in the date’s CSV → **Print row details**.
- If not found → print “not found.”

## Operation: `eod_summary` (end of day)

- Compute **total profit** and **transaction count** for a date via CSV scan.
- Write a summary/receipt entry for audit.

## Notes

- **Unique Transaction IDs** persist via `load_last_tx_id` / `save_last_tx_id` so IDs survive restarts.
- **Input handling**: integers via `ask_int` (with `clear_input`), doubles via `ask_double` (fgets/strtod) to avoid scanf pitfalls.
- **Denominations** are used for payout breakdown; when partial exchanges are enabled, the **remainder in LOC** is recorded.

