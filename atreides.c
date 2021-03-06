/***********************************************
*
* @Proposit:
* @Autor/s: Faozi Bouybaouene Gadrouz i Guillem Miró Sierra
* @Data ultima modificacio: 09/01/2022
*
************************************************/

//Llibreries pròpies
#include "atreides.h"
#include "ioscreen.h"

void RsiControlC(void);

//Variables globals
ConfigAtreides configuration;
int num_users, socket_fd;
User * users;
pthread_mutex_t lock;

/***********************************************
*
* @Nom: UpdateFile
* @Finalitat: Actualitzar el fitxer d'usuaris a l'acabar la execució del programa.
* @Parametres:
* @Retorn:
*
************************************************/
void UpdateFile() {
    char cadena[200];
    int fd;

    //Obrim el fitxer
    fd = open("Atreides/users_memory.txt", O_CREAT | O_RDWR, 0666);

    if (fd < 0) {
        printF("Fitxer d'usuaris erroni\n");
        raise(SIGINT);
    } else {

        //Insertem el nombre total d'usuaris
        sprintf(cadena, "%d\n", num_users);
        write(fd, cadena, sizeof(char) * strlen(cadena));

        //Recorrem tota la cadena d'usuaris i hi escribim el seu id, nom i codi postal separat per guions.
        for (int i = 0; i < num_users; i++) {
            sprintf(cadena, "%d-%s-%s\n", users[i].id, users[i].username, users[i].postal_code);
            write(fd, cadena, sizeof(char) * strlen(cadena));
            memset(cadena, 0, strlen(cadena));
            memset(cadena, 0, strlen(cadena));
        }

        close(fd);
    }
}

/***********************************************
*
* @Nom: RsiControlC
* @Finalitat: Controlar l'alliberament de memòria assignada, i tancament de FD i threads oberts, quan es faci ctrl+c
* @Parametres:
* @Retorn:
*
************************************************/
void RsiControlC(void) {
    //Printem el missatge d'adeu del sistema
    printF("\n\nDesconnectat d’Atreides. Dew!\n\n");

    //Alliberem els params de la configuració.
    free(configuration.ip);
    free(configuration.directory);

    //Escrivim el fitxer d'usuaris sencer.
    UpdateFile();

    for (int i = 0; i < num_users; i++) {
        free(users[i].username);
        free(users[i].postal_code);

        //Si el FD està inicialitzat el tanquem i amb ell el thread de cada client.
        if (users[i].file_descriptor != -1) {
            close(users[i].file_descriptor);
            pthread_cancel(users[i].thread);
            pthread_join(users[i].thread, NULL);
            pthread_detach(users[i].thread);
        }
    }
    free(users);

    close(socket_fd);

    //Destruim el mutex inicialitzat.
    pthread_mutex_destroy( & lock);

    //Acabem el programa enviant-nos a nosaltres mateixos el sigint.
    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}

/***********************************************
*
* @Nom: ATREIDES_sendFrame
* @Finalitat: Enviar la trama creada.
* @Parametres: int fd: file descriptor del client, char * frame: string amb les dades a enviar
* @Retorn:
*
************************************************/
void ATREIDES_sendFrame(int fd, char * frame) {
    write(fd, frame, 256);
    printF("Enviada resposta\n");
}

/***********************************************
*
* @Nom: ATREIDES_sendPhotoData
* @Finalitat: Enviar la trama creada de manera silenciosa per la funció de Photo.
* @Parametres: int fd: file descriptor del client, char * frame: string amb les dades a enviar
* @Retorn:
*
************************************************/
void ATREIDES_sendPhotoData(int fd, char * frame) {
    write(fd, frame, 256);
}

/***********************************************
*
* @Nom: ATREIDES_generateFrameLogin
* @Finalitat: Generar el frame de la trama de login.
* @Parametres: char * frame: trama inicialitzada, char type: tipus de trama a enviar, int id: id de l'usuari.
* @Retorn: trama creada i llesta per enviar.
*
************************************************/
char * ATREIDES_generateFrameLogin(char * frame, char type, int id) {

    int i = 0, j = 0;

    char * buffer, id_str[3];

    //Assignem el tipus a la trama
    frame[15] = type;

    //Copiem el id
    snprintf(id_str, 3, "%d", id);
    asprintf( & buffer, "%s", id_str);

    //Mentre la cadena no sigui \0, anem copiant al frame
    for (i = 16; buffer[i - 16] != '\0'; i++) {
        frame[i] = buffer[i - 16];
    }

    //Omplim el que queda de cadena amb \0
    for (j = i; j < 256; j++) {
        frame[j] = '\0';
    }

    free(buffer);
    return frame;

}

