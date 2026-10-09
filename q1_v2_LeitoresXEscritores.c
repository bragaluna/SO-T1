// Processos Leitores × Escritores
// Versão 2: Escritores com preferência sobre leitores (sem leitura suja)

#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define SALDO_INICIAL 1000
#define RODADAS 5

volatile int saldo = SALDO_INICIAL, extrato = 0;
int leitores_ativos = 0, escritores_pendentes = 0;  // contadores protegidos por mutex_l / mutex_e
sem_t sem_recurso;   // acesso exclusivo aos dados (1o leitor / escritor)
sem_t sem_leitura;   // fechado enquanto houver escritor pendente: barra novos leitores
sem_t sem_fila;      // fila de entrada dos leitores (só 1 leitor disputa sem_leitura)
sem_t mutex_l, mutex_e;
pthread_mutex_t io = PTHREAD_MUTEX_INITIALIZER;
int atraso_l, atraso_e, tempo_escrita, sujas = 0, *valores;
struct timespec t0;

void log_(const char *tipo, int id, const char *fmt, ...) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    long ms = (t.tv_sec - t0.tv_sec) * 1000 + (t.tv_nsec - t0.tv_nsec) / 1000000;
    va_list a; va_start(a, fmt);
    pthread_mutex_lock(&io);
    printf("[%5ldms] %-8s %d | ", ms, tipo, id);
    vprintf(fmt, a); printf("\n");
    pthread_mutex_unlock(&io);
    va_end(a);
}

// sem_wait que avisa quando a thread é bloqueada
void espera(sem_t *s, const char *tipo, int id, const char *motivo) {
    if (sem_trywait(s) != 0) {
        log_(tipo, id, "BLOQUEADO: %s", motivo);
        sem_wait(s);
        log_(tipo, id, "desbloqueado");
    }
}

void *leitor(void *arg) {
    int id = (long)arg;
    log_("LEITOR", id, "criado");
    for (int r = 0; r < RODADAS; r++) {
        usleep(atraso_l * 1000);
        espera(&sem_fila, "LEITOR", id, "outro leitor na fila de entrada");
        espera(&sem_leitura, "LEITOR", id, "há escritor ativo/esperando");
        sem_wait(&mutex_l);
        if (++leitores_ativos == 1) espera(&sem_recurso, "LEITOR", id, "escritor usando os dados");
        sem_post(&mutex_l);
        sem_post(&sem_leitura);
        sem_post(&sem_fila);

        log_("LEITOR", id, "entrou na região crítica (leitores ativos: %d)", leitores_ativos);
        int s = saldo, e = extrato;
        usleep(tempo_escrita * 1000 / 2); 
        if (s == SALDO_INICIAL + e)
            log_("LEITOR", id, "saldo=%d extrato=%d (consistente)", s, e);
        else {
            __sync_fetch_and_add(&sujas, 1);
            log_("LEITOR", id, "*** LEITURA SUJA *** saldo=%d extrato=%d", s, e);
        }
        log_("LEITOR", id, "saiu da região crítica");

        sem_wait(&mutex_l);
        if (--leitores_ativos == 0) sem_post(&sem_recurso);
        sem_post(&mutex_l);
    }
    log_("LEITOR", id, "finalizado");
    return NULL;
}

void *escritor(void *arg) {
    int id = (long)arg, v = valores[id];
    const char *op = v >= 0 ? "depósito" : "saque";
    log_("ESCRITOR", id, "criado (%s de %d por rodada)", op, abs(v));
    for (int r = 0; r < RODADAS; r++) {
        usleep(atraso_e * 1000);
        sem_wait(&mutex_e);
        if (++escritores_pendentes == 1) espera(&sem_leitura, "ESCRITOR", id, "leitor na fila de entrada");
        sem_post(&mutex_e);   // a partir daqui, novos leitores ficam barrados
        espera(&sem_recurso, "ESCRITOR", id, "leitores ou outro escritor usando os dados");

        log_("ESCRITOR", id, "entrou na região crítica");
        if (saldo + v < 0)
            log_("ESCRITOR", id, "saque de %d recusado: saldo insuficiente (saldo=%d)", abs(v), saldo);
        else {
            extrato += v;
            log_("ESCRITOR", id, "%s de %d: extrato=%d registrado, saldo=%d ainda desatualizado",
                 op, abs(v), extrato, saldo);
            usleep(tempo_escrita * 1000);
            saldo += v;
            log_("ESCRITOR", id, "saldo atualizado: saldo=%d extrato=%d", saldo, extrato);
        }
        log_("ESCRITOR", id, "saiu da região crítica");

        sem_post(&sem_recurso);
        sem_wait(&mutex_e);
        if (--escritores_pendentes == 0) sem_post(&sem_leitura);  // sem escritores: libera leitores
        sem_post(&mutex_e);
    }
    log_("ESCRITOR", id, "finalizado");
    return NULL;
}

int main(void) {
    int nl, ne;
    printf("Leitores, escritores: "); scanf("%d %d", &nl, &ne);
    printf("Atraso leitor (ms), atraso escritor (ms), tempo de escrita (ms): ");
    scanf("%d %d %d", &atraso_l, &atraso_e, &tempo_escrita);
    valores = malloc(ne * sizeof(int));
    printf("Valor de cada escritor (%d valores; positivo=depósito, negativo=saque): ", ne);
    for (int i = 0; i < ne; i++) scanf("%d", &valores[i]);

    sem_init(&sem_recurso, 0, 1); sem_init(&sem_leitura, 0, 1); sem_init(&sem_fila, 0, 1);
    sem_init(&mutex_l, 0, 1); sem_init(&mutex_e, 0, 1);
    clock_gettime(CLOCK_MONOTONIC, &t0);
    pthread_t *th = malloc((nl + ne) * sizeof(pthread_t));
    for (long i = 0; i < nl; i++) pthread_create(&th[i], NULL, leitor, (void *)i);
    for (long i = 0; i < ne; i++) pthread_create(&th[nl + i], NULL, escritor, (void *)i);
    for (int i = 0; i < nl + ne; i++) pthread_join(th[i], NULL);

    printf("\nFinal: saldo=%d extrato=%d | leituras sujas: %d\n", saldo, extrato, sujas);
    return 0;
}