#include <zephyr/kernel.h>             // Funções básicas do Zephyr (ex: k_msleep, k_thread, etc.)
#include <zephyr/device.h>             // API para obter e utilizar dispositivos do sistema
#include <zephyr/drivers/gpio.h>       // API para controle de pinos de entrada/saída (GPIO)
#include <pwm_z42.h>                // Biblioteca personalizada com funções de controle do TPM (Timer/PWM Module)

#include <zephyr/drivers/uart.h>
#include <stdlib.h>

const struct device *uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));


// Define o LED usando Device Tree
#define LED0_NODE DT_ALIAS(led0) //green
#define LED1_NODE DT_ALIAS(led1) //blue 
#define LED2_NODE DT_ALIAS(led2) // red

// Verifica se o LED está definido no Device Tree
#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
static const struct gpio_dt_spec ledVerde = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
#else
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

#if DT_NODE_HAS_STATUS(LED1_NODE, okay)
static const struct gpio_dt_spec ledAzul = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
#else
#error "Unsupported board: led1 devicetree alias is not defined"
#endif

#if DT_NODE_HAS_STATUS(LED2_NODE, okay)
static const struct gpio_dt_spec ledVermelho = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
#else
#error "Unsupported board: led2 devicetree alias is not defined"
#endif

void initLeds(){
    int ret;

    // Verifica se o device está pronto
    if (!gpio_is_ready_dt(&ledAzul)) {
        printk("Error: LED device %s is not ready\n", ledAzul.port->name);
        return;
    }
    if (!gpio_is_ready_dt(&ledVerde)) {
        printk("Error: LED device %s is not ready\n", ledVerde.port->name);
        return;
    }
    if (!gpio_is_ready_dt(&ledVermelho)) {
        printk("Error: LED device %s is not ready\n", ledVermelho.port->name);
        return;
    }

    // Configura o pino como saída
    ret = gpio_pin_configure_dt(&ledAzul, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error %d: failed to configure LED pin\n", ret);
        return;
    }
    ret = gpio_pin_configure_dt(&ledVerde, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error %d: failed to configure LED pin\n", ret);
        return;
    }
    ret = gpio_pin_configure_dt(&ledVermelho, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error %d: failed to configure LED pin\n", ret);
        return;
    }
}


// Define o valor do registrador MOD do TPM para configurar o período do PWM
#define TPM_MODULE 1000         // Define a frequência do PWM fpwm = (TPM_CLK / (TPM_MODULE * PS))
// Valores de duty cycle correspondentes a diferentes larguras de pulso


#define DUTY_RED 100
#define DUTY_GREEN 50
#define DUTY_BLUE 0

// TPM2 canal 0 - PTB18 - LED RED - led0
// TPM2 canal 1 - PTB19 - LED GREEN - led1
// TPM0 canal 1 - PTD1 - LED BLUE - led2

int main(void)
{
    uint16_t intensidade = 100;
    uint16_t duty_red  = TPM_MODULE*DUTY_RED/100;
    uint16_t duty_green  = TPM_MODULE*DUTY_GREEN/100;
    uint16_t duty_blue  = TPM_MODULE*DUTY_BLUE/100;

    // LED RED
    // Inicializa o módulo TPM2 com:
    // - base do TPMx
    // - fonte de clock PLL/FLL (TPM_CLK)
    // - valor do registrador MOD
    // - tipo de clock (TPM_CLK)
    // - prescaler de 1 a 128 (PS)
    // - modo de operação EDGE_PWM
    pwm_tpm_Init(TPM2, TPM_PLLFLL, TPM_MODULE, TPM_CLK, PS_128, EDGE_PWM);

    // Inicializa o canal 0 do TPM2 para gerar sinal PWM na porta GPIOB_18
    // - modo TPM_PWM_H (nível alto durante o pulso)
    pwm_tpm_Ch_Init(TPM2, 0, TPM_PWM_H, GPIOB, ledVermelho.pin);

    // Define o valor do duty cycle: nesse caso, duty_100 (LED quase desligado)
    pwm_tpm_CnV(TPM2, 0, duty_red);

    //LED GREEN
    //TPM2 ja ta inicializado
    pwm_tpm_Ch_Init(TPM2, 1, TPM_PWM_H, GPIOB, ledVerde.pin);
    pwm_tpm_CnV(TPM2, 1, duty_green);

    //LED GREEN
    pwm_tpm_Init(TPM0, TPM_PLLFLL, TPM_MODULE, TPM_CLK, PS_128, EDGE_PWM);
    pwm_tpm_Ch_Init(TPM0, 1, TPM_PWM_H, GPIOD, ledAzul.pin);
    pwm_tpm_CnV(TPM0, 1, duty_blue);

    //initLeds();

    char c;
    char buffer[10];
    int i = 0;
    uint16_t valor;

    while (1)
    {
        
        i = 0;
        while (i < sizeof(buffer) - 1) {
            // uart_poll_in retorna 0 quando lê um caractere com sucesso
            if (uart_poll_in(uart_dev, &c) == 0) {
                if (c == '\r' || c == '\n') break; // Fim da linha
                buffer[i] = c;
                i++;
            }
        }

        buffer[i] = '\0'; // Finaliza a string

        valor = atoi(buffer);

        if (valor >= 0 && valor <= 100)
        {
            intensidade = valor;
        }
        

        //atualiza duty cycle
        duty_red = (TPM_MODULE*DUTY_RED/100)*intensidade/100;
        duty_green = (TPM_MODULE*DUTY_GREEN/100)*intensidade/100;
        duty_blue = (TPM_MODULE*DUTY_BLUE/100)*intensidade/100;

        pwm_tpm_CnV(TPM2, 0, duty_red);
        pwm_tpm_CnV(TPM2, 1, duty_green);
        pwm_tpm_CnV(TPM0, 1, duty_blue);
    }
    

    return 0;
}