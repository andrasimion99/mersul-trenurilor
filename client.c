#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
extern int errno;

int port;
char auth[100];

int intarziere_sosire();
int intarziere_plecare();
int read_fct(int sd, char *msg, int dim);
int write_fct(int sd, char *msg, int dim);

int main(int argc, char *argv[])
{
  int sd;
  struct sockaddr_in server;
  char msg[100];
  char *mesaj;

  if (argc != 3)
  {
    printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
    return -1;
  }

  port = atoi(argv[2]);

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("Eroare la socket().\n");
    return errno;
  }

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr(argv[1]);
  server.sin_port = htons(port);

  if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[client]Eroare la connect().\n");
    return errno;
  }

  char alegere[20];
  int alegere_ok = 0;

  while (!alegere_ok)
  {
    printf("[client]Alegeti o optiune: \n1.Login \n2.Register\n");
    fflush(stdout);
    scanf("%100s", alegere);

    write_fct(sd, alegere, strlen(alegere));
    read_fct(sd, msg, 100);

    int username_ok = 0;

    if (strncmp(msg, "REGISTER PLEASE\n", 16) == 0)
    {
      alegere_ok = 1;
      char username[100];
      while (!username_ok)
      {
        printf("[client]Introduceti username: ");
        fflush(stdout);
        scanf("%100s", username);
        write_fct(sd, username, strlen(username));

        read(sd, msg, 100);
        if (strncmp(msg, "username exists", 15) == 0)
        {
          printf("Acest username este luat. Va rog introduceti altul.\n");
        }
        else if (strncmp(msg, "username ok", 11) == 0)
        {
          username_ok = 1;
          int password_ok = 0;
          printf("Username ok.\n");
          while (!password_ok)
          {
            char *password = getpass("[client]Introduceti password: ");
            char password1[20];
            strcpy(password1, password);
            password = getpass("[client]Confirmati parola: ");
            //in password se retine parola confirmata si in password1 prima parola introdusa
            if (strcmp(password, password1) == 0)
            {
              password_ok = 1;
              write_fct(sd, password, strlen(password));
              printf("Inregistrare cu succes!\n");
            }
            else
            {
              printf("Parolele nu se potrivesc. Incercati din nou!\n");
            }
          }
        }
      }
    }
    else if (strncmp(msg, "LOG IN PLEASE\n", 14) == 0)
    {
      alegere_ok = 1;
    }
    else if (strncmp(msg, "LOG IN PLEASE\n", 14) && strncmp(msg, "REGISTER PLEASE\n", 16))
    {
      printf("Alegere gresita!\n");
    }
  }

  while (1)
  {
    int ok = 0;
    read_fct(sd, msg, 100);

    if (strncmp(msg, "LOG IN PLEASE\n", 14) == 0)
    {
      char x[101], *info;

      printf("LOGIN PLEASE\n");
      fflush(stdout);
      printf("[client]Introduceti username: ");
      fflush(stdout);
      scanf("%100s", x);
      char *y = getpass("[client]Introduceti password: ");
      write_fct(sd, x, strlen(x));
      write_fct(sd, y, strlen(y));

      read_fct(sd, auth, 100);

      if (strncmp(auth, "autentificat", 12) == 0) //s-a autentificat
      {
        printf("autentificat\n");
        read_fct(sd, msg, 100);

        if (strncmp(msg, "calator", 7) == 0) // daca e calator
        {
          char statie_initiala[100], completare[100];
          while (1)
          {
            printf("In ce statie va aflati? \n");
            fflush(stdout);
            scanf("%100s", statie_initiala);
            gets(completare);
            strcat(statie_initiala, completare);
            write_fct(sd, statie_initiala, strlen(statie_initiala));
            char gara_ok[100];
            read_fct(sd, gara_ok, 100);
            if (strncmp(gara_ok, "gara ok", 7) == 0)
            {
              int lungime = 0;
              if (read(sd, &lungime, 4) < 0)
              {
                perror("[client]Eroare la read() de la server.\n");
                return errno;
              }
              info = (char *)calloc(lungime, sizeof(char *));
              read_fct(sd, info, lungime);
              printf("%s\n", info);
              printf("Incepeti prin a tasta \"meniu\"\n");

              while (1)
              {
                bzero(msg, 100);
                printf("[client]Dati o comanda: ");
                fflush(stdout);
                read(0, msg, 100);

                /* trimiterea mesajului la server */
                write_fct(sd, msg, strlen(msg));

                if (strncmp(msg, "exit", 4) == 0)
                {
                  ok = 1;
                  close(sd);
                  return 0;
                }
                else if (strncmp(msg, "intarzieri", 10) == 0) //daca vrea sa vada daca are intarzieri
                {
                  char rasp[100], nr_tren[100];
                  read_fct(sd, rasp, 100);

                  if (strncmp(rasp, "ok", 1) == 0)
                  {
                    printf("[client]Introduceti numarul trenului: ");
                    fflush(stdout);
                    scanf("%100s", nr_tren);

                    write_fct(sd, nr_tren, strlen(nr_tren));
                  }
                }
                else if (strncmp(msg, "statii", 6) == 0) //enumerare de statii
                {
                  char rasp[100], nr_tren[100];
                  read_fct(sd, rasp, 100);

                  if (strncmp(rasp, "ok", 1) == 0)
                  {
                    printf("[client]Introduceti numarul trenului: ");
                    fflush(stdout);
                    scanf("%100s", nr_tren);

                    write_fct(sd, nr_tren, strlen(nr_tren));
                  }
                }
                else if (strncmp(msg, "estimare sosire", 15) == 0) //ora de sosire in functie de intarzieri
                {
                  char rasp[100], nr_tren[100];
                  read_fct(sd, rasp, 100);

                  if (strncmp(rasp, "ok", 1) == 0)
                  {
                    printf("[client]Introduceti numarul trenului: ");
                    fflush(stdout);
                    scanf("%100s", nr_tren);

                    write_fct(sd, nr_tren, strlen(nr_tren));
                  }
                }
                else if (strncmp(msg, "cauta tren", 10) == 0) //daca cauta un traseu
                {
                  char rasp[100], statie_plecare[100], statie_sosire[100];
                  read_fct(sd, rasp, 100);

                  if (strncmp(rasp, "ok", 1) == 0)
                  {
                    printf("[client]Introduceti statia de plecare: ");
                    fflush(stdout);
                    scanf("%100s", statie_plecare);
                    gets(completare);
                    strcat(statie_plecare, completare);
                    printf("[client]Introduceti statia de sosire: ");
                    fflush(stdout);
                    scanf("%100s", statie_sosire);
                    gets(completare);
                    strcat(statie_sosire, completare);
                    write_fct(sd, statie_plecare, strlen(statie_plecare));
                    write_fct(sd, statie_sosire, strlen(statie_sosire));
                  }
                }
                else if (strncmp(msg, "plecari", 7) == 0) //plecari in urmatoarea ora
                {
                  char rasp[100], statie_req[100];
                  read_fct(sd, rasp, 100);

                  if (strncmp(rasp, "ok", 1) == 0)
                  {
                    printf("[client]Introduceti statia care va intereseaza: ");
                    fflush(stdout);
                    scanf("%100s", statie_req);
                    gets(completare);
                    strcat(statie_req, completare);

                    write_fct(sd, statie_req, strlen(statie_req));
                  }
                }
                else if (strncmp(msg, "sosiri", 6) == 0) //sosiri in urmatoarea ora
                {

                  char rasp[100], statie_req[100];
                  read_fct(sd, rasp, 100);

                  if (strncmp(rasp, "ok", 1) == 0)
                  {
                    printf("[client]Introduceti statia care va intereseaza: ");
                    fflush(stdout);
                    scanf("%100s", statie_req);
                    gets(completare);
                    strcat(statie_req, completare);
                    write_fct(sd, statie_req, strlen(statie_req));
                  }
                }

                /* citirea raspunsului dat de server (apel blocant pina cind serverul raspunde) */
                lungime = 0;
                if (read(sd, &lungime, 4) < 0)
                {
                  perror("[client]Eroare la read() de la server.\n");
                  return errno;
                }
                //printf("%d\n",lungime);
                mesaj = (char *)calloc(lungime, sizeof(char *));
                
                read_fct(sd, mesaj, lungime);
                //printf("%ld\n",strlen(mesaj));
                printf("[client]Mesajul primit este: %s\n", mesaj);
                free(mesaj);
              }
              if (!ok)
              {
                close(sd);
              }
            }
            else
            {
              printf("Statia introdusa este incorecta. Va rog introduceti din nou.\n");
            }
          }
        }
        else if (strncmp(msg, "panou", 5) == 0) // daca clientul este panoul
        {
          int lungime = 0;
          if (read(sd, &lungime, 4) < 0)
          {
            perror("[client]Eroare la read() de la server.\n");
            return errno;
          }
          info = (char *)calloc(lungime, sizeof(char *));
          read_fct(sd, info, lungime);
          printf("%s\n", info);

          while (1)
          {
            if (read(sd, &lungime, 4) < 0)
            {
              perror("[client]Eroare la read() de la server.\n");
              return errno;
            }
            info = (char *)calloc(lungime, sizeof(char *));
            read_fct(sd, info, lungime);
            if (strncmp(info, "Mersul Trenurilor\n", 18) == 0)
            {
              system("clear");
              printf("%s\n", info);
            }
            else
            {
              break;
            }
            sleep(1);
          }
          close(sd);
          exit(1);
        }
        else if (strncmp(msg, "tren", 4) == 0) // daca clientul este tren
        {
          int late = 0, new_value = 0, nr_statii = 0;

          if (read(sd, &nr_statii, 4) < 0)
          {
            perror("[client]Eroare la read() de la server.\n");
            return errno;
          }

          for (int i = 1; i <= nr_statii; i++)
          {
            int intarziere_ora_sosire = intarziere_sosire();
            if (intarziere_ora_sosire >= 0)
            {
              printf("Intarziere la sosire in statia %d cu %d minute fata de noua estimare.\n", i, intarziere_ora_sosire);
            }
            if (intarziere_ora_sosire < 0)
            {
              printf("Ajunge in statia %d cu %d minute mai devreme fata de noua estimare.\n", i, -intarziere_ora_sosire);
            }

            if (write(sd, &intarziere_ora_sosire, 4) <= 0)
            {
              perror("[client]Eroare la write() spre server.\n");
              return errno;
            }
            printf("Trenul asteapta 5 secunde in statia %d.\n", i);
            sleep(5); // cat sta in statie
            int intarziere_ora_plecare = intarziere_plecare();
            if (intarziere_ora_plecare >= 0)
            {
              printf("Intarziere la plecare din statia %d cu %d minute fata de noua estimare.\n", i, intarziere_ora_plecare);
            }
            if (write(sd, &intarziere_ora_plecare, 4) <= 0)
            {
              perror("[client]Eroare la write() spre server.\n");
              return errno;
            }
            if (i + 1 > nr_statii)
            {
              printf("Trenul a ajuns la destinatie!!\n");
            }
            else
            {
              printf("Trenul se deplaseaza spre statia %d timp de 30 de secunde.\n\n", i + 1);
              sleep(30); // pana ajunge in urmatoarea statie
            }
          }
          close(sd);
          exit(1);
        }
      }
      else
      {
        printf("Failed login. Wrong username or password. Please try again.\n");
      }
    }
  }
}

int intarziere_sosire()
{
  int d;
  if (rand() % 2)
  {
    d = (rand() % 6);
    if (rand() % 2)
    {
      d = -d;
    }
    return d;
  }
  else
  {
    return 0;
  }
}

int intarziere_plecare()
{
  int d;
  if (rand() % 2)
  {
    return (rand() % 6);
  }
  else
  {
    return 0;
  }
}

int read_fct(int sd, char *msg, int dim)
{
  bzero(msg, dim);
  if (read(sd, msg, dim) < 0)
  {
    perror("[client]Eroare la read() de la server.\n");
    return errno;
  }
}

int write_fct(int sd, char *msg, int dim)
{
  if (write(sd, msg, dim) <= 0)
  {
    perror("[client]Eroare la write() spre server.\n");
    return errno;
  }
}