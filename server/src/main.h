#include "estruturas.h"

extern Editor editor;
extern int active_users;

int main(int argc, char *argv[]);
void getMAX_USERS(int n);
bool find_username(char *username, char *filename);
void verify_env_var();
void SIGhandler(int sig);
void shutdown();
void *receiver();
bool check_if_users_exceeds_max_active();