/***********************************************
*
* @Nom: ATREIDES_generateFrameSearch
* @Finalitat: Generar el frame de la trama de search
* @Parametres: char * frame: trama inicialitzada, char type: tipus de trama a enviar, char * str: dades a enviar
* @Retorn: trama creada i llesta per enviar.
*
************************************************/
char * ATREIDES_generateFrameSearch(char * frame, char type, char * str) {

    int i = 0, j = 0;

    //Assignem el tipus a la trama
    frame[15] = type;

    //Mentre la cadena no sigui \0, anem copiant al frame
    for (i = 16; str[i - 16] != '\0'; i++) {
        frame[i] = str[i - 16];
    }

    //Omplim el que queda de cadena amb \0
    for (j = i; j < 256; j++) {
        frame[j] = '\0';
    }

    return frame;
}

/***********************************************
*
* @Nom: ATREIDES_fillUsers
* @Finalitat: Inicialitzar l'struct de usuaris.
* @Parametres:
* @Retorn: Struct d'usuaris creat i inicialitzat.
*
************************************************/
User * ATREIDES_fillUsers() {
    char caracter = ' ', * buffer;
    int fd, i = 0;
    User * users_read;

    //Obertura del fitxer
    fd = open("Atreides/users_memory.txt", O_RDONLY);

    //Si no existeix el fitxer...
    if (fd < 0) {
        fd = open("Atreides/users_memory.txt", O_CREAT | O_RDWR, 0666);
        if (fd < 0) {

            printF("Error creant fitxer\n");
            raise(SIGINT);

        } else {
            //Escrivim un usuari admin al fitxer i el tanquem per obrir-lo amb mode lectura/escritura.
            write(fd, "1\n", 2);
            write(fd, "1-Admin-00000\n", strlen("1-Admin-00000\n"));
            close(fd);

            fd = open("Atreides/users_memory.txt", O_RDONLY);
        }
    }

    //Llegim quants usuaris tenim al fitxer.
    buffer = IOSCREEN_readUntilIntro(fd, caracter, i);
    num_users = atoi(buffer);

    //Assignem la memòria necessària per a tots els usuaris.
    users_read = (User * ) malloc(sizeof(User) * num_users);

    free(buffer);

    i = 0;
    while (i < num_users) {

        buffer = NULL;
        buffer = IOSCREEN_read_until(fd, '-');
        users_read[i].id = atoi(buffer);

        users_read[i].username = IOSCREEN_read_until(fd, '-');

        users_read[i].postal_code = IOSCREEN_read_until(fd, '\n');
        //Inicialitzem el FD a -1, si l'usuari es connecta, li canviarem.
        users_read[i].file_descriptor = -1;

        free(buffer);
        i++;
    }

    //Tanquem el fitxer.
    close(fd);

    return users_read;
}

/***********************************************
*
* @Nom: ATREIDES_addUser
* @Finalitat: Afegir un nou usuari a l'struct d'usuaris.
* @Parametres: User u: nou usari a afegir
* @Retorn:
*
************************************************/
void ATREIDES_addUser(User u) {

    users = (User * ) realloc(users, ((num_users) * sizeof(User)));
    users[num_users - 1].username = (char * ) malloc((strlen(u.username) + 1) * sizeof(char));
    users[num_users - 1].postal_code = (char * ) malloc((strlen(u.postal_code) + 1) * sizeof(char));

    users[num_users - 1].id = u.id;
    strcpy(users[num_users - 1].username, u.username);
    strcpy(users[num_users - 1].postal_code, u.postal_code);
    users[num_users - 1].file_descriptor = u.file_descriptor;
    users[num_users - 1].thread = u.thread;
}

/***********************************************
*
* @Nom: ATREIDES_getUserByFD
* @Finalitat: Retornar la posició a l'array d'usuaris mitjançant el file descriptor.
* @Parametres: int fd: file descriptor de l'usuari
* @Retorn: posició a l'array d'usuaris
*
************************************************/
int ATREIDES_getUserByFD(int fd) {
    for (int i = 0; i < num_users; i++) {
        if (fd == users[i].file_descriptor) return i;
    }
    return -1;
}

