// Currency Exchange Store - Main Implementation
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "utils.h"

static int choose_currency(const char *prompt) {
    printf("%s\n", prompt);
    fflush(stdout);
    for (int i = 0; i < MAX_CUR; ++i) {
        printf("  %d) %s\n", i, CUR_NAME[i]);
        fflush(stdout);
    }
    int c = ask_int("Select currency index:", 0, MAX_CUR-1);
    return c;
}

static void check_criticals(void) {
    for (int i = 0; i < MAX_CUR; ++i) {
        if (currencies[i].bal < currencies[i].critical_min) {
            printf("[-] ALERT: %s reserve below critical minimum (%.2f < %.2f)\n",
                   currencies[i].name, currencies[i].bal, currencies[i].critical_min);
        }
    }
}

static double convert_via_local(int from, int to, double amount_from,
                                double *rate_from_loc, double *rate_to_loc,
                                double *profit_delta_loc) {
    double loc_in = amount_from * currencies[from].buy_to_loc;
    double amount_to = loc_in / currencies[to].sell_to_loc;

    if (rate_from_loc) *rate_from_loc = currencies[from].buy_to_loc;
    if (rate_to_loc)   *rate_to_loc   = currencies[to].sell_to_loc;

    double cost_loc = amount_to * currencies[to].buy_to_loc;
    double profit_delta = loc_in - cost_loc;
    if (profit_delta_loc) *profit_delta_loc = profit_delta;
    return amount_to;
}

static void pay_in_denoms(int cur, double amount) {
    long long total = (long long)(amount);
    double fractional = amount - total; 
    const int *den = DENOMS[cur];
    int n = D_COUNT[cur];

    if (total <= 0 || !den) {
        printf("[-] No denomination breakdown available for this amount.\n");
        fflush(stdout);
        return;
    }

    printf("\n=== Denomination Breakdown for %s %.2f ===\n", CUR_NAME[cur], amount);
    printf("Notes/Coins Required:\n");
    fflush(stdout);

    int total_pieces = 0;
    
    for (int i = 0; i < n; ++i) {
        int d = den[i];
        if (d <= 0) continue;
        long long cnt = total / d;
        if (cnt > 0) {
            printf("  %3d %s x %lld\n", 
                   d, 
                   d >= 20 ? "note(s)" : "coin(s)", 
                   cnt);
            fflush(stdout);
            total -= cnt * d;
            total_pieces += cnt;
        }
    }

    if (fractional > 0.009) {  // Check for significant fractional part
        printf("\nFractional amount: %.2f %s\n", fractional, CUR_NAME[cur]);
    }

    if (total > 0) {
        printf("\nWarning: Remainder of %lld cannot be broken down further\n", total);
    }

    printf("\nTotal pieces to handle: %d\n", total_pieces);
    printf("=======================================\n\n");
    fflush(stdout);
}

/* Print and save a receipt (tx_id must be provided by caller) */
static void handle_receipt(int tx_id, const char *date_text, int from, int to, double amt_from,
                         double amt_to, double rate) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char time_str[9];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
    
    Transaction trans = {
        .id = tx_id,
        .from_cur = from,
        .to_cur = to,
        .amount_from = amt_from,
        .amount_to = amt_to,
        .rate = rate
    };
    strncpy(trans.date, date_text, sizeof(trans.date) - 1);
    trans.date[sizeof(trans.date)-1] = '\0';
    strncpy(trans.time, time_str, sizeof(trans.time) - 1);
    trans.time[sizeof(trans.time)-1] = '\0';
    
    generate_receipt(&trans, date_text);
}

