#define _GNU_SOURCE

#include "utils.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>

#define MAX_CUR_LOCAL 5

const char *CUR_NAME[MAX_CUR_LOCAL] = { "LOC", "USD", "EUR", "GBP", "JPY" };
const int D_COUNT[MAX_CUR_LOCAL] =  { 9, 7, 7, 7, 7 };
const int DENOMS_LOC[] = { 200, 100, 50, 20, 10, 5, 2, 1, 0 };
const int DENOMS_USD[] = { 100, 50, 20, 10, 5, 2, 1 };
const int DENOMS_EUR[] = { 500, 200, 100, 50, 20, 10, 5 };
const int DENOMS_GBP[] = { 50, 20, 10, 5, 2, 1, 0 };
const int DENOMS_JPY[] = { 10000, 5000, 2000, 1000, 500, 100, 50 };
const int *DENOMS[MAX_CUR_LOCAL] = { DENOMS_LOC, DENOMS_USD, DENOMS_EUR, DENOMS_GBP, DENOMS_JPY };

Currency *currencies = NULL;

double profit_loc = 0.0;
char current_date[64] = "N/A";
int last_transaction_id = 0;

static const char *RECEIPT_FILE = "receipts_%s.txt";

int load_last_tx_id(void) {
    FILE *f = fopen("last_tx_id.txt", "r");
    if (!f) return 0;
    int id = 0;
    if (fscanf(f, "%d", &id) != 1) id = 0;
    fclose(f);
    return id;
}

void save_last_tx_id(int id) {
    FILE *f = fopen("last_tx_id.txt", "w");
    if (!f) return;
    fprintf(f, "%d\n", id);
    fclose(f);
}

void clear_input(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { /* discard */ }
}

int ask_int(const char *prompt, int min, int max) {
    char buffer[256];
    long val;
    do {
        printf("%s ", prompt);
        fflush(stdout);
        if (!fgets(buffer, sizeof(buffer), stdin)) {
            printf("Error reading input. Please try again.\n");
            fflush(stdout);
            continue;
        }
        if (buffer[0] == '\n') {
            printf("Empty input not allowed. Please enter a number.\n");
            fflush(stdout);
            continue;
        }
        errno = 0;
        char *endptr;
        val = strtol(buffer, &endptr, 10);
        if (endptr == buffer || (*endptr != '\n' && *endptr != '\0')) {
            printf("Invalid input. Try again.\n");
            fflush(stdout);
            continue;
        }
        if (errno == ERANGE) {
            printf("Integer overflow error: value is too large or too small. Please try again.\n");
            fflush(stdout);
            continue;
        }
        if (val < min || val > max) {
            printf("Please enter a number in [%d..%d]\n", min, max);
            fflush(stdout);
            continue;
        }
        break;
    } while (1);
    return (int)val;
}

double ask_double(const char *prompt, double min, double max) {
    double x;
    char buffer[256];
    do {
        printf("%s ", prompt);
        fflush(stdout);
        if (!fgets(buffer, sizeof(buffer), stdin)) {
            printf("Error reading input. Please try again.\n");
            fflush(stdout);
            continue;
        }
        if (buffer[0] == '\n') {
            printf("Empty input not allowed. Please enter a number.\n");
            fflush(stdout);
            continue;
        }
            errno = 0;
            char *endptr_int;
            long long li = strtoll(buffer, &endptr_int, 10);
            if (endptr_int != buffer && (*endptr_int == '\n' || *endptr_int == '\0')) {
                if (errno == ERANGE) {
                    printf("Integer overflow error: value is too large or too small. Please try again.\n");
                    fflush(stdout);
                    continue;
                }
                x = (double)li;
            } else {
                errno = 0;
                char *endptr;
                x = strtod(buffer, &endptr);
                if (endptr == buffer || (*endptr != '\n' && *endptr != '\0')) {
                    printf("Invalid input: Please enter a valid number.\n");
                    fflush(stdout);
                    continue;
                }
                if (errno == ERANGE) {
                    printf("Integer overflow error: value is too large or too small. Please try again.\n");
                    fflush(stdout);
                    continue;
                }
            }
        if (x < min || x > max) {
            printf("Value must be between %.2f and %.2f. Please try again.\n", min, max);
            fflush(stdout);
            continue;
        }
        break;
    } while (1);
    return x;
}

