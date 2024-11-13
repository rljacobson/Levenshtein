#pragma once


#include <mysql.h>

// Declare all UDF functions with C linkage
extern "C" {
// Levenshtein

int lev_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
long long lev(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void lev_deinit(UDF_INIT *initid);

int levlim_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
long long levlim(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void levlim_deinit(UDF_INIT *initid);

int levlimopt_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
long long levlimopt(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void levlimopt_deinit(UDF_INIT *initid);


// Damerau–Levenshtein

int damlev2D_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
long long damlev2D(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void damlev2D_deinit(UDF_INIT *initid);

int damlev_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
long long damlev(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void damlev_deinit(UDF_INIT *initid);

int damlevlim_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
long long damlevlim(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void damlevlim_deinit(UDF_INIT *initid);

int damlevmin_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
long long damlevmin(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void damlevmin_deinit(UDF_INIT *initid);

int damlevminp_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
long long damlevminp(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void damlevminp_deinit(UDF_INIT *initid);

int damlevp_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
long long damlevp(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void damlevp_deinit(UDF_INIT *initid);

int noop_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
long long noop(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void noop_deinit(UDF_INIT *initid);
}
