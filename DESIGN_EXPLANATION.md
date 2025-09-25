# Design Explanation

## Data Structures

### `typedef struct Transaction`
- `int id`
- `char date[11]`
- `char time[9]`
- `int from_cur`, `int to_cur`
- `double amount_from`, `double amount_to`
- `double rate`            (effective rate for this tx)
- `double profit`          (in LOC)

### `typedef struct Currency`
- `char name[MAX_NAME]`
- `int d_count`
- `const int *denoms`
- `double start_bal`
- `double bal`
- `double critical_min`
- `double buy_to_loc`      (price to buy foreign → LOC)
- `double sell_to_loc`     (price to sell foreign → LOC)

Rationale: These structs centralize runtime state and keep CSV/receipt logic clear and type-safe.

## Key Functions (as implemented)
- `init_defaults()` — Initialize currency names, denominations, default balances/rates.
- `ask_int(...)`, `ask_double(...)`, `clear_input()` — Robust user input with range checks; doubles parsed with `fgets`/`strtod`.
- `make_daily_csv_name(date, out, cap)` — Build per-day CSV pathname.
- `ensure_csv_header(FILE*)` — Write header if file is empty/new.
- `csv_log_transaction(date, tx_id, from, to, amt_from, amt_to, rate_from_loc, rate_to_loc, partial, remainder_loc, profit_delta)` — Append **new-format** CSV row.
- `csv_list_transactions_for_date(date)` — Print all rows (supports **legacy** and **new** formats; skips malformed lines).
- `csv_find_transaction_by_id(date, tx_id)` — Locate and print one row.
- `csv_append_manual_transaction(...)` — Append a manual row in **new-format**.
- `csv_sum_profit_for_date(date, *tx_count)` — Sum profit and count rows for **a single date**.
- `csv_sum_profit_for_month(...)` — Monthly aggregation for reporting.
- `generate_receipt(Transaction*, date)` — Human-readable receipt per transaction.
- `generate_daily_summary(date)` — End-of-day summary/receipt using CSV scan.
- `load_last_tx_id() / save_last_tx_id(int)` — Persist the transaction ID counter across runs.

## Control Flow (high level)
- `scenario_exchange()` — Validate currencies/amounts; compute via LOC; handle **partial** logic and denominations; update balances; log CSV; generate receipt.
- `scenario_show_rates()`, `scenario_mgmt_set_rates()`, `scenario_mgmt_reserves()`, `scenario_mgmt_crit()` — View/update runtime parameters.
- `scenario_show_balances()` — Print currency states and critical warnings.
- `scenario_help()` — Show usage help.
- `scenario_end_of_day()` — Scan CSV and produce daily summary.

## Error Handling
- File open failures are checked and reported.
- CSV parsing: malformed lines are **skipped**; totals/counts only include successfully parsed rows.
- Input validation loops until valid values are entered.

## Compatibility
- CSV reader accepts **legacy** rows (no `tx_id`, fewer fields) **and** **new** rows (with `tx_id`, `partial`, `remainder_loc`, `profit`).

## Design Rationale (concise)
- **Separation of concerns** between UI and persistence.
- **Auditability** via unique `tx_id` and receipts.
- **Resilience** (skip bad lines; continue).
- **Clarity** (docs now match code exactly).