void init_defaults(void) {
    if (currencies) free(currencies);
    currencies = calloc(MAX_CUR_LOCAL, sizeof(Currency));
    if (!currencies) {
        fprintf(stderr, "Memory allocation failed for currencies!\n");
        fflush(stdout);
        exit(1);
    }

    for (int i = 0; i < MAX_CUR_LOCAL; ++i) {
        const char *src = CUR_NAME[i];
        int j = 0;
        for (; src[j] && j < MAX_NAME-1; ++j)
            currencies[i].name[j] = src[j];
        currencies[i].name[j] = '\0';
        currencies[i].d_count = D_COUNT[i];
        currencies[i].denoms = DENOMS[i];
    }

    currencies[CUR_LOC].start_bal = 50000.0;
    currencies[CUR_USD].start_bal = 10000.0;
    currencies[CUR_EUR].start_bal = 8000.0;
    currencies[CUR_GBP].start_bal = 3000.0;
    currencies[CUR_JPY].start_bal = 1000000.0;

    for (int i = 0; i < MAX_CUR_LOCAL; ++i)
        currencies[i].bal = currencies[i].start_bal;

    currencies[CUR_LOC].critical_min = 10000.0;
    currencies[CUR_USD].critical_min = 2000.0;
    currencies[CUR_EUR].critical_min = 1500.0;
    currencies[CUR_GBP].critical_min = 500.0;
    currencies[CUR_JPY].critical_min = 200000.0;

    currencies[CUR_LOC].buy_to_loc = 1.0;   currencies[CUR_LOC].sell_to_loc = 1.0;
    currencies[CUR_USD].buy_to_loc = 41.36; currencies[CUR_USD].sell_to_loc = 41.45;
    currencies[CUR_EUR].buy_to_loc = 48.38; currencies[CUR_EUR].sell_to_loc = 48.60;
    currencies[CUR_GBP].buy_to_loc = 55.91; currencies[CUR_GBP].sell_to_loc = 56.26;
    currencies[CUR_JPY].buy_to_loc = 0.27;  currencies[CUR_JPY].sell_to_loc = 0.28;

    last_transaction_id = load_last_tx_id();
}

void make_daily_csv_name(const char *date_text, char *out, size_t cap) {
    snprintf(out, cap, "sales_%s.csv", date_text);
}

void generate_receipt(const Transaction *t, const char *date_text) {
    char fname[128];
    snprintf(fname, sizeof(fname), RECEIPT_FILE, date_text);

    FILE *f = fopen(fname, "a");
    if (!f) {
        fprintf(stderr, "Error opening receipt file: %s\n", fname);
        return;
    }

    fprintf(f, "\n========= CURRENCY EXCHANGE RECEIPT =========\n");
    fprintf(f, "Transaction ID: %d\n", t->id);
    fprintf(f, "Date: %s %s\n", t->date, t->time);
    fprintf(f, "From: %.2f %s\n", t->amount_from, CUR_NAME[t->from_cur]);
    fprintf(f, "To: %.2f %s\n", t->amount_to, CUR_NAME[t->to_cur]);
    fprintf(f, "Rate: 1 %s = %.4f %s\n", 
            CUR_NAME[t->from_cur], t->rate, CUR_NAME[t->to_cur]);
    fprintf(f, "==========================================\n\n");

    fclose(f);

    printf("\nReceipt generated and saved to %s\n", fname);
}

double csv_sum_profit_for_date(const char *date_text, int *tx_count_out) {
    char fname[128];
    make_daily_csv_name(date_text, fname, sizeof(fname));

    FILE *f = fopen(fname, "r");
    if (!f) {
        if (tx_count_out) *tx_count_out = 0;
        return 0.0;
    }

    char line[512];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        if (tx_count_out) *tx_count_out = 0;
        return 0.0;
    }

    double total_profit = 0.0;
    int count = 0;

    while (fgets(line, sizeof(line), f)) {
        char date[16], timebuf[16], fromc[32], toc[32];
        int txid = 0;
        double a_from=0, a_to=0, r_from=0, r_to=0, rem_loc=0, prof=0;
        int partial_flag=0;

        int n = sscanf(line, "%15[^,],%15[^,],%d,%31[^,],%31[^,],%lf,%lf,%lf,%lf,%d,%lf,%lf",
                       date, timebuf, &txid, fromc, toc,
                       &a_from, &a_to, &r_from, &r_to,
                       &partial_flag, &rem_loc, &prof);

        if (n == 12) {
            total_profit += prof;
            count++;
            continue;
        }

        n = sscanf(line, "%15[^,],%15[^,],%31[^,],%31[^,],%lf,%lf,%lf,%lf,%d,%lf,%lf",
                   date, timebuf, fromc, toc,
                   &a_from, &a_to, &r_from, &r_to,
                   &partial_flag, &rem_loc, &prof);
        if (n == 11) {
            total_profit += prof;
            count++;
            continue;
        }

    }
    fclose(f);

    if (tx_count_out) *tx_count_out = count;
    return total_profit;
}

