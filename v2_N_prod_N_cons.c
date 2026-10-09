//VARIOS PRODUTORES e VARIOS CONSUMIDORES

#include "util.h"
#include <semaphore.h>

int n_prod, n_cons, tam, itens, pmin, pmax, cmin, cmax;
int *buffer;
int in = 0, out = 0, ocupados = 0;
int reservados = 0; // itens ja reservados pelos C
sem_t vazias, cheias, mutex, mutex_cont;

//PRODUTOR
static void *produtor(void *p) {
   
    int id = (int)(size_t)p;
    semear(id, 1);

    for (int k = 1; k <= itens; k++) {
       
        int valor = id * 1000 + k;
        log_msg("Produtor %d produzindo item %d", id, valor);
        dormir_ms(atraso(pmin, pmax));

        if (sem_trywait(&vazias) != 0) {          
            log_msg("Produtor %d dormindo (buffer cheio)", id);
            sem_wait(&vazias);                   
        }

        sem_wait(&mutex);                         
        buffer[in] = valor;
        in = (in + 1) % tam;
        ocupados++;
        log_msg("Produtor %d colocou item %d no buffer", id, valor);
        mostrar_buffer(buffer, tam, ocupados);
        sem_post(&mutex);                      
        sem_post(&cheias);                       
    }
    return NULL;
}

//CONSUMIDOR
static void *consumidor(void *p) {
    int id = (int)(size_t)p;
    semear(id, 2);

    for (;;) { // faz loop até consumir todos itens 
        sem_wait(&mutex_cont); // reserva um item
        
        if (reservados >= n_prod * itens) {
            sem_post(&mutex_cont); // libera mutex
            break; 
        }
        reservados++;
        sem_post(&mutex_cont);

        if (sem_trywait(&cheias) != 0) {       
            log_msg("Consumidor %d dormindo (buffer vazio)", id);
            sem_wait(&cheias);                    
        }
        sem_wait(&mutex);                       
        
        int valor = buffer[out];
        buffer[out] = VAZIO;
        out = (out + 1) % tam;
        ocupados--;
        log_msg("Consumidor %d retirou item %d do buffer", id, valor);
        mostrar_buffer(buffer, tam, ocupados);
        sem_post(&mutex);                        
        sem_post(&vazias);                     
        log_msg("Consumidor %d consumindo item %d", id, valor);
        dormir_ms(atraso(cmin, cmax));
    }
    return NULL;
}

int main(int argc, char **argv) { // permite alterar os valores direto no terminal, a inves de varios inputs
    
    n_prod = arg(argc, argv, 1, 3);   n_cons = arg(argc, argv, 2, 2);
    tam    = arg(argc, argv, 3, 5);   itens  = arg(argc, argv, 4, 6);
    pmin   = arg(argc, argv, 5, 100); pmax   = arg(argc, argv, 6, 600);  // P: garante que as velocidades da producao variem
    cmin   = arg(argc, argv, 7, 300); cmax   = arg(argc, argv, 8, 900);  // C: garante que as velocidades do consumo variem
   
    if (n_prod < 1 || n_prod > 999 || n_cons < 1 || tam < 1 || itens < 1 || itens > 999 || pmin > pmax || cmin > cmax) {
        printf("Parametros invalidos (n_prod e itens entre 1 e 999, minimo <= maximo).\n"); // P nao produz mais rapido que C consome, e vice versa (perda de itens)
        return 1;
    }

    iniciar_log();
    buffer = calloc(tam, sizeof(int));
    sem_init(&vazias, 0, tam);   
    sem_init(&cheias, 0, 0);    
    sem_init(&mutex, 0, 1);      // proteger buffer
    sem_init(&mutex_cont, 0, 1); // proteger variavel reservados

    printf("Versão 2: %d produtores x %d consumidores | buffer = %d | %d itens por produtor\n\n", n_prod, n_cons, tam, itens);

    pthread_t *tp = malloc(sizeof(pthread_t) * n_prod); // threads P 
    pthread_t *tc = malloc(sizeof(pthread_t) * n_cons); // threads C 

    for (int i=0; i<n_cons; i++) pthread_create(&tc[i], NULL, consumidor, (void *)(size_t)(i + 1));
    for (int i=0; i<n_prod; i++) pthread_create(&tp[i], NULL, produtor, (void *)(size_t)(i + 1));
    for (int i=0; i<n_prod; i++) pthread_join(tp[i], NULL);
    for (int i=0; i<n_cons; i++) pthread_join(tc[i], NULL);

    printf("\nFim: todos os %d itens foram produzidos e consumidos.\n", n_prod * itens);
    sem_destroy(&vazias);
    sem_destroy(&cheias); 
    sem_destroy(&mutex); 
    sem_destroy(&mutex_cont);
    free(buffer); 
    free(tp); 
    free(tc);
    return 0;
}