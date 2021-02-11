#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#define PORT 4444

extern int errno;
int c, auth = 0, Register = 0, username_ok = 0;
char numar_tren[10];
int nr_gasit = 0, gara_gasita = 0, numar_copii = 0;

int verify_auth(xmlNode *a_node, char x[101], char y[101], int *read, int *write);
int verify_username(xmlNode *a_node, char x[101]);
void update_user(xmlNode *a_node, char username[100], char password[100], xmlDoc *document);
int verify_gara(xmlNode *a_node, char gara[101]);
void verif_statie(xmlNode *a_node, char statie_initiala[100], char response[1000], xmlNode *root);
char *mersul_trenurilor(xmlNode *a_node, char numar_tren[100], char response[1000]);
int getHour();
int getMinutes();
void cautaTren(xmlNode *a_node, char statie_plecare[100], char statie_sosire[100], char response[1000], xmlNode *root);
void create_response(xmlNode *node, char response[1000]);
void parcurgere_nr_tren(xmlNode *a_node, char nr_tren[10], char statie_plecare[100], char statie_sosire[100], char response[1000]);
void infoTrenIntarzieri(xmlNode *a_node, char nr_tren[100], char response[100]);
void infoEstimareSosire(xmlNode *a_node, char numar_tren[10], char statie_initiala[100], char response[100]);
void infoPlecari(xmlNode *a_node, char statie_req[100], char response[1000], xmlNode *root);
void parcurgere_nr_tren_plecari(xmlNode *a_node, char numar_tren[10], char statie_req[100], char response[1000]);
void infoSosiri(xmlNode *a_node, char statie_req[100], char response[1000], xmlNode *root);
void parcurgere_nr_tren_sosiri(xmlNode *a_node, char numar_tren[10], char statie_req[100], char response[1000]);
int updatedData(char *filename, time_t oldMTime);
int nr_statii_total(xmlNode *a_node, char numar_tren[100]);
void statii_tren(xmlNode *a_node, char numar_tren[100], char response[1000]);
void updateIntarzieri_sosire(xmlNode *a_node, char numar_tren[100], int numar_statie, int intarziere_statie, xmlDoc *document, xmlNode *root);
void updateIntarzieri_plecare(xmlNode *a_node, char numar_tren[100], int numar_statie, int intarziere_statie, xmlDoc *document, xmlNode *root);
void change_intarzieri_total(xmlNode *a_node, char numar_tren[100], xmlDoc *document, char char_intarziere_statie[10]);
void read_fct(int sd, char msg[1000], int dim);
void write_fct(int sd, char msg[1000], int dim);

void sighandler(int sig) /* functia de tratare a semnalului */
{
    wait(&c);
    //printf("COPIL SEMNAL\n");
    fflush(stdout);
}