/***********************************************
*
* @Nom: ATREIDES_receiveUser
* @Finalitat: Processament de l'informació de la trama de rebre un usuari.
* @Parametres: char data[240]: dades rebudes a la trama
* @Retorn: un usuari nou inicialitzat.
*
************************************************/
User ATREIDES_receiveUser(char data[240]) {
    int i, j;
    User u;

    i = 0;

    //Processem el nom d'usuari
    u.username = (char * ) malloc(1 * sizeof(char));
    while (data[i] != '*') {
        u.username[i] = data[i];
        u.username = (char * ) realloc(u.username, i + 2);
        i++;
    }
    u.username[i] = '\0';

    i++;

    j = 0;

    //Processem el codi postal
    u.postal_code = (char * ) malloc(1 * sizeof(char));

    while (data[i] != '\0') {
        u.postal_code[j] = data[i];
        u.postal_code = (char * ) realloc(u.postal_code, j + 2);
        i++;
        j++;
    }
    u.postal_code[j] = '\0';

    //set del id d'usuari a 0. Després es reassignarà.
    u.id = 0;

    return u;
}

/***********************************************
*
* @Nom: ATREIDES_receiveSearch
* @Finalitat: Processament de l'informació de la trama search.
* @Parametres: char data[240]: dades rebudes a la trama
* @Retorn: Usuari amb les dades plenes
*
************************************************/
User ATREIDES_receiveSearch(char data[240]) {
    int i, j, k;
    User u;
    char * id;

    i = 0;

    //Processem el nom d'usuari
    u.username = (char * ) malloc(1 * sizeof(char));
    while (data[i] != '*') {
        u.username[i] = data[i];
        u.username = (char * ) realloc(u.username, i + 2);
        i++;
    }
    u.username[i] = '\0';
    i++;

    j = 0;
    id = (char * ) malloc(1 * sizeof(char));

    //Processem el id de l'usuari
    while (data[i] != '*') {
        id[j] = data[i];
        id = (char * ) realloc(id, j + 2);
        i++;
        j++;
    }
    id[j] = '\0';
    i++;

    u.id = atoi(id);

    k = 0;
    u.postal_code = (char * ) malloc(1 * sizeof(char));

    //Processem el codi postal
    while (data[i] != '\0') {
        u.postal_code[k] = data[i];
        u.postal_code = (char * ) realloc(u.postal_code, k + 2);
        i++;
        k++;
    }
    u.postal_code[k] = '\0';

    free(id);
    return u;
}

/***********************************************
*
* @Nom: ATREIDES_receivePhoto
* @Finalitat: Rebre una imatge sencera i guardar-la correctament.
* @Parametres: Photo p: dades de la imatge que hem rebut i emmagatzemat, int fd: file descriptor de l'usuari, int id: posició de l'usuari a l'array d'usuaris.
* @Retorn:
*
************************************************/
void ATREIDES_receivePhoto(Photo p, int fd, int id) {
    Frame frame;
    int out, contador_trames = 0;
    char * md5 = NULL, * out_file = NULL, cadena[200], * filename = NULL, * trama = NULL;

    //Assignem el nom del fitxer com a id_usuari.jpg
    asprintf( & filename, "%d.jpg", users[id].id);

    //Imprimim per pantalla
    sprintf(cadena, "Guardada com %s\n", filename);
    write(STDOUT_FILENO, cadena, sizeof(char) * strlen(cadena));

    //Concatenem el nom del fitxer amb el directori on el guardarem.
    asprintf( & out_file, "%s/%s", configuration.directory, filename);
    free(filename);

    //Obrim el fitxer amb els permisos necessaris. En cas de que ja existeixi, el trunquem (esborrar el contingut i tornar a fer)
    out = open(out_file, O_CREAT | O_WRONLY | O_TRUNC, 0666);

    //Mirem quantes trames hem de rebre
    int num_trames = p.file_size / 240;

    //Si hem de rebre una trama final més petita de 240, afegim una més.
    if (p.file_size % 240 != 0) {
        num_trames++;
    }

    //Anem rebent les trames.
    while (contador_trames < num_trames) {
        //Rebem el frame de dades
        frame = FRAME_CONFIG_receiveFrame(fd);

        //Si és la última trama, l'escrivim amb el tamany correcte.
        if (contador_trames == num_trames - 1 && p.file_size % 240 != 0) {
            write(out, frame.data, p.file_size % 240);
        } else {
            write(out, frame.data, 240);
        }

        contador_trames++;
    }
    close(out);

    //Rebem el string amb el codi md5
    md5 = FRAME_CONFIG_getMD5(out_file);

    //Comprovem que els md5 coincideixin i ho enviem al Fremen.
    if (md5 != NULL) {
        if (strcmp(p.file_md5, md5) != 0) {
            printF("Error: Les fotos no són iguals! \n");
            trama = FRAME_CONFIG_generateCustomFrame(2, 'R', 1);
        } else {
            trama = FRAME_CONFIG_generateCustomFrame(2, 'I', 0);
        }
        ATREIDES_sendFrame(fd, trama);
        free(trama);
        free(md5);
    }

    free(out_file);
}