static void scenario_exchange(void) {
    int from = choose_currency("Currency you GIVE to the cashier (from client):");
    int to   = choose_currency("Currency you WANT to receive (to client):");
    double amt_from = ask_double("Enter amount to exchange:", 0.01, 1e12);

    if (from == to) {
        printf("[-] From and To currencies are the same. Nothing to do.\n");
        fflush(stdout);
        return;
    }

    double rate_from_loc = 0.0, rate_to_loc = 0.0, profit_delta = 0.0;
    double amt_to = convert_via_local(from, to, amt_from, &rate_from_loc, &rate_to_loc, &profit_delta);

    if (currencies[to].bal < amt_to) {
        printf("[-] Insufficient reserve of %s. Available: %.2f, Needed: %.2f\n",
               CUR_NAME[to], currencies[to].bal, amt_to);
        fflush(stdout);
        return;
    }

    int partial = ask_int("Partial exchange? 1=Yes, 0=No:", 0, 1);
    double part_fixed_to = 0.0;
    double remainder_loc_for_client = 0.0;

    if (partial) {
        part_fixed_to = ask_double("Enter how many units of the target currency to receive now:", 0.0, amt_to);
        double loc_value_total = amt_from * currencies[from].buy_to_loc;
        double loc_value_given_as_to = part_fixed_to * currencies[to].sell_to_loc;
        if (loc_value_given_as_to > loc_value_total + 1e-9) {
            printf("[-] Chosen partial amount exceeds exchangeable value. Aborting.\n");
            fflush(stdout);
            return;
        }
        remainder_loc_for_client = loc_value_total - loc_value_given_as_to;

        amt_to = part_fixed_to;
    }

    currencies[from].bal += amt_from;
    currencies[to].bal   -= amt_to;
    if (partial) {
        if (currencies[CUR_LOC].bal < remainder_loc_for_client) {
            printf("[-] Insufficient LOC reserve for partial payout remainder (need %.2f LOC).\n", remainder_loc_for_client);
            fflush(stdout);
            currencies[from].bal -= amt_from;
            currencies[to].bal   += amt_to;
            return;
        }
        currencies[CUR_LOC].bal -= remainder_loc_for_client;
    }

    profit_loc += profit_delta;

    double effective_rate = amt_to / amt_from;
        int tx_id = ++last_transaction_id;
        handle_receipt(tx_id, current_date, from, to, amt_from, amt_to, effective_rate);
        save_last_tx_id(last_transaction_id);
    
    if (partial) {
        printf("Partial payout details: %.2f %s paid; remainder to client: %.2f LOC\n",
               amt_to, CUR_NAME[to], remainder_loc_for_client);
        fflush(stdout);
    }
    csv_log_transaction(current_date, tx_id, from, to, amt_from, amt_to,
                    rate_from_loc, rate_to_loc,
                    partial, remainder_loc_for_client, profit_delta);

    int want_denoms = ask_int("Would you like a denomination breakdown for the payout currency? 1=Yes,0=No:", 0, 1);
    if (want_denoms) {
        pay_in_denoms(to, amt_to);
        if (partial && remainder_loc_for_client > 0.0) {
            int want_loc = ask_int("Breakdown for the LOC remainder as well? 1=Yes,0=No:", 0, 1);
            if (want_loc) pay_in_denoms(CUR_LOC, remainder_loc_for_client);
        }
    }

    check_criticals();
}

static void scenario_show_rates(void) {
    printf("\n[*] Current Exchange Rates (relative to LOC)\n");
    printf("Index  Code   BUY->LOC        SELL->LOC\n");
    fflush(stdout);
    for (int i = 0; i < MAX_CUR; ++i) {
        printf("%5d  %-5s  %12.6f  %12.6f\n", i, CUR_NAME[i], currencies[i].buy_to_loc, currencies[i].sell_to_loc);
        fflush(stdout);
    }
    printf("Note: BUY->LOC is what the desk credits in LOC per 1 unit when client gives that currency.\n");
    printf("      SELL->LOC is what the client must pay in LOC per 1 unit of that currency they receive.\n\n");
    fflush(stdout);
}