int main()
{
    struct sockaddr_in server;
    struct sockaddr_in from;    
    char msg[100];
    char msgrasp[1000] = " ";
    int sd;
    pid_t pid1, pid2;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[server]Eroare la socket().\n");
        return errno;
    }
    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[server]Eroare la bind().\n");
        return errno;
    }

    if (listen(sd, 5) == -1)
    {
        perror("[server]Eroare la listen().\n");
        return errno;
    }

    while (1)
    {
        int client;
        unsigned int length = sizeof(from);

        printf("[server]Asteptam la portul %d...\n", PORT);
        fflush(stdout);

        client = accept(sd, (struct sockaddr *)&from, &length);
        if (client < 0)
        {
            perror("[server]Eroare la accept().\n");
            continue;
        }

        pid1 = fork();
        if (pid1 == -1)
        {
            printf("eroare la fork");
            continue;
        }
        else if (pid1) //parinte
        {
            signal(SIGCHLD, sighandler);
        }
        else //copil
        {
            char alegere[20];
            int alegere_ok = 0;
            while (!alegere_ok)
            {
                read_fct(client, alegere, 100);

                if (strncmp(alegere, "Register", 8) == 0) //REGISTER
                {
                    alegere_ok = 1;
                    write_fct(client, "REGISTER PLEASE\n", strlen("REGISTER PLEASE\n"));

                    char username[100], password[100];
                    while (!username_ok)
                    {
                        read_fct(client, username, 100);

                        xmlDoc *document3;
                        xmlNode *root3;
                        char *filename3;
                        filename3 = "./clients.xml";

                        document3 = xmlReadFile(filename3, NULL, 0);
                        root3 = xmlDocGetRootElement(document3);
                        Register = 0;
                        if (verify_username(root3, username)) //inseamna ca mai exista acel username
                        {
                            write_fct(client, "username exists", 100);
                        }
                        else //inseamna ca usernameul este ok
                        {
                            username_ok = 1;
                            write_fct(client, "username ok", strlen("username ok"));
                            read_fct(client, password, 100);
                            document3 = xmlReadFile(filename3, NULL, 0);
                            root3 = xmlDocGetRootElement(document3);
                            update_user(root3, username, password, document3);
                        }
                    }
                }
                else if (strncmp(alegere, "Login", 5) == 0)
                {
                    alegere_ok = 1;
                    write_fct(client, "LOG IN PLEASE\n", 100);
                }
                else
                {
                    write_fct(client, "alegere gresita", 100);
                }
            }

            int ok = 0;
            while (!ok) //cat timp nu e autentificat
            {
                write_fct(client, "LOG IN PLEASE\n", 100);

                xmlDoc *document1;
                xmlNode *root1;
                char *filename1;
                char x[101], y[101];
                int Read_var = 0;
                int Write_var = 0;

                filename1 = "./clients.xml";

                document1 = xmlReadFile(filename1, NULL, 0);
                root1 = xmlDocGetRootElement(document1);

                read_fct(client, x, 100);
                read_fct(client, y, 100);

                if (verify_auth(root1, x, y, &Read_var, &Write_var)) //s-a autentificat
                {
                    ok = 1;
                    printf("\nx: %s y: %s read_var: %d write_var: %d\n\n", x, y, Read_var, Write_var);
                    xmlFreeDoc(document1);
                    xmlCleanupParser();
                    write_fct(client, "autentificat", 100);

                    //verificam ce fel de autentificare a avut loc si trimitem la client
                    if (Read_var == 1 && Write_var == 1) //daca e calator
                    {
                        char statie_initiala[100];
                        write_fct(client, "calator", 100);
                        int var_gara = 0;
                        while (!var_gara)
                        {
                            read_fct(client, statie_initiala, 100);

                            xmlDoc *document2;
                            xmlNode *root2;
                            char *filename2;

                            filename2 = "./gari.xml";

                            document2 = xmlReadFile(filename2, NULL, 0);
                            root2 = xmlDocGetRootElement(document2);

                            if (verify_gara(root2, statie_initiala))
                            {
                                var_gara = 1;
                                xmlFreeDoc(document2);
                                xmlCleanupParser();
                                write_fct(client, "gara ok", 100);

                                xmlDoc *document;
                                xmlNode *root;
                                char *filename;

                                filename = "./trenuri.xml";

                                document = xmlReadFile(filename, NULL, 0);
                                root = xmlDocGetRootElement(document);

                                char resp[1001] = "Mersul Trenurilor\n";

                                verif_statie(root, statie_initiala, resp, root);
                                int lungime = strlen(resp);
                                if (write(client, &lungime, 4) <= 0)
                                {
                                    perror("[server]Eroare la write() catre client.\n");
                                    break; /* continuam sa ascultam */
                                }
                                write_fct(client, resp, lungime);

                                xmlFreeDoc(document);
                                xmlCleanupParser();

                                printf("[server]Asteptam mesajul...\n");
                                fflush(stdout);
                                pid2 = fork();
                                if (pid2 == -1)
                                {
                                    printf("eroare la fork");
                                    continue;
                                }
                                else if (pid2) //parinte
                                {
                                    signal(SIGCHLD, sighandler);
                                }
                                else //copil
                                {
                                    while (1)
                                    {
                                        /* citirea mesajului */
                                        read_fct(client, msg, 100);

                                        printf("[server]Mesajul a fost receptionat...%s\n", msg);

                                        if (strncmp(msg, "intarzieri", strlen(msg) - 1) == 0) // intarzieri pentru un tren anume
                                        {
                                            char nr_tren[100];

                                            write_fct(client, "ok", 100);

                                            read_fct(client, nr_tren, 100);

                                            document = xmlReadFile(filename, NULL, 0);
                                            root = xmlDocGetRootElement(document);

                                            bzero(msgrasp, 100);
                                            infoTrenIntarzieri(root, nr_tren, msgrasp);

                                            if (strncmp(msgrasp, "Trenul", 6))
                                            {
                                                strcat(msgrasp, "Nu exista un tren cu numarul introdus.\n Va rugam apelati din nou comanda \"intarzieri\".\n");
                                            }
                                        }
                                        else if (strncmp(msg, "meniu", strlen(msg) - 1) == 0)
                                        {
                                            bzero(msgrasp, 100);
                                            strcat(msgrasp, "Comenzi acceptate\n1.intarzieri\n2.estimare sosire\n3.cauta tren\n4.plecari\n5.sosiri\n6.statii\n7.meniu\n8.exit\n");
                                        }
                                        else if (strncmp(msg, "statii", strlen(msg) - 1) == 0) //enumerare de statii
                                        {
                                            char nr_tren[100];

                                            write_fct(client, "ok", 100);

                                            read_fct(client, nr_tren, 100);

                                            document = xmlReadFile(filename, NULL, 0);
                                            root = xmlDocGetRootElement(document);

                                            bzero(msgrasp, 100);
                                            statii_tren(root, nr_tren, msgrasp);
                                            if (strlen(msgrasp) == 0)
                                            {
                                                strcat(msgrasp, "Nu exista un tren cu numarul introdus.\n");
                                            }
                                        }
                                        else if (strncmp(msg, "estimare sosire", strlen(msg) - 1) == 0) //estimare sosire pentru statia initiala
                                        {
                                            char nr_tren[100];

                                            write_fct(client, "ok", 100);

                                            read_fct(client, nr_tren, 100);

                                            document = xmlReadFile(filename, NULL, 0);
                                            root = xmlDocGetRootElement(document);

                                            bzero(msgrasp, 100);
                                            infoEstimareSosire(root, nr_tren, statie_initiala, msgrasp);
                                            if (strncmp(msgrasp, "Trenul", 6))
                                            {
                                                strcat(msgrasp, "Nu exista un tren cu numarul introdus sau trenul nu trece prin statia dumneavoastra.\n");
                                            }
                                        }
                                        else if (strncmp(msg, "cauta tren", strlen(msg) - 1) == 0) //cauta un traseu statia1->statia2
                                        {
                                            char statie_plecare[100], statie_sosire[100];
                                            write_fct(client, "ok", 100);

                                            read_fct(client, statie_plecare, 100);
                                            read_fct(client, statie_sosire, 100);

                                            document2 = xmlReadFile(filename2, NULL, 0);
                                            root2 = xmlDocGetRootElement(document2);

                                            gara_gasita = 0;
                                            int verif_statie_plecare = verify_gara(root2, statie_plecare);
                                            gara_gasita = 0;
                                            int verif_statie_sosire = verify_gara(root2, statie_sosire);

                                            if (verif_statie_plecare && verif_statie_sosire)
                                            {
                                                document = xmlReadFile(filename, NULL, 0);
                                                root = xmlDocGetRootElement(document);

                                                bzero(msgrasp, 1000);
                                                cautaTren(root, statie_plecare, statie_sosire, msgrasp, root);

                                                if (strncmp(msgrasp, "\nTrenul", 7))
                                                {
                                                    strcat(msgrasp, "HEY NU AI TREN pe ruta asta! Incearca din nou comanda cauta tren.\n");
                                                }
                                            }
                                            else
                                            {
                                                bzero(msgrasp, 1000);
                                                strcat(msgrasp, "Nu exista statiile introduse.\n");
                                            }
                                        }
                                        else if (strncmp(msg, "plecari", strlen(msg) - 1) == 0) //plecari in urmatoarea ora pt o statie
                                        {
                                            char statie_req[100];
                                            write_fct(client, "ok", 100);
                                            read_fct(client, statie_req, 100);

                                            document2 = xmlReadFile(filename2, NULL, 0);
                                            root2 = xmlDocGetRootElement(document2);

                                            gara_gasita = 0;
                                            int verif_statie_req = verify_gara(root2, statie_req);

                                            if (verif_statie_req)
                                            {
                                                document = xmlReadFile(filename, NULL, 0);
                                                root = xmlDocGetRootElement(document);

                                                bzero(msgrasp, 1000);
                                                infoPlecari(root, statie_req, msgrasp, root);

                                                if (strncmp(msgrasp, "\nTrenul", 7))
                                                {
                                                    strcat(msgrasp, "Nu aveti plecari in urmatoarea ora in statia respectiva.\n");
                                                }
                                            }
                                            else
                                            {
                                                bzero(msgrasp, 1000);
                                                strcat(msgrasp, "Nu exista statia introdusa.\n");
                                            }
                                        }
                                        else if (strncmp(msg, "sosiri", strlen(msg) - 1) == 0) //sosiri in urmatoarea ora pt o statie
                                        {
                                            char statie_req[100];
                                            write_fct(client, "ok", 100);
                                            read_fct(client, statie_req, 100);

                                            document2 = xmlReadFile(filename2, NULL, 0);
                                            root2 = xmlDocGetRootElement(document2);

                                            gara_gasita = 0;
                                            int verif_statie_req = verify_gara(root2, statie_req);

                                            if (verif_statie_req)
                                            {
                                                document = xmlReadFile(filename, NULL, 0);
                                                root = xmlDocGetRootElement(document);

                                                bzero(msgrasp, 1000);
                                                infoSosiri(root, statie_req, msgrasp, root);

                                                if (strncmp(msgrasp, "\nTrenul", 7))
                                                {
                                                    strcat(msgrasp, "Nu aveti sosiri in urmatoarea ora in statia respectiva.\n");
                                                }
                                            }
                                            else
                                            {
                                                bzero(msgrasp, 1000);
                                                strcat(msgrasp, "Nu exista statia introdusa.\n");
                                            }
                                        }
                                        else
                                        {
                                            bzero(msgrasp, 100);
                                            strcat(msgrasp, "Nu exista comanda. Incercati din nou una din comenzile din \"meniu\".\n");
                                        }

                                        printf("[server]Trimitem mesajul inapoi...%s\n", msgrasp);

                                        // /* returnam mesajul clientului */
                                        lungime = strlen(msgrasp);
                                       // printf("%d\n", lungime);
                                        if (write(client, &lungime, 4) <= 0)
                                        {
                                            perror("[server]Eroare la write() catre client.\n");
                                            break; /* continuam sa ascultam */
                                        }
                                        write_fct(client, msgrasp, lungime);
                                    }
                                    /* am terminat cu acest client, inchidem conexiunea */
                                    close(client);
                                    exit(2);
                                }
                            }
                            else
                            {
                                write_fct(client, "Statie incorecta", 100);
                            }
                        }
                    }
                    else if (Read_var == 1 && Write_var == 0) //daca e panou
                    {
                        write_fct(client, "panou", 100);

                        xmlDoc *document;
                        xmlNode *root;
                        char *filename;

                        filename = "./trenuri.xml";

                        struct stat fileStat;
                        if (stat(filename, &fileStat) < 0)
                        {
                            perror("\neroare la gasirea informatiilor la stat");
                        }
                        time_t oldMTime = fileStat.st_mtime;

                        document = xmlReadFile(filename, NULL, 0);
                        root = xmlDocGetRootElement(document);

                        char resp[1001] = "Mersul Trenurilor\n";

                        verif_statie(root, y + 6, resp, root); // y = panou_Iasi....x+6 = Iasi
                        int lungime = strlen(resp);
                        if (write(client, &lungime, 4) <= 0)
                        {
                            perror("[server]Eroare la write() catre client.\n");
                            break; /* continuam sa ascultam */
                        }
                        write_fct(client, resp, lungime);

                        xmlFreeDoc(document);
                        xmlCleanupParser();
                        while (1)
                        {
                            document = xmlReadFile(filename, NULL, 0);
                            root = xmlDocGetRootElement(document);
                            if (updatedData(filename, oldMTime))
                            {
                                if (stat(filename, &fileStat) < 0)
                                {
                                    perror("\neroare la gasirea informatiilor la stat");
                                }
                                oldMTime = fileStat.st_mtime;
                                bzero(resp, 1001);
                                strcpy(resp, "Mersul Trenurilor\n");
                                verif_statie(root, x + 6, resp, root);
                                lungime = strlen(resp);
                                if (write(client, &lungime, 4) <= 0)
                                {
                                    perror("[server]Eroare la write() catre client.\n");
                                    break; /* continuam sa ascultam */
                                }
                                write_fct(client, resp, lungime);
                            }
                            if (kill(getpid() - 1, 0))
                            {
                                close(client);
                                kill(getpid(), SIGKILL);
                            }
                            sleep(1);
                        }
                    }
                    else if (Read_var == 0 && Write_var == 1) // daca e tren
                    {
                        int intarziere1 = 0, intarziere2 = 0;
                        int nr_statii = 0;
                        write_fct(client, "tren", 100);

                        xmlDoc *document;
                        xmlNode *root;
                        char *filename;

                        filename = "./trenuri.xml";

                        document = xmlReadFile(filename, NULL, 0);
                        root = xmlDocGetRootElement(document);

                        nr_statii = nr_statii_total(root, x + 5);

                        if (write(client, &nr_statii, 4) <= 0)
                        {
                            perror("[server]Eroare la write() catre client.\n");
                            break; /* continuam sa ascultam */
                        }

                        for (int i = 1; i <= nr_statii; i++)
                        {
                            if (read(client, &intarziere1, 4) <= 0)
                            {
                                close(client);
                                exit(2);
                                perror("[server]Eroare la read() de la client.\n");
                            }

                            printf("intarziere la sosire: %d pt statia %d\n", intarziere1, i);

                            document = xmlReadFile(filename, NULL, 0);
                            root = xmlDocGetRootElement(document);

                            updateIntarzieri_sosire(root, x + 5, i, intarziere1, document, root); //x+5 e numarul trenului deoarece x = tren_1833

                            if (read(client, &intarziere2, 4) <= 0)
                            {
                                close(client);
                                exit(2);
                                perror("[server]Eroare la read() de la client.\n");
                            }

                            printf("intarziere la plecare: %d pt statia %d\n", intarziere2, i);

                            document = xmlReadFile(filename, NULL, 0);
                            root = xmlDocGetRootElement(document);

                            updateIntarzieri_plecare(root, x + 5, i, intarziere2, document, root);
                        }
                        close(client);
                        exit(1);
                    }
                }
                else
                {
                    printf("\nx: %s y: %s read_var: %d write_var: %d\n\n", x, y, Read_var, Write_var); //login fail
                    write_fct(client, "failed", 100);
                }
            }
            fflush(stdout);
            exit(1);
        }

    } /* while */
} /* main */

