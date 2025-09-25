#!/usr/bin/env bash
set -euo pipefail
WD=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$WD/.." && pwd)

echo "Running tests from $ROOT"

# Ensure binary exists
if [ ! -x "$ROOT/build/exchange_store_cp1" ]; then
  echo "Binary not found; building..."
  (cd "$ROOT" && make)
fi

# Prepare test CSVs
mkdir -p "$ROOT/tests/data"
cat > "$ROOT/tests/data/sales_2025-09-22.csv" <<'CSV'
date,time,from_currency,to_currency,amount_from,amount_to,rate_from_loc,rate_to_loc,partial,remainder_loc,profit_loc
2025-09-22,12:50:24,1,LOC,100000.000000,2412.545235,1.000000,41.450000,0,0.000000,217.129071
CSV

cat > "$ROOT/tests/data/sales_2025-09-23.csv" <<'CSV'
# new format with tx_id
2025-09-23,09:00:00,1,USD,LOC,500.000000,12.091234,1.000000,41.400000,0,0.000000,10.123456
CSV

# Copy test CSVs into project root (simulate daily files)
cp "$ROOT/tests/data/sales_2025-09-22.csv" "$ROOT/sales_2025-09-22.csv"
cp "$ROOT/tests/data/sales_2025-09-23.csv" "$ROOT/sales_2025-09-23.csv"

# Run summary for 2025-09-22
echo "--- Summary for 2025-09-22 ---"
"$ROOT/build/exchange_store_cp1" <<'EOF'
7
0
EOF

# List transactions for 2025-09-22 using CSV helper (we'll call the binary's menu option 10)
echo "--- List transactions (menu option 10) for 2025-09-22 ---"
"$ROOT/build/exchange_store_cp1" <<'EOF'
10
2025-09-22
0
EOF

# Search for a tx_id that won't exist (menu option 11)
echo "--- Search for tx id 1 (menu option 11) ---"
"$ROOT/build/exchange_store_cp1" <<'EOF'
11
1
0
EOF

echo "Tests completed. Check outputs above."
