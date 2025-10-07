#include <pthread.h>
#include <assert.h>

// Estados do sistema
#define BOMBA_DESLIGADA 0
#define BOMBA_LIGADA 1

#define NIVEL_BAIXO 0
#define NIVEL_NORMAL 1  
#define NIVEL_ALTO 2

#define PRESSAO_NORMAL 0
#define PRESSAO_ALTA 1

// Parâmetros da bomba de 7,5 CV
const int CORRENTE_NOMINAL = 25;    // Amperes
const int CORRENTE_MAXIMA = 30;     // Amperes
const int TEMPERATURA_MAXIMA = 80;  // °C
const int MAX_CICLOS_CONTINUOS = 15;

// Variáveis de estado global
int comando_bomba = BOMBA_DESLIGADA;
int sensor_nivel = NIVEL_NORMAL;
int sensor_pressao = PRESSAO_NORMAL;
int temperatura = 30;
int corrente_atual = 0;
int ciclos_operacao = 0;
int timer_operacao = 0;
int ligacoes_realizadas = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Função para ativar a bomba
void ativar_bomba() {
    pthread_mutex_lock(&mutex);
    if (comando_bomba == BOMBA_DESLIGADA) {
        comando_bomba = BOMBA_LIGADA;
        timer_operacao = MAX_CICLOS_CONTINUOS;
        ciclos_operacao = 0;
        ligacoes_realizadas++;
        
        // Simular corrente de partida (mais alta)
        corrente_atual = CORRENTE_NOMINAL + 5;
    }
    pthread_mutex_unlock(&mutex);
}

// Função para desativar a bomba
void desativar_bomba() {
    pthread_mutex_lock(&mutex);
    if (comando_bomba == BOMBA_LIGADA) {
        comando_bomba = BOMBA_DESLIGADA;
        timer_operacao = 0;
        corrente_atual = 0;
    }
    pthread_mutex_unlock(&mutex);
}

// Thread 1: Controlador principal da bomba
void* controlador_bomba(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        
        // Condições para ligar a bomba
        if (sensor_nivel == NIVEL_ALTO && 
            sensor_pressao == PRESSAO_NORMAL && 
            timer_operacao == 0 &&
            temperatura < 75 &&  // Margem de segurança
            comando_bomba == BOMBA_DESLIGADA) {
            ativar_bomba();
        }
        
        // Condições para desligar a bomba
        if (comando_bomba == BOMBA_LIGADA) {
            if (sensor_nivel == NIVEL_BAIXO || 
                sensor_pressao == PRESSAO_ALTA || 
                temperatura >= TEMPERATURA_MAXIMA ||
                corrente_atual > CORRENTE_MAXIMA) {
                desativar_bomba();
            }
        }
        
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

// Thread 2: Atualização de timer e parâmetros físicos
void* atualizar_parametros(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        
        if (comando_bomba == BOMBA_LIGADA) {
            // Decrementar timer
            if (timer_operacao > 0) {
                timer_operacao--;
                ciclos_operacao++;
            }
            
            // Atualizar corrente (simular variações)
            if (corrente_atual > CORRENTE_NOMINAL) {
                corrente_atual--;  // Reduzir da corrente de partida
            } else {
                corrente_atual = CORRENTE_NOMINAL + nondet_int() % 3 - 1;
            }
            
            // Aumentar temperatura durante operação
            if (temperatura < TEMPERATURA_MAXIMA) {
                temperatura += 1 + nondet_int() % 2;
            }
            
            // Desligar se timer chegou a zero
            if (timer_operacao == 0 && comando_bomba == BOMBA_LIGADA) {
                desativar_bomba();
            }
        } else {
            // Resfriamento quando desligada
            if (temperatura > 30) {
                temperatura -= 1 + nondet_int() % 2;
            }
        }
        
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

// Thread 3: Simulador de sensores (não determinístico)
void* simulador_sensores(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        
        // Simular variações não determinísticas nos sensores
        sensor_nivel = nondet_int();
        __ESBMC_assume(sensor_nivel >= NIVEL_BAIXO && sensor_nivel <= NIVEL_ALTO);
        
        sensor_pressao = nondet_int();
        __ESBMC_assume(sensor_pressao >= PRESSAO_NORMAL && sensor_pressao <= PRESSAO_ALTA);
        
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

// VERIFICAÇÕES DE SEGURANÇA

// Propriedade 1: Bomba nunca deve ligar com nível baixo
void verificar_nivel_baixo() {
    assert(!(sensor_nivel == NIVEL_BAIXO && comando_bomba == BOMBA_LIGADA));
}

// Propriedade 2: Bomba deve desligar com pressão alta
void verificar_pressao_alta() {
    assert(!(sensor_pressao == PRESSAO_ALTA && comando_bomba == BOMBA_LIGADA));
}

// Propriedade 3: Corrente dentro dos limites
void verificar_corrente() {
    assert(corrente_atual <= CORRENTE_MAXIMA);
}

// Propriedade 4: Temperatura segura
void verificar_temperatura() {
    assert(temperatura <= TEMPERATURA_MAXIMA);
}

// Propriedade 5: Não operar indefinidamente
void verificar_operacao_continua() {
    assert(ciclos_operacao <= MAX_CICLOS_CONTINUOS * 2);
}

// Propriedade 6: Bomba desliga quando timer chega a zero
void verificar_timer() {
    assert(!(timer_operacao == 0 && comando_bomba == BOMBA_LIGADA));
}

// Propriedade 7: Prevenção de partidas muito rápidas
void verificar_partidas_rapidas() {
    assert(ligacoes_realizadas <= 20); // Limite arbitrário para teste
}

// Função principal de verificação
void verificar_seguranca() {
    verificar_nivel_baixo();
    verificar_pressao_alta();
    verificar_corrente();
    verificar_temperatura();
    verificar_operacao_continua();
    verificar_timer();
    verificar_partidas_rapidas();
}

// Thread de verificação contínua
void* verificador_seguranca(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        verificar_seguranca();
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main() {
    pthread_t t1, t2, t3, t4;
    
    // Inicializar variáveis
    comando_bomba = BOMBA_DESLIGADA;
    sensor_nivel = NIVEL_NORMAL;
    sensor_pressao = PRESSAO_NORMAL;
    temperatura = 30;
    corrente_atual = 0;
    
    // Criar threads
    pthread_create(&t1, NULL, controlador_bomba, NULL);
    pthread_create(&t2, NULL, atualizar_parametros, NULL);
    pthread_create(&t3, NULL, simulador_sensores, NULL);
    pthread_create(&t4, NULL, verificador_seguranca, NULL);
    
    // Manter o programa rodando
    while (1) {
        // Verificação adicional no loop principal
        pthread_mutex_lock(&mutex);
        verificar_seguranca();
        pthread_mutex_unlock(&mutex);
    }
    
    return 0;
}