char *mersul_trenurilor(xmlNode *a_node, char numar_tren[100], char response[1000])
{
    xmlNode *node = NULL;

    for (node = a_node; node; node = node->next)
    {
        //printf("aici %s\n", node->name);
        if (node->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(node) == 0)
            {
                if (nr_gasit == 1)
                {
                    if (strcmp((const char *)node->name, "statie"))
                    {
                        strcat(response, (const char *)node->name);
                        strcat(response, " -> ");
                        strcat(response, (const char *)xmlNodeGetContent(node));
                        strcat(response, "\n");
                    }
                }
            }
            else
            {
                if (strcmp((const char *)node->name, "tren") == 0)
                {
                    if (strcmp((const char *)xmlGetProp(node, (const xmlChar *)"Numar"), numar_tren) == 0)
                    {
                        nr_gasit = 1;
                        strcat(response, "\nTrenul ");
                        strcat(response, (const char *)xmlGetProp(node, (const xmlChar *)"Numar"));
                        strcat(response, "\n--------------------\n");
                    }
                    else
                    {
                        nr_gasit = 0;
                    }
                }
            }
        }
        mersul_trenurilor(node->children, numar_tren, response);
    }
    return response;
}

void verif_statie(xmlNode *a_node, char statie_initiala[100], char response[1000], xmlNode *root)
{
    xmlNode *node = NULL;
    for (node = a_node; node; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(node) == 0)
            {
                if (strcmp((const char *)node->name, "statie") == 0 && strcmp((const char *)xmlGetProp(node, (const xmlChar *)"numeStatie"), statie_initiala) == 0)
                {
                    strcpy(response, mersul_trenurilor(root, numar_tren, response));
                }
            }
            else
            {
                if (strcmp((const char *)node->name, "tren") == 0)
                {
                    strcpy(numar_tren, (const char *)xmlGetProp(node, (const xmlChar *)"Numar"));
                }
            }
        }
        verif_statie(node->children, statie_initiala, response, root);
    }
}