double csv_sum_profit_for_month(const char *year_month, int *tx_count_out) {
    DIR *d = opendir(".");
    if (!d) {
        if (tx_count_out) *tx_count_out = 0;
        return 0.0;
    }

    char prefix[64];
    snprintf(prefix, sizeof(prefix), "sales_%s-", year_month); 
    size_t prefix_len = strlen(prefix);

    struct dirent *entry;
    double total_profit = 0.0;
    int count = 0;

    while ((entry = readdir(d)) != NULL) {
        const char *name = entry->d_name;
        if (strncmp(name, prefix, prefix_len) != 0) continue;
        size_t namelen = strlen(name);
        if (namelen < 5) continue;
        if (strcmp(name + namelen - 4, ".csv") != 0) continue;

        FILE *f = fopen(name, "r");
        if (!f) continue;

        char line[512];
        if (!fgets(line, sizeof(line), f)) { fclose(f); continue; }

        while (fgets(line, sizeof(line), f)) {
            char date[16], timebuf[16], fromc[32], toc[32];
            int txid = 0;
            double a_from=0, a_to=0, r_from=0, r_to=0, rem_loc=0, prof=0;
            int partial_flag=0;

            int n = sscanf(line, "%15[^,],%15[^,],%d,%31[^,],%31[^,],%lf,%lf,%lf,%lf,%d,%lf,%lf",
                           date, timebuf, &txid, fromc, toc,
                           &a_from, &a_to, &r_from, &r_to,
                           &partial_flag, &rem_loc, &prof);

            if (n == 12) {
                total_profit += prof;
                count++;
                continue;
            }

            n = sscanf(line, "%15[^,],%15[^,],%31[^,],%31[^,],%lf,%lf,%lf,%lf,%d,%lf,%lf",
                       date, timebuf, fromc, toc,
                       &a_from, &a_to, &r_from, &r_to,
                       &partial_flag, &rem_loc, &prof);
            if (n == 11) {
                total_profit += prof;
                count++;
                continue;
            }
        }
        fclose(f);
    }

    closedir(d);
    if (tx_count_out) *tx_count_out = count;
    return total_profit;
}

void ensure_csv_header(FILE *f) {
    long pos = ftell(f);
    if (pos == 0) {
        fprintf(f,
            "date,time,tx_id,from_currency,to_currency,amount_from,amount_to,rate_from_loc,rate_to_loc,partial,remainder_loc,profit_loc\n");
        fflush(f);
    }
}

void csv_log_transaction(
    const char *date_text,
    int tx_id,
    int from, int to,
    double amt_from, double amt_to,
    double rate_from_loc, double rate_to_loc,
    int partial, double remainder_loc_for_client,
    double profit_delta_loc
) {
    char fname[128];
    make_daily_csv_name(date_text, fname, sizeof(fname));

    FILE *f = fopen(fname, "a");
    if (!f) {
        fprintf(stderr, "CSV open failed (%s): %s\n", fname, strerror(errno));
        return;
    }
    ensure_csv_header(f);

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char timebuf[16];
    strftime(timebuf, sizeof(timebuf), "%H:%M:%S", tm_info);

    fprintf(f, "%s,%s,%d,%s,%s,%.6f,%.6f,%.6f,%.6f,%d,%.6f,%.6f\n",
        date_text, timebuf, tx_id,
        CUR_NAME[from], CUR_NAME[to],
        amt_from, amt_to,
        rate_from_loc, rate_to_loc,
        partial ? 1 : 0, remainder_loc_for_client,
        profit_delta_loc
    );
    fclose(f);
}

