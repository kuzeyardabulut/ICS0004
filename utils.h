#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdio.h>

#define MAX_CUR 5
#define MAX_NAME 8
#define BUF 256

enum { CUR_LOC = 0, CUR_USD = 1, CUR_EUR = 2, CUR_GBP = 3, CUR_JPY = 4 };

typedef struct {
    int id;
    char date[11];
    char time[9];
    int from_cur;
    int to_cur;
    double amount_from;
    double amount_to;
    double rate;
    double profit;
} Transaction;

typedef struct {
    char name[MAX_NAME];
    int d_count;
    const int *denoms;
    double start_bal;
    double bal;
    double critical_min;
    double buy_to_loc;
    double sell_to_loc;
} Currency;

/* Externs for globals defined in utils.c */
extern Currency *currencies;
extern const char *CUR_NAME[5];
extern const int *DENOMS[5];
extern const int D_COUNT[5];
extern double profit_loc;
extern char current_date[64];
extern int last_transaction_id;

/* Initialization */
void init_defaults(void);

/* Input helpers */
int ask_int(const char *prompt, int min, int max);
double ask_double(const char *prompt, double min, double max);
void clear_input(void);

/* CSV and receipt helpers */
void make_daily_csv_name(const char *date_text, char *out, size_t cap);
void generate_receipt(const Transaction *t, const char *date_text);
void generate_daily_summary(const char *date_text);
double csv_sum_profit_for_date(const char *date_text, int *tx_count_out);
void ensure_csv_header(FILE *f);
void csv_log_transaction(const char *date_text, int tx_id, int from, int to,
                         double amt_from, double amt_to, double rate_from_loc, double rate_to_loc,
                         int partial, double remainder_loc_for_client, double profit_delta_loc);

int csv_list_transactions_for_date(const char *date_text);
int csv_find_transaction_by_id(const char *date_text, int tx_id);
int csv_append_manual_transaction(const char *date_text, int tx_id,
                                  const char *time_text,
                                  const char *from_code, const char *to_code,
                                  double amt_from, double amt_to,
                                  double rate_from_loc, double rate_to_loc,
                                  int partial, double remainder_loc, double profit_loc_delta);

void save_last_tx_id(int id);
int load_last_tx_id(void);

/* Denominations */
extern const int DENOMS_LOC[];
extern const int DENOMS_USD[];
extern const int DENOMS_EUR[];
extern const int DENOMS_GBP[];
extern const int DENOMS_JPY[];

#endif /* UTILS_H */