int getHour()
{
    time_t now;
    struct tm *now_tm;
    int hour;
    // int min;

    now = time(NULL);
    now_tm = localtime(&now);
    hour = now_tm->tm_hour;
    //min = now_tm->tm_min;
    return hour;
    // sprintf(Time_now, "%d:%d", hour, min);
}

int getMinutes()
{
    time_t now;
    struct tm *now_tm;
    int min;

    now = time(NULL);
    now_tm = localtime(&now);
    min = now_tm->tm_min;
    return min;
}

int verify_auth(xmlNode *a_node, char x[101], char y[101], int *read, int *write)
{
    xmlNode *node = NULL;

    for (node = a_node; node; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(node) == 0)
            {
                xmlNode *sibling_node = xmlNextElementSibling(node);
                xmlNode *next_sibling_node = xmlNextElementSibling(sibling_node);

                xmlSetProp(node, node->name, xmlNodeGetContent(node->next));
                //printf("name: %s with: %s\n", node->name, xmlNodeGetContent(node));

                if (strcmp((const char *)node->name, "user") == 0 && strcmp((const char *)xmlNodeGetContent(node), x) == 0 && strcmp((const char *)sibling_node->name, "password") == 0 && strcmp((const char *)xmlNodeGetContent(sibling_node), y) == 0)
                {
                    auth = 1;
                    if (strcmp((const char *)next_sibling_node->name, "drepturi") == 0)
                    {
                        if (strcmp((const char *)xmlNodeGetContent(next_sibling_node), "R,W") == 0)
                        {
                            *read = 1;
                            *write = 1;
                        }
                        else if (strcmp((const char *)xmlNodeGetContent(next_sibling_node), "R") == 0)
                        {
                            *read = 1;
                            *write = 0;
                        }
                        else if (strcmp((const char *)xmlNodeGetContent(next_sibling_node), "W") == 0)
                        {
                            *read = 0;
                            *write = 1;
                        }
                    }
                }
            }
            // else
            // {
            //   printf("name: %s are %li copii\n", node->name, xmlChildElementCount(node));
            // }
        }
        verify_auth(node->children, x, y, read, write);
    }
    return auth;
}

