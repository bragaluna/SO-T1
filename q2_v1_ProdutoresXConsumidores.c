// Produtores × Consumidores
// Versão com vários produtores e 1 consumidor

#include "util.h"
#include <semaphore.h>

int n_prod, tam, itens, pmin, pmax, cmin, cmax;
int *buffer;
int in = 0, out = 0, ocupados = 0;
sem_t vazias, cheias, mutex;

static void *produtor(void *p) {
    int id = (int)(size_t)p;
    semear(id, 1);

    for (int k = 1; k <= itens; k++) {
        int valor = id * 1000 + k;
        log_msg("Produtor %d produzindo item %d", id, valor);
        dormir_ms(atraso(pmin, pmax));

        if (sem_trywait(&vazias) != 0) {  
            log_msg("Produtor %d dormindo", id);
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

static void *consumidor(void *p) {
    (void)p;
    semear(1, 2);

    for (int n = 0; n < n_prod * itens; n++) {
        if (sem_trywait(&cheias) != 0) {  // se falhou, vai bloquear
            log_msg("Consumidor 1 dormindo (buffer vazio)");
            sem_wait(&cheias);                  
        }
        
        sem_wait(&mutex);                   
        int valor = buffer[out];
        buffer[out] = VAZIO;
        out = (out + 1) % tam;
        ocupados--;
        log_msg("Consumidor 1 retirou item %d do buffer", valor);
        mostrar_buffer(buffer, tam, ocupados);
        sem_post(&mutex);                        
        sem_post(&vazias);                       

        log_msg("Consumidor 1 consumindo item %d", valor);
        dormir_ms(atraso(cmin, cmax));
    }
    return NULL;
}

int main(int argc, char **argv) {
   
    n_prod = arg(argc, argv, 1, 3);   tam  = arg(argc, argv, 2, 5);   itens = arg(argc, argv, 3, 6);
    pmin   = arg(argc, argv, 4, 100); pmax = arg(argc, argv, 5, 600);
    cmin   = arg(argc, argv, 6, 300); cmax = arg(argc, argv, 7, 900);

    if (n_prod < 1 || n_prod > 999 || tam < 1 || itens < 1 || itens > 999 || pmin > pmax || cmin > cmax) {
        printf("Parametros invalidos\n");
        return 1;
    }

    iniciar_log();
    buffer = calloc(tam, sizeof(int));
    sem_init(&vazias, 0, tam);   
    sem_init(&cheias, 0, 0);     
    sem_init(&mutex, 0, 1);     

    printf("Versão 1: %d produtores x 1 consumidor | buffer = %d | %d itens por produtor ===\n\n", n_prod, tam, itens);

    pthread_t *tp = malloc(sizeof(pthread_t) * n_prod), tc; //cria N threads produtoras e 1 consumidora
    pthread_create(&tc, NULL, consumidor, NULL);
    
    for (int i = 0; i < n_prod; i++) // cria
        pthread_create(&tp[i], NULL, produtor, (void *)(size_t)(i + 1));
    for (int i = 0; i < n_prod; i++)  // espera P terminar
        pthread_join(tp[i], NULL);
    pthread_join(tc, NULL);   // espera C terminar

    printf("\nFim: todos os %d itens foram produzidos e consumidos.\n", n_prod * itens);
    
    sem_destroy(&vazias); 
    sem_destroy(&cheias); 
    sem_destroy(&mutex);
    free(buffer); 
    free(tp);
    return 0;
}