/***********************************************
*
* @Nom: ATREIDES_receiveSendInfo
* @Finalitat: Rebre la part d'informació de la trama send.
* @Parametres: char data[240]: dades rebudes a la trama
* @Retorn: Un objecte photo amb les dades plenes. 
*
************************************************/
Photo ATREIDES_receiveSendInfo(char data[240]) {
    int i, j, k;
    Photo p;
    char * number;

    i = 0;

    //Processem el nom del fitxer.
    while (data[i] != '*') {
        p.file_name[i] = data[i];
        i++;
    }
    p.file_name[i] = '\0';
    i++;

    j = 0;
    number = (char * ) malloc(1 * sizeof(char));

    //Processem la mida i la passem a int.
    while (data[i] != '*') {
        number[j] = data[i];
        number = (char * ) realloc(number, j + 2);
        i++;
        j++;
    }
    number[j] = '\0';
    i++;

    p.file_size = atoi(number);

    k = 0;

    //Processem el codi md5
    while (data[i] != '\0') {
        p.file_md5[k] = data[i];
        i++;
        k++;
    }
    p.file_md5[k] = '\0';

    free(number);

    return p;
}

/***********************************************
*
* @Nom: ATREIDES_checkPhoto
* @Finalitat: Intentar obrir una photo per veure si existeix
* @Parametres: char data[240]: dades rebudes a la trama
* @Retorn: FD de la imatge
*
************************************************/
int ATREIDES_checkPhoto(char data[240]) {
    char * out_file = NULL, * filename = NULL;
    int photo_fd;

    asprintf( & filename, "%s.jpg", data);

    //Concatenem el nom del directori amb la imatge
    asprintf( & out_file, "%s/%s", configuration.directory, filename);

    free(filename);

    //Obrim la imatge amb mode lectura.
    photo_fd = open(out_file, O_RDONLY);
    free(out_file);

    return photo_fd;
}

/***********************************************
*
* @Nom: ATREIDES_generatePhotoInfo
* @Finalitat: Generar la informació d'una imatge
* @Parametres: Photo p: imatge inicialitzada, char data[240]: dades rebudes a la trama, int fd: file descriptor de l'usuari
* @Retorn: objecte photo amb les dades copiades.
*
************************************************/
Photo ATREIDES_generatePhotoInfo(Photo p, char data[240], int fd) {
    struct stat stats;
    char * filename = NULL, * out_file = NULL, * md5 = NULL, * data_to_send = NULL, * frame = NULL;
    int i, j;

    //Creem el nom de la imatge i l'afegim al struct.
    asprintf( & filename, "%s.jpg", data);
    strcpy(p.file_name, filename);
    free(filename);

    asprintf( & out_file, "%s/%s", configuration.directory, p.file_name);

    //Generem el md5 i l'afegim al struct.
    md5 = FRAME_CONFIG_getMD5(out_file);
    strcpy(p.file_md5, md5);
    free(md5);

    //Mirem el tamany del fitxer i el desem a l'struct.
    if (stat(out_file, & stats) == 0) {
        p.file_size = stats.st_size;
    }

    //Generem les dades pel frame a enviar
    asprintf( & data_to_send, "%s*%d*%s", p.file_name, p.file_size, p.file_md5);

    frame = FRAME_CONFIG_generateFrame(2);

    frame[15] = 'F';

    for (i = 16; data_to_send[i - 16] != '\0'; i++) {
        frame[i] = data_to_send[i - 16];
    }

    for (j = i; j < 256; j++) {
        frame[j] = '\0';
    }

    ATREIDES_sendFrame(fd, frame);

    free(data_to_send);
    free(frame);
    free(out_file);

    return p;
}