int verify_username(xmlNode *a_node, char x[101])
{
    xmlNode *node = NULL;

    for (node = a_node; node; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(node) == 0)
            {
                if (strcmp((const char *)node->name, "user") == 0 && strcmp((const char *)xmlNodeGetContent(node), x) == 0)
                {
                    Register = 1;
                }
            }
        }
        verify_username(node->children, x);
    }
    return Register;
}

void update_user(xmlNode *a_node, char username[100], char password[100], xmlDoc *document)
{
    xmlNode *node = NULL;

    for (node = a_node; node; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(node))
            {
                if (strcmp((const char *)node->name, "clients") == 0)
                {
                    xmlNodePtr clientChild = xmlNewChild(node, NULL, (xmlChar *)"client", NULL);
                    xmlNewChild(clientChild, NULL, (xmlChar *)"user", (const xmlChar *)username);
                    xmlNewChild(clientChild, NULL, (xmlChar *)"password", (const xmlChar *)password);
                    xmlNewChild(clientChild, NULL, (xmlChar *)"drepturi", (const xmlChar *)"R,W");
                    xmlSaveFile("./clients.xml", document);
                    break;
                }
            }
        }
        update_user(node->children, username, password, document);
    }
}

int verify_gara(xmlNode *a_node, char gara[101])
{
    xmlNode *node = NULL;

    for (node = a_node; node; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(node) == 0)
            {
                if (strcmp((const char *)node->name, "gara") == 0 && strcmp((const char *)xmlNodeGetContent(node), gara) == 0)
                {
                    gara_gasita = 1;
                }
            }
        }
        verify_gara(node->children, gara);
    }
    return gara_gasita;
}

void cautaTren(xmlNode *a_node, char statie_plecare[100], char statie_sosire[100], char response[1000], xmlNode *root)
{
    xmlNode *node = NULL;
    int statieP, statieS;
    for (node = a_node; node; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(node) == 0)
            {
                if (strcmp((const char *)node->name, "statie") == 0 && strcmp((const char *)xmlGetProp(node, (const xmlChar *)"numeStatie"), statie_plecare) == 0)
                {
                    statieP = 1;
                }
                if (strcmp((const char *)node->name, "statie") == 0 && strcmp((const char *)xmlGetProp(node, (const xmlChar *)"numeStatie"), statie_sosire) == 0)
                {
                    statieS = 1;
                    if (statieP == 1 && statieS == 1)
                    {
                        parcurgere_nr_tren(root, numar_tren, statie_plecare, statie_sosire, response);
                    }
                }
            }
            else
            {
                if (strcmp((const char *)node->name, "tren") == 0)
                {
                    strcpy(numar_tren, (const char *)xmlGetProp(node, (const xmlChar *)"Numar"));

                    statieP = 0;
                    statieS = 0;
                }
            }
        }
        cautaTren(node->children, statie_plecare, statie_sosire, response, root);
    }
}

void parcurgere_nr_tren(xmlNode *a_node, char nr_tren[10], char statie_plecare[100], char statie_sosire[100], char response[1000])
{
    xmlNode *node = NULL;
    for (node = a_node; node; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(node) == 0)
            {
                if (nr_gasit == 1)
                {
                    if (strcmp((const char *)node->name, "statie") == 0 && strcmp((const char *)xmlGetProp(node, (const xmlChar *)"numeStatie"), statie_plecare) == 0)
                    {
                        create_response(node, response);
                    }
                    if (strcmp((const char *)node->name, "statie") == 0 && strcmp((const char *)xmlGetProp(node, (const xmlChar *)"numeStatie"), statie_sosire) == 0)
                    {
                        create_response(node, response);
                    }
                }
            }
            else
            {
                if (strcmp((const char *)node->name, "tren") == 0)
                {
                    if (strcmp((const char *)xmlGetProp(node, (const xmlChar *)"Numar"), numar_tren) == 0)
                    {
                        nr_gasit = 1;
                        strcat(response, "\nTrenul ");
                        strcat(response, (const char *)xmlGetProp(node, (const xmlChar *)"Numar"));
                        strcat(response, "\n--------------------\n");
                    }
                    else
                    {
                        nr_gasit = 0;
                    }
                }
            }
        }
        parcurgere_nr_tren(node->children, numar_tren, statie_plecare, statie_sosire, response);
    }
}

void create_response(xmlNode *node, char response[1000])
{
    strcat(response, "Nume Statie");
    strcat(response, " -> ");
    strcat(response, (const char *)xmlGetProp(node, (const xmlChar *)"numeStatie"));
    strcat(response, "\n");
    strcat(response, "ora_sosire");
    strcat(response, " -> ");
    strcat(response, (const char *)xmlGetProp(node, (const xmlChar *)"ora_sosire"));
    strcat(response, "\n");
    strcat(response, "ora_plecare");
    strcat(response, " -> ");
    strcat(response, (const char *)xmlGetProp(node, (const xmlChar *)"ora_plecare"));
    strcat(response, "\n");
    strcat(response, "intarziere");
    strcat(response, " -> ");
    strcat(response, (const char *)xmlGetProp(node, (const xmlChar *)"intarziere_s"));
    strcat(response, "\n\n");
}