static void scenario_mgmt_set_rates(void) {
    printf("\n--- Management: Set Rates (relative to LOC) ---\n");
    printf("For each currency, enter BUY->LOC then SELL->LOC (must be > 0 and SELL >= BUY).\n");
    fflush(stdout);
    for (int i = 0; i < MAX_CUR; ++i) {
        double buy = ask_double((char [BUF]){0}, 0.000001, 1e12);
        printf("Currency %s:\n", CUR_NAME[i]);
        fflush(stdout);
        buy = ask_double("  BUY->LOC:", 0.000001, 1e12);
        double sell = ask_double("  SELL->LOC (>= BUY):", buy, 1e12);
        currencies[i].buy_to_loc = buy;
        currencies[i].sell_to_loc = sell;
    }
    printf("[*] Rates updated.\n\n");
    fflush(stdout);
}

static void scenario_mgmt_reserves(void) {
    printf("\n--- Management: Adjust Reserves ---\n");
    fflush(stdout);
    int idx = choose_currency("Select currency to modify:");
    double delta = ask_double("Positive to add to reserve, negative to remove:", -1e12, 1e12);
    if (currencies[idx].bal + delta < 0) {
        printf("[-] Operation would make reserve negative. Aborted.\n");
        fflush(stdout);
        return;
    }
    currencies[idx].bal += delta;
    if (delta >= 0) printf("Added %.2f %s to reserves.\n", delta, CUR_NAME[idx]);
    else            printf("Removed %.2f %s from reserves.\n", -delta, CUR_NAME[idx]);
    fflush(stdout);
}

static void scenario_mgmt_crit(void) {
    printf("\n--- Management: Set Critical Minimums ---\n");
    fflush(stdout);
    for (int i = 0; i < MAX_CUR; ++i) {
        printf("Currency %s:\n", CUR_NAME[i]);
        fflush(stdout);
        currencies[i].critical_min = ask_double("  Critical minimum:", 0.0, 1e12);
    }
    printf("[*] Critical minimums updated.\n\n");
    fflush(stdout);
}

static void scenario_show_balances(void) {
    printf("\n[*] Current Balances\n");
    fflush(stdout);
    for (int i = 0; i < MAX_CUR; ++i) {
        printf("%s: %.2f\n", CUR_NAME[i], currencies[i].bal);
    }
    printf("\n");
    fflush(stdout);
}

static void scenario_help(void) {
    printf("\n=== Currency Exchange System - Help ===\n\n");
    printf("Available Operations:\n\n");
    printf("1. Perform Exchange\n");
    printf("   - Select source and target currencies\n");
    printf("   - Enter amount to exchange\n");
    printf("   - Option for partial exchange in LOC\n");
    printf("   - Get denomination breakdown\n\n");
    
    printf("2. Show Rates\n");
    printf("   - Display current exchange rates\n");
    printf("   - Shows buy and sell rates for all currencies\n\n");
    
    printf("3. Management: Set Rates\n");
    printf("   - Update buy and sell rates\n");
    printf("   - Must maintain buy ≤ sell relationship\n\n");
    
    printf("4. Management: Adjust Reserves\n");
    printf("   - Add or remove currency from reserves\n");
    printf("   - Cannot reduce below zero\n\n");
    
    printf("5. Management: Set Critical Minimums\n");
    printf("   - Set alert thresholds for each currency\n");
    printf("   - System warns when balance falls below minimum\n\n");
    
    printf("6. Show Balances\n");
    printf("   - Display current reserves for all currencies\n\n");
    
    printf("7. End-of-day Report\n");
    printf("   - Summary of all transactions\n");
    printf("   - Total profit calculation\n");
    printf("   - Critical balance warnings\n\n");
    
    printf("Features:\n");
    printf("- Automatic receipt generation\n");
    printf("- Transaction logging in CSV format\n");
    printf("- Denomination breakdown assistance\n");
    printf("- Critical balance monitoring\n");
    printf("- Partial exchange support\n\n");
    
    printf("Press Enter to continue...");
    getchar();
}

