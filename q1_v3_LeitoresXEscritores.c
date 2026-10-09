// Processos Leitores × Escritores
// Versão 3: Sem controle de concorrência 

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define SALDO_INICIAL 1000
#define RODADAS 5

volatile int saldo = SALDO_INICIAL;
int esperado = SALDO_INICIAL, perdidas = 0, leituras_erradas = 0;
pthread_mutex_t io = PTHREAD_MUTEX_INITIALIZER; 
int atraso_l, atraso_e, tempo_escrita, *valores;
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

void *leitor(void *arg) {
    int id = (long)arg;
    log_("LEITOR", id, "criado");
    for (int r = 0; r < RODADAS; r++) {
        usleep(atraso_l * 1000);
        int s = saldo, e = esperado;
        if (s == e) log_("LEITOR", id, "consultou saldo=%d (correto)", s);
        else {
            __sync_fetch_and_add(&leituras_erradas, 1);
            log_("LEITOR", id, "*** ERRO *** consultou saldo=%d, mas o correto seria %d (diferença %+d)", s, e, s - e);
        }
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
        int lido = saldo;                       // 1. lê o saldo
        log_("ESCRITOR", id, "leu saldo=%d, vai fazer %s de %d", lido, op, abs(v));
        if (lido + v < 0) {
            log_("ESCRITOR", id, "saque de %d recusado: saldo insuficiente", abs(v));
            continue;
        }
        usleep(tempo_escrita * 1000);           // 2. processa (janela da condição de corrida)
        if (saldo != lido) {                    // alguém alterou o saldo no meio
            __sync_fetch_and_add(&perdidas, 1);
            log_("ESCRITOR", id, "*** ATUALIZAÇÃO PERDIDA *** saldo mudou de %d para %d durante a operação; "
                 "vai sobrescrever com %d", lido, saldo, lido + v);
        }
        saldo = lido + v;                       // 3. grava (sobrescreve o trabalho dos outros)
        __sync_fetch_and_add(&esperado, v);
        int s = saldo, e = esperado;
        if (s == e) log_("ESCRITOR", id, "gravou saldo=%d (correto)", s);
        else log_("ESCRITOR", id, "*** ERRO *** gravou saldo=%d, mas o correto seria %d (diferença %+d)", s, e, s - e);
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

    clock_gettime(CLOCK_MONOTONIC, &t0);
    pthread_t *th = malloc((nl + ne) * sizeof(pthread_t));
    for (long i = 0; i < nl; i++) pthread_create(&th[i], NULL, leitor, (void *)i);
    for (long i = 0; i < ne; i++) pthread_create(&th[nl + i], NULL, escritor, (void *)i);
    for (int i = 0; i < nl + ne; i++) pthread_join(th[i], NULL);

    printf("\nFinal: saldo=%d | saldo correto=%d | %s\nAtualizações perdidas: %d | leituras com valor errado: %d\n",
           saldo, esperado, saldo == esperado ? "OK" : "*** SALDO FINAL INCORRETO ***", perdidas, leituras_erradas);
    return 0;
}