void infoTrenIntarzieri(xmlNode *a_node, char nr_tren[100], char response[100])
{
    xmlNode *node = NULL;
    for (node = a_node; node; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(node))
            {
                if (strcmp((const char *)node->name, "tren") == 0 && strcmp((const char *)xmlGetProp(node, (const xmlChar *)"Numar"), nr_tren) == 0)
                {
                    //printf("aici: %s\n", xmlLastElementChild(node)->name);
                    strcat(response, "Trenul ");
                    strcat(response, nr_tren);
                    strcat(response, " are ");
                    strcat(response, (const char *)xmlNodeGetContent(xmlLastElementChild(node)));
                    strcat(response, " minute intarziere\n");
                    return;
                }
            }
        }
        infoTrenIntarzieri(node->children, nr_tren, response);
    }
}

void infoEstimareSosire(xmlNode *a_node, char numar_tren[10], char statie_initiala[100], char response[100])
{
    xmlNode *node = NULL;

    for (node = a_node; node; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(node) == 0)
            {
                if (nr_gasit == 1)
                {
                    if (strcmp((const char *)node->name, "statie") == 0 && strcmp((const char *)xmlGetProp(node, (const xmlChar *)"numeStatie"), statie_initiala) == 0)
                    {
                        const char *ora_plecare = (const char *)xmlGetProp(node, (const xmlChar *)"ora_sosire");
                        const char *intarziere_sosire = (const char *)xmlGetProp(node, (const xmlChar *)"intarziere_s");
                        int int_intarziere_sosire = atoi(intarziere_sosire);
                        int ora_int = atoi(ora_plecare);
                        if (strlen(ora_plecare) == 4)
                        {
                            strcpy((char *)ora_plecare, ora_plecare + 2);
                        }
                        if (strlen(ora_plecare) == 5)
                        {
                            strcpy((char *)ora_plecare, ora_plecare + 3);
                        }
                        int min_int = atoi(ora_plecare);
                        if (min_int + int_intarziere_sosire > 60)
                        {
                            min_int = int_intarziere_sosire - (60 - min_int);
                            ora_int++;
                        }
                        else if (min_int + int_intarziere_sosire < 0)
                        {
                            min_int = 60 + min_int + int_intarziere_sosire;
                            ora_int--;
                        }
                        else
                        {
                            min_int = min_int + int_intarziere_sosire;
                        }
                        sprintf(response, "Trenul este estimat sa soseasca la ora %d:%d", ora_int, min_int);
                    }
                }
            }
            else
            {
                if (strcmp((const char *)node->name, "tren") == 0)
                {
                    if (strcmp((const char *)xmlGetProp(node, (const xmlChar *)"Numar"), numar_tren) == 0)
                    {
                        nr_gasit = 1;
                    }
                    else
                    {
                        nr_gasit = 0;
                    }
                }
            }
        }
        infoEstimareSosire(node->children, numar_tren, statie_initiala, response);
    }
}

void infoPlecari(xmlNode *a_node, char statie_req[100], char response[1000], xmlNode *root)
{
    xmlNode *node = NULL;
    for (node = a_node; node; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(node) == 0)
            {
                if (strcmp((const char *)node->name, "statie") == 0 && strcmp((const char *)xmlGetProp(node, (const xmlChar *)"numeStatie"), statie_req) == 0)
                {
                    const char *ora_plecare = (const char *)xmlGetProp(node, (const xmlChar *)"ora_plecare");

                    int ora_int = atoi(ora_plecare);
                    if (strlen(ora_plecare) == 4)
                    {
                        strcpy((char *)ora_plecare, ora_plecare + 2);
                    }
                    if (strlen(ora_plecare) == 5)
                    {
                        strcpy((char *)ora_plecare, ora_plecare + 3);
                    }
                    int min_int = atoi(ora_plecare);

                    if ((ora_int == getHour() && getMinutes() <= min_int) || (ora_int == getHour() + 1 && min_int <= getMinutes()))
                    {
                        parcurgere_nr_tren_plecari(root, numar_tren, statie_req, response);
                    }
                }
            }
            else
            {
                if (strcmp((const char *)node->name, "tren") == 0)
                {
                    strcpy(numar_tren, (const char *)xmlGetProp(node, (const xmlChar *)"Numar"));
                }
            }
        }
        infoPlecari(node->children, statie_req, response, root);
    }
}

void parcurgere_nr_tren_plecari(xmlNode *a_node, char numar_tren[10], char statie_req[100], char response[1000])
{
    xmlNode *node = NULL;
    for (node = a_node; node; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(node) == 0)
            {
                if (nr_gasit == 1)
                {
                    if (strcmp((const char *)node->name, "statie") == 0 && strcmp((const char *)xmlGetProp(node, (const xmlChar *)"numeStatie"), statie_req) == 0)
                    {
                        strcat(response, "Ora plecare");
                        strcat(response, " -> ");
                        strcat(response, (const char *)xmlGetProp(node, (const xmlChar *)"ora_plecare"));
                        strcat(response, "\n");
                        strcat(response, "Pleaca din");
                        strcat(response, " -> ");
                        strcat(response, (const char *)xmlGetProp(node, (const xmlChar *)"numeStatie"));
                        strcat(response, "\n");
                    }
                    if (strcmp((const char *)node->name, "ora_stop") == 0 || strcmp((const char *)node->name, "statie_stop") == 0 || strcmp((const char *)node->name, "intarziere") == 0)
                    {
                        strcat(response, (const char *)node->name);
                        strcat(response, " -> ");
                        strcat(response, (const char *)xmlNodeGetContent(node));
                        strcat(response, "\n");
                    }
                }
            }
            else
            {
                if (strcmp((const char *)node->name, "tren") == 0)
                {
                    if (strcmp((const char *)xmlGetProp(node, (const xmlChar *)"Numar"), numar_tren) == 0)
                    {
                        nr_gasit = 1;
                        strcat(response, "\nTrenul ");
                        strcat(response, (const char *)xmlGetProp(node, (const xmlChar *)"Numar"));
                        strcat(response, "\n--------------------\n");
                    }
                    else
                    {
                        nr_gasit = 0;
                    }
                }
            }
        }
        parcurgere_nr_tren_plecari(node->children, numar_tren, statie_req, response);
    }
}

