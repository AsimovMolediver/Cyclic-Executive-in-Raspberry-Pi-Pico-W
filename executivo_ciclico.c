#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/cyw43_arch.h"

// Definições do executivo cíclico
#define HYPERPERIOD 2 // Total de ciclos menores no hiperperíodo
#define LED_PIN CYW43_WL_GPIO_LED_PIN // Pino do LED embutido no Pico W

// Variáveis globais compartilhadas
volatile int minor_cycle_count_core1 = 0; // Contador do ciclo menor para o Core 1
volatile int minor_cycle_flag_1 = 0;      // Flag de sincronização para o Core 1
volatile int minor_cycle_flag_2 = 0;      // Flag de sincronização para o Core 2

// Funções das tarefas do Core 1
void major_10() {
    cyw43_arch_gpio_put(LED_PIN, 1); // Liga o LED
    printf("Core 1 - major_10: LED ligado\n");
}

void major_11() {
    cyw43_arch_gpio_put(LED_PIN, 0); // Desliga o LED
    printf("Core 1 - major_11: LED desligado\n");
}

// Funções das tarefas do Core 2
void major_20() {
    int a = 5, b = 15;
    printf("Core 1 - major_20: Soma %d + %d = %d\n", a, b, a + b);
}

void major_21() {
    int c = 5, d = 15;
    printf("Core 2 - major_21: multiplicacao %d x %d = %d\n", c, d, c * d);
}

// Função de interrupção do temporizador
bool timer_callback(struct repeating_timer *t) {
    minor_cycle_flag_1 = 1; // Libera o próximo ciclo menor para o Core 1
    minor_cycle_flag_2 = 1; // Libera o próximo ciclo menor para o Core 2
    return true;
}

// Função do Core 2
void core2_loop() {
    while (true) {
        while (!minor_cycle_flag_2); // Aguarda a interrupção do temporizador para o Core 2
        minor_cycle_flag_2 = 0; // Reseta a flag após liberar o ciclo

        switch (minor_cycle_count_core1) {
            case 0:
                major_20(); // Executa a tarefa major_20
                break;
            case 1:
                major_21(); // Executa a tarefa major_21
                break;
            default:
                printf("Core 2 - Erro no ciclo!\n");
                break;
        }

    }
}

// Função principal do Core 1
int main() {
    stdio_init_all();
    printf("Inicializando o Executivo Cíclico\n");

    // Inicializa o Wi-Fi para controle do LED
    if (cyw43_arch_init()) {
        printf("Falha ao inicializar o Wi-Fi\n");
        return -1;
    }

    // Inicia o Core 2
    multicore_launch_core1(core2_loop);

    // Configura o temporizador para 2000 ms por ciclo menor
    struct repeating_timer timer;
    add_repeating_timer_ms(2000, timer_callback, NULL, &timer);

    // Loop principal do Core 1
    while (true) {
        while (!minor_cycle_flag_1); // Aguarda a interrupção do temporizador para o Core 1
        minor_cycle_flag_1 = 0; // Reseta a flag após liberar o ciclo

        switch (minor_cycle_count_core1) 
        {
            case 0:
                major_10(); // Executa a tarefa major_10
                break;
            case 1:
                major_11(); // Executa a tarefa major_11
                break;
            default:
                printf("Core 1 - Erro no ciclo!\n");
                break;
        }

        minor_cycle_count_core1++;
        // Atualiza o ciclo menor para o próximo estado
        if (minor_cycle_count_core1 >= HYPERPERIOD) 
        {
            minor_cycle_count_core1 = 0;
            printf("Cores - Hiperperíodo concluído.\n");
        }
    }
}