/***********************************************
*
* @Nom: ATREIDES_generateFrameSend
* @Finalitat: Generar un frame del tipus Send
* @Parametres: char * frame: frame inicialitzat, char type: tipus de trama a enviar, char data[240]: dades rebudes a la trama
* @Retorn:
*
************************************************/
void ATREIDES_generateFrameSend(char * frame, char type, char data[240]) {
    int i = 0;

    frame[15] = type;

    for (i = 16; i < 256; i++) {
        frame[i] = data[i - 16];
    }
}

/***********************************************
*
* @Nom: ATREIDES_sendPhoto
* @Finalitat: Enviar una fotografia a Fremen.
* @Parametres: Photo p: objecte amb les dades de la imatge, int user_fd: file descriptor de l'usuari
* @Retorn:
*
************************************************/
void ATREIDES_sendPhoto(Photo p, int user_fd) {
    int contador_trames = 0, num_trames = 0;
    char * frame, buffer[240];

    //Mirem quantes trames haurem d'enviar.
    num_trames = p.file_size / 240;
    if (p.file_size % 240 != 0) {
        num_trames++;
    }

    while (contador_trames < num_trames) {
        //Inicialitzem la cadena amb zeros.
        memset(buffer, 0, sizeof(buffer));

        //Llegim 240 bytes de la imatge
        read(p.photo_fd, buffer, 240);

        //Generem el frame a enviar
        frame = FRAME_CONFIG_generateFrame(2);
        ATREIDES_generateFrameSend(frame, 'D', buffer);

        ATREIDES_sendPhotoData(user_fd, frame);

        contador_trames++;

        free(frame);

        //Afegim un delay per a que no es saturi res.
        usleep(300);
    }

    close(p.photo_fd);
}

/***********************************************
*
* @Nom: ATREIDES_searchUsers
* @Finalitat:
* @Parametres: User u: usuari que demana les dades, int fd: file descriptor de l'usuari que demana les dades
* @Retorn: trama amb les dades a enviar. 
*
************************************************/
char * ATREIDES_searchUsers(User u, int fd) {

    int num_users_pc = 0, i;
    char * res = NULL, * cadena, cadena_print[200];

    //Mirem quants usuaris hi han al codi postal
    for (i = 0; i < num_users; i++) {
        if (strcmp(users[i].postal_code, u.postal_code) == 0) {
            num_users_pc++;
        }
    }

    //Afegim la dada a la trama
    asprintf( & res, "%d", num_users_pc);

    //Imprimim per pantalla
    memset(cadena_print, 0, sizeof(cadena_print));
    sprintf(cadena_print, "Feta la cerca\nHi ha %d persones humanes a %s\n", num_users_pc, u.postal_code);
    write(STDOUT_FILENO, cadena_print, strlen(cadena_print)); 

    for (i = 0; i < num_users; i++) {
        if (strcmp(u.postal_code, users[i].postal_code) == 0) {

            //Imprimim les persones trobades per pantalla.
            memset(cadena_print, 0, sizeof(cadena_print));
            sprintf(cadena_print, "%d %s\n", users[i].id, users[i].username);
            write(STDOUT_FILENO, cadena_print, strlen(cadena_print));

            //Si la propera trama ocuparà més de 240 bytes, enviem la actual i en generem una de nova.
            if (strlen(res) + strlen(users[i].username) + 5 > 240) {
                char * frame_send;

                frame_send = FRAME_CONFIG_generateFrame(2);
                frame_send = ATREIDES_generateFrameSearch(frame_send, 'L', res);

                ATREIDES_sendFrame(fd, frame_send);
                free(frame_send);

                memset(res, 0, sizeof(char) * strlen(res));
            }

            asprintf( & cadena, "%s*%s*%d", res, users[i].username, users[i].id);

            //Tractament de la cadena per evitar problemes de memòria
            free(res);
            res = strdup(cadena);
            free(cadena);
        }
    }

    return res;
}