void infoSosiri(xmlNode *a_node, char statie_req[100], char response[1000], xmlNode *root)
{
    xmlNode *node = NULL;
    for (node = a_node; node; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(node) == 0)
            {
                if (strcmp((const char *)node->name, "statie") == 0 && strcmp((const char *)xmlGetProp(node, (const xmlChar *)"numeStatie"), statie_req) == 0)
                {
                    const char *ora_plecare = (const char *)xmlGetProp(node, (const xmlChar *)"ora_sosire");

                    int ora_int = atoi(ora_plecare);
                    if (strlen(ora_plecare) == 4)
                    {
                        strcpy((char *)ora_plecare, ora_plecare + 2);
                    }
                    if (strlen(ora_plecare) == 5)
                    {
                        strcpy((char *)ora_plecare, ora_plecare + 3);
                    }
                    int min_int = atoi(ora_plecare);

                    if ((ora_int == getHour() && getMinutes() <= min_int) || (ora_int == getHour() + 1 && min_int <= getMinutes()))
                    {
                        parcurgere_nr_tren_sosiri(root, numar_tren, statie_req, response);
                    }
                }
            }
            else
            {
                if (strcmp((const char *)node->name, "tren") == 0)
                {
                    strcpy(numar_tren, (const char *)xmlGetProp(node, (const xmlChar *)"Numar"));
                }
            }
        }
        infoSosiri(node->children, statie_req, response, root);
    }
}

void parcurgere_nr_tren_sosiri(xmlNode *a_node, char numar_tren[10], char statie_req[100], char response[1000])
{
    xmlNode *node = NULL;
    for (node = a_node; node; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(node) == 0)
            {
                if (nr_gasit == 1)
                {
                    if (strcmp((const char *)node->name, "statie") == 0 && strcmp((const char *)xmlGetProp(node, (const xmlChar *)"numeStatie"), statie_req) == 0)
                    {
                        strcat(response, "Ora sosire");
                        strcat(response, " -> ");
                        strcat(response, (const char *)xmlGetProp(node, (const xmlChar *)"ora_sosire"));
                        strcat(response, "\n");
                        strcat(response, "In statia");
                        strcat(response, " -> ");
                        strcat(response, (const char *)xmlGetProp(node, (const xmlChar *)"numeStatie"));
                        strcat(response, "\n");
                    }
                    if (strcmp((const char *)node->name, "ora_start") == 0 || strcmp((const char *)node->name, "statie_start") == 0 || strcmp((const char *)node->name, "intarziere") == 0)
                    {
                        strcat(response, (const char *)node->name);
                        strcat(response, " -> ");
                        strcat(response, (const char *)xmlNodeGetContent(node));
                        strcat(response, "\n");
                    }
                }
            }
            else
            {
                if (strcmp((const char *)node->name, "tren") == 0)
                {
                    if (strcmp((const char *)xmlGetProp(node, (const xmlChar *)"Numar"), numar_tren) == 0)
                    {
                        nr_gasit = 1;
                        strcat(response, "\nTrenul ");
                        strcat(response, (const char *)xmlGetProp(node, (const xmlChar *)"Numar"));
                        strcat(response, "\n--------------------\n");
                    }
                    else
                    {
                        nr_gasit = 0;
                    }
                }
            }
        }
        parcurgere_nr_tren_sosiri(node->children, numar_tren, statie_req, response);
    }
}

int updatedData(char *filename, time_t oldMTime)
{
    struct stat fileStat;
    if (stat(filename, &fileStat) < 0)
    {
        perror("\neroare la gasirea informatiilor la stat");
    }
    //printf("HEREE %ld\n",fileStat.st_mtime);
    return fileStat.st_mtime > oldMTime;
}

int nr_statii_total(xmlNode *a_node, char numar_tren[100])
{
    xmlNode *node = NULL;

    for (node = a_node; node; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(node))
            {
                if (strcmp((const char *)node->name, "tren") == 0)
                {
                    if (strcmp((const char *)xmlGetProp(node, (const xmlChar *)"Numar"), numar_tren) == 0)
                    {
                        nr_gasit = 1;
                    }
                    else
                    {
                        nr_gasit = 0;
                    }
                }
                if (nr_gasit == 1 && strcmp((const char *)node->name, "statii") == 0)
                {
                    numar_copii = xmlChildElementCount(node);
                }
            }
        }
        nr_statii_total(node->children, numar_tren);
    }
    return numar_copii;
}
void statii_tren(xmlNode *a_node, char numar_tren[100], char response[1000])
{
  xmlNode *node = NULL;

  for (node = a_node; node; node = node->next)
  {
    if (node->type == XML_ELEMENT_NODE)
    {
      if (xmlChildElementCount(node) == 0)
      {
        if (nr_gasit == 1)
        {
          if (strcmp((const char *)node->name, "statie") == 0)
          {
            strcat(response, "Statia");
            strcat(response, " -> ");
            strcat(response, (const char *)xmlGetProp(node, (const xmlChar *)"numeStatie"));
            strcat(response, "\n");
          }
        }
      }
      else
      {
        if (strcmp((const char *)node->name, "tren") == 0)
        {
          if (strcmp((const char *)xmlGetProp(node, (const xmlChar *)"Numar"), numar_tren) == 0)
          {
            nr_gasit = 1;
          }
          else
          {
            nr_gasit = 0;
          }
        }
      }
    }
    statii_tren(node->children, numar_tren, response);
  }
}