void scenario_end_of_day(const char *current_date) {
    generate_daily_summary(current_date);
    check_criticals();
}

static void show_menu(void) {
    printf("============================================\n");
    printf("   [*] Currency Exchange - Main Menu\n");
    printf("   Date: %s\n", current_date);
    printf("============================================\n");
    printf(" 1) Perform an exchange\n");
    printf(" 2) Show rates\n");
    printf(" 3) Management: set rates\n");
    printf(" 4) Management: adjust reserves\n");
    printf(" 5) Management: set critical minimums\n");
    printf(" 6) Show balances\n");
    printf(" 7) End-of-day report\n");
    printf(" 8) Help/About\n");
    printf(" 9) Add manual transaction (append to CSV)\n");
    printf("10) List transactions for a date\n");
    printf("11) Search transaction by ID (today)\n");
    printf(" 0) Exit\n");
    fflush(stdout);
}

int main(void) {
    init_defaults();
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(current_date, sizeof(current_date), "%Y-%m-%d", tm_info);
    
    while (1) {
        show_menu();
        int choice = ask_int("Choose option:", 0, 11);
        switch (choice) {
            case 0: return 0;
            case 1: scenario_exchange(); break;
            case 2: scenario_show_rates(); break;
            case 3: scenario_mgmt_set_rates(); break;
            case 4: scenario_mgmt_reserves(); break;
            case 5: scenario_mgmt_crit(); break;
            case 6: scenario_show_balances(); break;
            case 7: scenario_end_of_day(current_date); break;
            case 8: scenario_help(); break;
            case 9: {
                int from = choose_currency("From currency index:");
                int to = choose_currency("To currency index:");
                double amt_from = ask_double("Amount from:", 0.0, 1e12);
                double amt_to = ask_double("Amount to:", 0.0, 1e12);
                time_t tt = time(NULL);
                struct tm *tm2 = localtime(&tt);
                char timestr[16];
                strftime(timestr, sizeof(timestr), "%H:%M:%S", tm2);
                int txid = ++last_transaction_id;
                csv_append_manual_transaction(current_date, txid, timestr, CUR_NAME[from], CUR_NAME[to],
                                              amt_from, amt_to, currencies[from].buy_to_loc, currencies[to].sell_to_loc,
                                              0, 0.0, 0.0);
                save_last_tx_id(last_transaction_id);
                printf("Added transaction id %d\n", txid);
                break;
            }
            case 10: {
                char datebuf[32];
                printf("Enter date (YYYY-MM-DD) or press Enter for today: ");
                if (!fgets(datebuf, sizeof(datebuf), stdin)) break;
                size_t L = strlen(datebuf); while (L && (datebuf[L-1]=='\n' || datebuf[L-1]=='\r')) datebuf[--L] = '\0';
                if (L == 0) strncpy(datebuf, current_date, sizeof(datebuf));
                csv_list_transactions_for_date(datebuf);
                break;
            }
            case 11: {
                int qid = ask_int("Enter transaction ID to search:", 1, 1000000000);
                int found = csv_find_transaction_by_id(current_date, qid);
                if (!found) printf("Transaction %d not found for %s\n", qid, current_date);
                break;
            }
            default: printf("Unknown option\n"); break;
        }
    }

    for (;;) {
        show_menu();
        int choice = ask_int("Choose:", 0, 8);
        switch (choice) {
            case 1: scenario_exchange(); break;
            case 2: scenario_show_rates(); break;
            case 3: scenario_mgmt_set_rates(); break;
            case 4: scenario_mgmt_reserves(); break;
            case 5: scenario_mgmt_crit(); break;
            case 6: scenario_show_balances(); break;
            case 7: scenario_end_of_day(current_date); break;
            case 8: scenario_help(); break;
            case 0:
                printf("[*] Goodbye!\n");
                fflush(stdout);
                free(currencies);  
                return 0;
            default:
                printf("[-] Unknown option.\n");
                break;
        }
    }
}