/***********************************************
*
* @Nom: ATREIDES_threadClient
* @Finalitat: Tractament de les funcionalitats del thread per a cada client connectat
* @Parametres: void * fdClient: file descriptor obert per al client.
* @Retorn:
*
************************************************/
void * ATREIDES_threadClient(void * fdClient) {

    int fd = * ((int * ) fdClient);

    Frame frame;
    int i, exit, u_id;
    User u;
    char * frame_send, cadena[300], * search_data;
    Photo photo;

    exit = 0;
    while (!exit) {

        frame = FRAME_CONFIG_receiveFrame(fd);

        switch (frame.type) {
        case 'C':
            //Cas trama login
            u = ATREIDES_receiveUser(frame.data);

            //assignem el file descriptor obert per tancarlo al final
            u.file_descriptor = fd;
            u.thread = pthread_self();

            i = 0;
            for (i = 0; i < num_users; i++) {
                //en cas de que trobem l'usuari (ja està creat), assignem el id, file descritor i el thread.
                if ((strcmp(u.username, users[i].username) == 0) && (strcmp(u.postal_code, users[i].postal_code) == 0)) {
                    u.id = users[i].id;
                    users[i].file_descriptor = fd;
                    users[i].thread = pthread_self();
                }
            }

            if (u.id == 0) {
                //Revisem que no estiguem intentant escriure diversos usuaris a la vegada i ho bloquegem amb el mutex.
                pthread_mutex_lock( & lock);
                num_users++;
                pthread_mutex_unlock( & lock);

                //Afegim l'usuari.
                u.id = num_users;
                ATREIDES_addUser(u);
            }

            sprintf(cadena, "\nRebut Login de %s %s\nAssignat a ID %d.\n", u.username, u.postal_code, u.id);
            write(STDOUT_FILENO, cadena, sizeof(char) * strlen(cadena));

            free(u.username);
            free(u.postal_code);

            frame_send = FRAME_CONFIG_generateFrame(2);
            frame_send = ATREIDES_generateFrameLogin(frame_send, 'O', u.id);

            ATREIDES_sendFrame(fd, frame_send);

            free(frame_send);
            break;

        case 'S':
            //Cas trama Search
            u = ATREIDES_receiveSearch(frame.data);

            sprintf(cadena, "\nRebut Search %s de %s %d\n", u.postal_code, u.username, u.id);
            write(STDOUT_FILENO, cadena, strlen(cadena));

            search_data = ATREIDES_searchUsers(u, fd);

            frame_send = FRAME_CONFIG_generateFrame(2);
            frame_send = ATREIDES_generateFrameSearch(frame_send, 'L', search_data);

            ATREIDES_sendFrame(fd, frame_send);
            free(frame_send);
            free(search_data);

            free(u.username);
            free(u.postal_code);
            break;

        case 'F':
            //Cas trama Send

            //Guardem les dades de la imatge
            photo = ATREIDES_receiveSendInfo(frame.data);
            //Guardem la posició del usuari al array mitjançant el FD.
            u_id = ATREIDES_getUserByFD(fd);

            sprintf(cadena, "\nRebut send %s de %s %d\n", photo.file_name, users[u_id].username, users[u_id].id);
            write(STDOUT_FILENO, cadena, sizeof(char) * strlen(cadena));

            //Rebem la nova imatge.
            ATREIDES_receivePhoto(photo, fd, u_id);
            break;

        case 'P':
            //Cas trama Photo

            //Guardem la posició del usuari al array mitjançant el FD.
            u_id = ATREIDES_getUserByFD(fd);

            sprintf(cadena, "\nRebut photo %s de %s %d\n", frame.data, users[u_id].username, users[u_id].id);
            write(STDOUT_FILENO, cadena, sizeof(char) * strlen(cadena));

            //Agafem el file descriptor de la imatge.
            int photo_fd = ATREIDES_checkPhoto(frame.data);

            //Si la foto existeix i està desada al sistema...
            if (photo_fd > 0) {
                Photo p;

                p.photo_fd = photo_fd;

                //Generem i enviem la les dades de la imatge i enviem la imatge.
                p = ATREIDES_generatePhotoInfo(p, frame.data, fd);
                ATREIDES_sendPhoto(p, fd);

                close(photo_fd);
            } else {
                //Enviem la trama de foto not found.
                frame_send = FRAME_CONFIG_generateCustomFrame(2, 'F', 2);
                printF("No hi ha foto registrada.\n");
                ATREIDES_sendFrame(fd, frame_send);

                free(frame_send);
            }
            break;

        case 'Q':
            exit = 1;

            //Rebem les dades de l'usuari.
            u = ATREIDES_receiveUser(frame.data);

            sprintf(cadena, "\nRebut Logout de  %s %s \nDesconnectat d'Atreides.\n", u.username, u.postal_code);
            write(STDOUT_FILENO, cadena, strlen(cadena));

            //Guardem el pid del thread per a dins de la rsi poder tancar-lo.
            pthread_detach(pthread_self());
            pthread_cancel(pthread_self());

            free(u.username);
            free(u.postal_code);

            break;
        }

    }

    close(fd);

    return NULL;
}