void updateIntarzieri_sosire(xmlNode *a_node, char numar_tren[100], int numar_statie, int intarziere_statie, xmlDoc *document, xmlNode *root)
{
    xmlNode *node = NULL;
    int nr_statie = 0;

    for (node = a_node; node; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(node) == 0)
            {
                if (nr_gasit == 1)
                {
                    if (strcmp((const char *)node->name, "statie") == 0)
                    {
                        nr_statie++;
                        if (nr_statie >= numar_statie)
                        {
                            char char_nr_statie[10];
                            char char_intarziere_statie[10], char_intarziere_s[10];
                            int intarziere_s = 0;
                            int int_intarziere = atoi((const char *)xmlGetProp(node, (const xmlChar *)"intarziere_p"));
                            if (-intarziere_statie >= int_intarziere)
                            {
                                int_intarziere = 0;
                                intarziere_s = intarziere_statie;
                            }
                            else
                            {
                                int_intarziere = int_intarziere + intarziere_statie;
                                intarziere_s = int_intarziere;
                            }

                            //printf("aici %d\n", int_intarziere);
                            sprintf(char_nr_statie, "%d", nr_statie);
                            sprintf(char_intarziere_statie, "%d", int_intarziere);
                            sprintf(char_intarziere_s, "%d", intarziere_s);
                            xmlSetProp(node, (const xmlChar *)"intarziere_p", (const xmlChar *)char_intarziere_statie);
                            xmlSetProp(node, (const xmlChar *)"intarziere_s", (const xmlChar *)char_intarziere_s);
                            change_intarzieri_total(root, numar_tren, document, char_intarziere_statie);
                            xmlSaveFile("./trenuri.xml", document);
                        }
                    }
                }
            }
            else
            {
                if (strcmp((const char *)node->name, "tren") == 0)
                {
                    if (strcmp((const char *)xmlGetProp(node, (const xmlChar *)"Numar"), numar_tren) == 0)
                    {
                        nr_gasit = 1;
                        nr_statie = 0;
                    }
                    else
                    {
                        nr_gasit = 0;
                    }
                }
            }
        }
        updateIntarzieri_sosire(node->children, numar_tren, numar_statie, intarziere_statie, document, root);
    }
}

void updateIntarzieri_plecare(xmlNode *a_node, char numar_tren[100], int numar_statie, int intarziere_statie, xmlDoc *document, xmlNode *root)
{
    xmlNode *node = NULL;
    int nr_statie = 0;

    for (node = a_node; node; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(node) == 0)
            {
                if (nr_gasit == 1)
                {
                    if (strcmp((const char *)node->name, "statie") == 0)
                    {
                        nr_statie++;
                        if (nr_statie > numar_statie)
                        {
                            char char_nr_statie[10];
                            char char_intarziere_statie[10];
                            int int_intarziere = atoi((const char *)xmlGetProp(node, (const xmlChar *)"intarziere_p"));
                            int_intarziere = int_intarziere + intarziere_statie;
                            //printf("aici %d\n", int_intarziere);
                            sprintf(char_nr_statie, "%d", nr_statie);
                            sprintf(char_intarziere_statie, "%d", int_intarziere);
                            xmlSetProp(node, (const xmlChar *)"intarziere_p", (const xmlChar *)char_intarziere_statie);
                            change_intarzieri_total(root, numar_tren, document, char_intarziere_statie);
                            xmlSaveFile("./trenuri.xml", document);
                        }
                    }
                }
            }
            else
            {
                if (strcmp((const char *)node->name, "tren") == 0)
                {
                    if (strcmp((const char *)xmlGetProp(node, (const xmlChar *)"Numar"), numar_tren) == 0)
                    {
                        nr_gasit = 1;
                        nr_statie = 0;
                    }
                    else
                    {
                        nr_gasit = 0;
                    }
                }
            }
        }
        updateIntarzieri_plecare(node->children, numar_tren, numar_statie, intarziere_statie, document, root);
    }
}

void change_intarzieri_total(xmlNode *a_node, char numar_tren[100], xmlDoc *document, char char_intarziere_statie[10])
{
    xmlNode *node = NULL;

    for (node = a_node; node; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE)
        {
            if (xmlChildElementCount(node))
            {
                if (strcmp((const char *)node->name, "tren") == 0)
                {
                    if (strcmp((const char *)xmlGetProp(node, (const xmlChar *)"Numar"), numar_tren) == 0)
                    {
                        xmlNodeSetContent(xmlLastElementChild(node), (const xmlChar *)char_intarziere_statie); //ultimu copil e intarzierea per total
                        xmlSaveFile("./trenuri.xml", document);
                    }
                }
            }
        }
        change_intarzieri_total(node->children, numar_tren, document, char_intarziere_statie);
    }
}

void read_fct(int sd, char msg[1000], int dim)
{
    bzero(msg, dim);
    if (read(sd, msg, dim) < 0)
    {
        perror("[server]Eroare la read() de la client.\n");
        close(sd); /* inchidem conexiunea cu clientul */
    }
}

void write_fct(int sd, char msg[1000], int dim)
{
    if (write(sd, msg, dim) <= 0)
    {
        perror("[server]Eroare la write() catre client.\n");
        close(sd);
    }
}