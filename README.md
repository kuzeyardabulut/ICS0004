# ICS0004 — Currency Exchange Store

Fundamentals of Programming (TalTech) coursework project

---

## 🚀 Overview
A console application that simulates a small currency‑exchange office. It lets a cashier perform exchanges between supported currencies, validate inputs, apply fixed rates, manage cash reserves, and generate receipts and end‑of‑day reports. The codebase is written in **C (C11)** and built with a simple **Makefile**.

> **Why this exists**: to practice structured programming, input validation, basic algorithms, file I/O, and modular C design without using network access or external libraries.

---

## ✨ Features
- **Exchange workflow**
  - Choose *FROM* and *TO* currencies
  - Enter amount, validate inputs
  - Check reserves and optionally offer a **partial exchange** when reserves are low
  - (Optional) **Denomination breakdown** for cash payout
  - Generate and show a **receipt**
- **Rates & reserves**
  - Show current fixed (hard‑coded) rates
  - **Set rates** (management menu)
  - **Adjust reserves** (add/remove)
  - **Set critical minimums** (warns when a currency’s reserve is too low)
- **Reporting**
  - Show in‑memory balances/reserves
  - **End‑of‑day report** (summary)
  - **Add a manual transaction** (append to CSV)
  - **List transactions for a date**
  - **Search transaction by ID (today)**
- **Help/About** screen

> The exact menu entries may look like this in the program:
>
> ```text
>  1) Perform an exchange
>  2) Show rates
>  3) Management: set rates
>  4) Management: adjust reserves
>  5) Management: set critical minimums
>  6) Show balances
>  7) End‑of‑day report
>  8) Help/About
>  9) Add manual transaction (append to CSV)
> 10) List transactions for a date
> 11) Search transaction by ID (today)
>  0) Exit
> ```

---

## 📁 Project structure
```
ICS0004/
├─ main.c                 # App entry point: menus, control flow
├─ utils.c                # Input helpers, validation, formatting, I/O
├─ utils.h                # Shared declarations
├─ Makefile               # Build/run/test helpers
├─ DECISION_TABLE.md      # Decision logic summary (scenarios & outcomes)
├─ DESIGN_EXPLANATION.md  # Design notes, assumptions, constraints
├─ docs/                  # Additional docs (diagrams/specs)
└─ tests/                 # Test scripts & fixtures (e.g., test_runner.sh)
```

---

## 🛠️ Build
**Requirements**
- POSIX‑like environment (Linux/macOS/WSL)
- `gcc` or `clang` with C11 support
- `make`

**Commands**
```bash
# Build
make

# Run the app (builds if needed)
make run

# Run simple tests (if available)
make test

# Clean build artifacts
make clean
```

The default build target produces a binary under `build/` (e.g., `build/exchange_store_cp1`).

---

## ▶️ Usage
Launch the program and follow the on‑screen menu. Typical flow for a cash exchange:
1. Choose **Perform an exchange**
2. Select **FROM** and **TO** currencies
3. Enter the **amount**
4. The program verifies reserves and may offer **partial exchange** if needed
5. (Optional) Request **denomination breakdown** for payout
6. A **receipt** prints to the console; some actions may append to a CSV log

**Notes**
- Rates are **fixed** (hard‑coded) and **no internet access** is used.
- Input is sanitized; invalid numeric input is rejected and re‑prompted.
- Very large values are constrained to prevent integer overflow.

---

## 🧪 Tests
If `tests/test_runner.sh` is present, you can run:
```bash
make test
```
This typically executes the compiled program against basic scenarios. Feel free to extend the script with more cases.

---

## 🔧 Design highlights
- Clear separation between **UI/menu logic** (`main.c`) and **helpers/validation** (`utils.c`).
- Consistent input handling (`scanf` + buffer clearing) to avoid leftover input artifacts.
- Defensive checks around amounts and reserve adjustments; guides the user when limits are exceeded.
- CSV append for manual transactions and date‑based lookups (when those features are enabled).

For deeper rationale, see **DESIGN_EXPLANATION.md** and **DECISION_TABLE.md**.

---

## 🗺️ Roadmap ideas
- Persist rates and reserves to files (load/save between runs)
- Stronger receipt/report formatting (timestamps, IDs)
- More comprehensive automated tests
- Locale/currency formatting helpers

---

## 🧑‍🤝‍🧑 Roles (course context)
- **Client** — enters amount and sees the result
- **Cashier** — runs the program to perform exchanges
- **Administrator** — sets fixed rates and manages reserves

---

## 📄 License
No license file is included. If you plan to share or reuse this code publicly, consider adding a license (e.g., MIT).

---

## 🙌 Acknowledgments
Developed for **ICS0004 — Fundamentals of Programming** at **TalTech**. Thanks to the course staff and peers for guidance and feedback.