/***********************************************
*
* @Nom: ATREIDES_fillConfiguration
* @Finalitat: Omplir l'struct de configuracíó d'Atreides
* @Parametres: char * argv: nom del fitxer de configuració.
* @Retorn: struct ple amb les dades llegides
*
************************************************/
ConfigAtreides ATREIDES_fillConfiguration(char * argv) {
    char caracter = ' ', * cadena = NULL, * temp = NULL;
    int i = 0, fd, size = 0;
    ConfigAtreides c;

    //Obertura del fitxer
    fd = open(argv, O_RDONLY);

    if (fd < 0) {
        printF("Fitxer de configuració erroni\n");
        raise(SIGINT);

    } else {
        //Desem les dades de la ip
        c.ip = IOSCREEN_readUntilIntro(fd, caracter, i);

        //Desem les dades del port
        cadena = IOSCREEN_readUntilIntro(fd, caracter, i);
        c.port = atoi(cadena);
        free(cadena);

        //Tractem la cadena del directori per a treure la barra d'inici
        temp = IOSCREEN_readUntilIntro(fd, caracter, i);
        size = strlen(temp);

        c.directory = (char * ) malloc(sizeof(char) * size);
        memset(c.directory, 0, size * sizeof(char));

        for (i = 1; temp[i] != '\0'; i++) {
            c.directory[i - 1] = temp[i];
        }

        //Mirem si el directori existeix. Si no existeix, el creem.
        struct stat st = {0};
        if (stat(c.directory , &st) == -1) {
            mkdir(c.directory, 0700);
        }

        close(fd);
        free(temp);
        printF("Llegit el fitxer de configuració\n");
    }

    return c;
}

/***********************************************
*
* @Nom: ATREIDES_configSocket
* @Finalitat: Configurar el socket de connexió del servidor
* @Parametres: ConfigAtreides config:
* @Retorn:
*
************************************************/
int ATREIDES_configSocket(ConfigAtreides config) {

    struct sockaddr_in s_addr;
    int fdSocket = -1;

    //Creem el socket
    fdSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (fdSocket < 0) {
        printF("ERROR durant la creacio del socket\n");
        return -1;
    }

    //Assignem les dades de port i ip
    memset( & s_addr, 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(config.port);
    s_addr.sin_addr.s_addr = inet_addr(config.ip);

    //Fem el bind al socket.
    if (bind(fdSocket, (void * ) & s_addr, sizeof(s_addr)) < 0) {
        printF("ERROR fent el bind\n");
        return -1;
    }

    //Escoltem connexions de fins a 6 usuaris concurrents.
    listen(fdSocket, 6);

    return fdSocket;
}

/***********************************************
*
* @Nom: Main
* @Finalitat: Control del programa
*
************************************************/
int main(int argc, char ** argv) {
    int clientFD;

    //Check d'argumentos d'entrada
    if (argc != 2) {
        printF("Error, falta el nom de fitxer de configuracio.\n");
        return -1;
    }

    //Assignació del signal de CtrlC a la nostra función
    signal(SIGINT, (void * ) RsiControlC);

    //Lectura del fitxer de configuració y tancament del seu file descriptor.
    configuration = ATREIDES_fillConfiguration(argv[1]);

    //Càrrega d'usuaris a l'struct
    users = ATREIDES_fillUsers();

    //Config del socket
    socket_fd = ATREIDES_configSocket(configuration);

    if (socket_fd < 1) {
        printF("ERROR: imposible preparar el socket\n");
        close(socket_fd);
        raise(SIGINT);
    }

    //Intentem inicialitzar el mutex.
    if (pthread_mutex_init( & lock, NULL) != 0) {
        printF("\nERROR: Init del mutex\n");
        close(socket_fd);
        raise(SIGINT);
    }

    while (1) {
        printF("Esperant connexions...\n");
        clientFD = accept(socket_fd, (struct sockaddr * ) NULL, NULL);

        //Creem i llancem la funció del thread.
        pthread_t thrd;
        pthread_create( & thrd, NULL, ATREIDES_threadClient, & clientFD);
    }

    raise(SIGINT);

    return 0;
}