int csv_list_transactions_for_date(const char *date_text) {
    char fname[128];
    make_daily_csv_name(date_text, fname, sizeof(fname));
    FILE *f = fopen(fname, "r");
    if (!f) {
        fprintf(stderr, "Could not open %s for reading: %s\n", fname, strerror(errno));
        return -1;
    }

    char line[512];
    if (!fgets(line, sizeof(line), f)) { fclose(f); return 0; }

    printf("Transactions in %s:\n", fname);
    int printed = 0;
    while (fgets(line, sizeof(line), f)) {
        size_t L = strlen(line);
        while (L && (line[L-1] == '\n' || line[L-1] == '\r')) { line[--L] = '\0'; }
        if (L == 0) continue;
        printf("%s\n", line);
        printed++;
    }
    fclose(f);
    return printed;
}

int csv_find_transaction_by_id(const char *date_text, int tx_id) {
    char fname[128];
    make_daily_csv_name(date_text, fname, sizeof(fname));
    FILE *f = fopen(fname, "r");
    if (!f) {
        fprintf(stderr, "Could not open %s for searching: %s\n", fname, strerror(errno));
        return -1;
    }

    char line[512];
    if (!fgets(line, sizeof(line), f)) { fclose(f); return 0; }

    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        int id = 0;
        char date[16], timebuf[16], fromc[32], toc[32];
        double a_from=0, a_to=0, r_from=0, r_to=0, rem_loc=0, prof=0;
        int partial_flag=0;
        int n = sscanf(line, "%15[^,],%15[^,],%d,%31[^,],%31[^,],%lf,%lf,%lf,%lf,%d,%lf,%lf",
                       date, timebuf, &id, fromc, toc,
                       &a_from, &a_to, &r_from, &r_to,
                       &partial_flag, &rem_loc, &prof);
        if (n == 12) {
            if (id == tx_id) { printf("%s", line); found = 1; break; }
            continue;
        }
    }
    fclose(f);
    return found;
}

int csv_append_manual_transaction(const char *date_text, int tx_id,
                                  const char *time_text,
                                  const char *from_code, const char *to_code,
                                  double amt_from, double amt_to,
                                  double rate_from_loc, double rate_to_loc,
                                  int partial, double remainder_loc, double profit_loc_delta) {
    char fname[128];
    make_daily_csv_name(date_text, fname, sizeof(fname));
    FILE *f = fopen(fname, "a");
    if (!f) {
        fprintf(stderr, "Could not open %s for appending: %s\n", fname, strerror(errno));
        return -1;
    }
    ensure_csv_header(f);
    fprintf(f, "%s,%s,%d,%s,%s,%.6f,%.6f,%.6f,%.6f,%d,%.6f,%.6f\n",
            date_text, time_text, tx_id, from_code, to_code,
            amt_from, amt_to, rate_from_loc, rate_to_loc, partial ? 1 : 0, remainder_loc, profit_loc_delta);
    fclose(f);
    return 0;
}

void generate_daily_summary(const char *date_text) {
    int tx_count = 0;
    double total_profit = csv_sum_profit_for_date(date_text, &tx_count);

    char year_month[8+1];
    if (strlen(date_text) >= 7) {
        strncpy(year_month, date_text, 7);
        year_month[7] = '\0';
    } else {
        year_month[0] = '\0';
    }

    int month_tx_count = 0;
    double month_profit = 0.0;
    if (year_month[0]) {
        month_profit = csv_sum_profit_for_month(year_month, &month_tx_count);
    }

    double cashier_bonus = month_profit * 0.05; 

    printf("\n=== End-of-day report for %s ===\n", date_text);
    printf("Total Transactions: %d\n", tx_count);
    printf("Total Profit (LOC): %.6f\n", total_profit);
    printf("(Transactions are read from sales_%s.csv)\n", date_text);
    if (year_month[0]) {
        printf("Month-to-date Transactions (%s): %d\n", year_month, month_tx_count);
        printf("Month-to-date Profit (LOC): %.6f\n", month_profit);
        printf("Cashier monthly bonus (5%% of month profit): %.6f\n", cashier_bonus);
    }
    printf("\n");
    fflush(stdout);
}
