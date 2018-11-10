#include "main.h"
#include "comandos.h"
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

Editor editor;
aux temp; // estrutura auxiliar para validar login, e para a função shutdown()
int max_users;

void getMAX_USERS(int n) {
  srand(time(NULL));
  max_users = rand() % n;
}

bool find_username(char *username, char *filename) {
  FILE *f = fopen(filename, "r");
  char buffer[9];

  if (f == NULL) {
    printf("ERRO AO ABRIR FICHEIRO!\n");
    fclose(f);
    return false;
  }

  while (fscanf(f, "%8s", buffer) == 1) {
    if (strcmp(buffer, username) == 0) {
      fclose(f);
      return true;
    }
  }
  fclose(f);
  return false;
}

void verify_env_var() {
  // se a variável de ambiente não existe, então assumimos o valor por omissão
  // (=15 linhas)
  if (getenv("MEDIT_MAXLINES") == NULL)
    editor.lines = MAX_LINES;
  else {
    editor.lines = atoi(getenv("MEDIT_MAXLINES"));
  }
  // se a variável de ambiente não existe, então assumimos o valor por omissão
  // (=45 colunas)
  if (getenv("MEDIT_MAXCOLUMNS") == NULL)
    editor.columns = MAX_COLUMNS;
  else {
    editor.columns = atoi(getenv("MEDIT_MAXCOLUMNS"));
  }

  if (getenv("MEDIT_TIMEOUT") == NULL)
    editor.timeout = MEDIT_TIMEOUT;
  else {
    editor.timeout = atoi(getenv("MEDIT_TIMEOUT"));
  }
  if (getenv("MEDIT_MAXUSERS") == NULL)
    max_users = MEDIT_MAXUSERS;
  else {
    getMAX_USERS(editor.lines);
  }
  editor.screenrows = 0;
  editor.num_chars = 0;
  editor.cursor.x = 4;
  editor.cursor.y = 5;
}

void shutdown() {
  char pipe[20];
  int fd;

  sprintf(pipe, "../pipe-%d", temp.pid);
  fd = open(pipe, O_WRONLY);

  temp.action = 1;
  write(fd, &temp, sizeof(temp));
  printf("Programa terminado\n");
  close(fd);
  unlink(PIPE);
  exit(0);
}

void SIGhandler(int sig) {
  signal(sig, SIG_IGN);
  shutdown();
}

int main(int argc, char *argv[]) {
  char *file, comando[80], pipe[20];
  int opt, fd_pipe, n_named_pipes;
  // fd_pipe, filehandler para o pipe principal
  // opt, serve para ajudar a ler os argumentos opcionais da linha de comandos
  // n_named_pipes, numero de named pipes de interação(para
  // receberem ligações dos clientes)

  signal(SIGINT, SIGhandler);

  // saber se o admin enviou pela linha de comandos
  while ((opt = getopt(argc, argv, "f:p:n:")) != -1) {
    switch (opt) {
    case 'f':
      if (optarg) { // se existir argumento e se o ficheiro existir
        if (verify_file_existence(argv[2]) == 0)
          file = argv[2];
      } else {
        printf("Base de dados de usernames: ");
        scanf("%s", file);
        if (verify_file_existence(argv[2]) != 0)
          file = "../out/medit.db"; // valor por defeito!
      }
      break;
    case 'p':
      if (optarg)             // vai analisar se -p foi introduzido pelo user
        strcpy(pipe, optarg); // copia o valor do argumento para a variável pipe
      else { // senão existir argumento opcional, toma o valor por omissão
        strcpy(pipe, PIPE);
      }
      break;
    case 'n':
      if (optarg)
        n_named_pipes = atoi(argv[2]);
      else { // assumimos por defeito, o valor 3 (=MAXUSERS)
        n_named_pipes = MEDIT_MAXUSERS;
      }
    }
  }
  verify_env_var();

  if (access(PIPE, F_OK) == 0) { // verifica se já existe o PIPE
    printf("Já se encontra um servidor em execução. A sair...\n");
    exit(0);
  }
  if (mkfifo(PIPE, S_IRWXU) < 0) { // cria o pipe
    printf("Erro ao criar pipe. A sair...\n");
    exit(0);
  }

  // vai abrir o pipe para leitura/escrita
  fd_pipe = open(PIPE, O_RDWR);
  if (fd_pipe == -1) {
    printf("Erro ao abrir pipe. A sair...\n");
    exit(0);
  }
  printf("Servidor iniciado!\n");
  char client_fifo[50];
  int client_fd;
  read(fd_pipe, &temp, sizeof(temp));
  sprintf(client_fifo, "../pipe-%d", temp.pid);
  client_fd = open(client_fifo, O_WRONLY);
  if (find_username(temp.user, "../out/medit.db") == true) {
    write(client_fd, "Verificado!", strlen("Verificado"));
    printf("User %s iniciou sessao!\n", temp.user);
  } else {
    write(client_fd, "Nao encontrado!", strlen("Nao encontrado!"));
  }
close(fd_pipe);
  close(client_fd);
  unlink(PIPE);
  while (1) {
    scanf(" %79[^\n]s", comando);
    if (comando[strlen(comando) - 1] == '\n')
      comando[strlen(comando) - 1] = '\0';
    if (strcmp(comando, " ") != 0)
      cmd(comando);
    printf("Comando: %s\n", comando);
  }

  

  return 0;
}
