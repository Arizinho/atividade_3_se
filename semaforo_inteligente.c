#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include <stdio.h>

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

#define LED_RED 13
#define LED_GREEN 11
#define LED_BLUE 12
#define BUZZER 10
#define BUTTON_A 5
#define OUT_PIN 7 // GPIO matriz de leds

#define BIT0 4
#define BIT1 20
#define BIT2 19
#define BIT3 18


volatile bool modo = 1; //flag boleano para indicar modo (1 = padrão, 0 = modo noite)
volatile uint8_t estado = 0; //inteiro para indicar estado do semáforo (0 = vermelho, 1 = verde, 2 = amarelo)

void gpio_irq_handler(uint gpio, uint32_t events);
void valor_bin(uint8_t numero);

//tarefa para controle dos LEDs do semáforo
void vSemaforoLedTask()
{
    bool alterna_led = 0; //boleano para piscar led no modo noite
    uint32_t last_time = 0; //variável para guardar o tempo da última alteração de cor do semáforo

    //inicialização do LED RGB
    gpio_init(LED_RED);
    gpio_init(LED_GREEN);
    gpio_init(LED_BLUE);
    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_set_dir(LED_BLUE, GPIO_OUT);

    while (true)
    {
        //inicializa semáforo em modo padrão
        last_time = to_ms_since_boot(get_absolute_time()); //salva valor de tempo
        //cor inicial -> vermelha
        estado = 0;
        gpio_put(LED_RED, 1);
        gpio_put(LED_GREEN, 0);
        gpio_put(LED_BLUE, 0);

        //looping para modo padrão de funcionamento do semáforo
        while(modo){
            //mantém sinal vermelho durante 15s
            if (estado == 0 && (to_ms_since_boot(get_absolute_time())-last_time) > 9000)
            {
                last_time = to_ms_since_boot(get_absolute_time()); //atualiza tempo

                //altera cor do sinal para verde
                gpio_put(LED_RED, 0);
                gpio_put(LED_GREEN, 1);
                gpio_put(LED_BLUE, 0);

                estado = (estado + 1)%3; //altera estado
            }

            //mantém sinal verde durante 10s
            else if (estado == 1 && (to_ms_since_boot(get_absolute_time())-last_time) > 6000)
            {
                last_time = to_ms_since_boot(get_absolute_time()); //atualiza tempo

                //altera cor do sinal para amarelo
                gpio_put(LED_RED, 1);
                gpio_put(LED_GREEN, 1);
                gpio_put(LED_BLUE, 0);
                
                estado = (estado + 1)%3; //altera estado
            }

            //mantém sinal amarelo durante 5s
            else if (estado == 2 && (to_ms_since_boot(get_absolute_time())-last_time) > 3000)
            {
                last_time = to_ms_since_boot(get_absolute_time()); //atualiza tempo

                //altera cor do sinal para vermelho
                gpio_put(LED_RED, 1);
                gpio_put(LED_GREEN, 0);
                gpio_put(LED_BLUE, 0);

                estado = (estado + 1)%3; //altera estado
            }

            vTaskDelay(pdMS_TO_TICKS(30));
        }

        //inicializa semaforo para modo noturno
        last_time = to_ms_since_boot(get_absolute_time()); //salva tempo atual
        alterna_led = 1;
        gpio_put(LED_RED, 1);
        gpio_put(LED_GREEN, 1);
        gpio_put(LED_BLUE, 0);

        //looping de modo noturno
        while(!modo) {
            //pisca led em cor amarela com 1s apagado e 1s aceso
            if (to_ms_since_boot(get_absolute_time())-last_time > 1000)
            {
                last_time = to_ms_since_boot(get_absolute_time());
                alterna_led = !alterna_led;
                gpio_put(LED_RED, alterna_led);
                gpio_put(LED_GREEN, alterna_led); 
            }
            vTaskDelay(pdMS_TO_TICKS(30));
        }
        
    }
}

//tarefa para sinal sonoro do semáforo
void vSinaisSonorosTask()
{
    //flag para indicar que buzzer está on
    bool buzzer_on = 0;

    //variavel para salvar o tempo atual
    uint32_t last_time_buzzer = 0;

    //inicializa PWM
    gpio_set_function(BUZZER, GPIO_FUNC_PWM);
    pwm_set_clkdiv(5, 125.0); 
    pwm_set_wrap(5, 1999); 
    pwm_set_gpio_level(BUZZER, 0); 
    pwm_set_enabled(5, true); 

    while (true)
    {
        //atualiza tempo
        last_time_buzzer = to_ms_since_boot(get_absolute_time());
        pwm_set_gpio_level(BUZZER, 1000);
        buzzer_on = 1;

        //looping para modo padrão de funcionamento do semáforo
        while (modo){
            switch (estado)
            {
            //caso sinal vermelho -> sinal sonoro durante 0,5s e 1,5s de silêncio
            case 0:
                if(buzzer_on && (to_ms_since_boot(get_absolute_time())-last_time_buzzer) > 500){
                    last_time_buzzer = to_ms_since_boot(get_absolute_time());
                    buzzer_on = 0;
                    pwm_set_gpio_level(BUZZER, 0);
                }
                else if(!buzzer_on && (to_ms_since_boot(get_absolute_time())-last_time_buzzer) > 1500){
                    last_time_buzzer = to_ms_since_boot(get_absolute_time());
                    buzzer_on = 1;
                    pwm_set_gpio_level(BUZZER, 1000);
                }
                break;
            
            //caso sinal verde -> sinal sonoro durante 1s e 1s de silêncio
            case 1:
                if(buzzer_on && (to_ms_since_boot(get_absolute_time())-last_time_buzzer) > 1000){
                    last_time_buzzer = to_ms_since_boot(get_absolute_time());
                    buzzer_on = 0;
                    pwm_set_gpio_level(BUZZER, 0);
                }
                else if(!buzzer_on && (to_ms_since_boot(get_absolute_time())-last_time_buzzer) > 1000){
                    last_time_buzzer = to_ms_since_boot(get_absolute_time());
                    buzzer_on = 1;
                    pwm_set_gpio_level(BUZZER, 1000);
                }
                break;

            //caso sinal amarelo -> sinal sonoro durante 0,5s e 0,5s de silêncio
            case 2:
                if(buzzer_on && (to_ms_since_boot(get_absolute_time())-last_time_buzzer) > 500){
                    last_time_buzzer = to_ms_since_boot(get_absolute_time());
                    buzzer_on = 0;
                    pwm_set_gpio_level(BUZZER, 0);
                }
                else if(!buzzer_on && (to_ms_since_boot(get_absolute_time())-last_time_buzzer) > 500){
                    last_time_buzzer = to_ms_since_boot(get_absolute_time());
                    buzzer_on = 1;
                    pwm_set_gpio_level(BUZZER, 1000);
                }
            }
            vTaskDelay(pdMS_TO_TICKS(30));
        }

        //looping para modo noturno
        while (!modo){
            //emite sinal sonoro durante 1,75s e 0,25s de silêncio
            if(buzzer_on && (to_ms_since_boot(get_absolute_time())-last_time_buzzer) > 1750){
                last_time_buzzer = to_ms_since_boot(get_absolute_time());
                buzzer_on = 0;
                pwm_set_gpio_level(BUZZER, 0);
            }
            else if(!buzzer_on && (to_ms_since_boot(get_absolute_time())-last_time_buzzer) > 250){
                last_time_buzzer = to_ms_since_boot(get_absolute_time());
                buzzer_on = 1;
                pwm_set_gpio_level(BUZZER, 1000);
            }
            vTaskDelay(pdMS_TO_TICKS(30));
        }
    }
}

//tarefa para mostrar informações no display
void vDisplayTask()
{
    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA);                                        // Pull up the data line
    gpio_pull_up(I2C_SCL);                                        // Pull up the clock line
    ssd1306_t ssd;                                                // Inicializa a estrutura do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd);                                         // Configura o display
    ssd1306_send_data(&ssd);                                      // Envia os dados para o display
    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    while (true)
    {   
        //modo padrão do semáforo
        if (modo)
        {
            switch (estado)
            {
            case 0:
                ssd1306_fill(&ssd, false);                          // Limpa o display
                ssd1306_draw_string(&ssd, "PARE!", 45, 10); // Desenha uma string
                ssd1306_draw_string(&ssd, "Sinal Vermelho.", 3, 25); // Desenha uma string
                ssd1306_send_data(&ssd); 
                break;
            
            case 1:
                ssd1306_fill(&ssd, false);                          // Limpa o display
                ssd1306_draw_string(&ssd, "SIGA!", 45, 10); // Desenha uma string
                ssd1306_draw_string(&ssd, "Sinal Verde.", 20, 25); // Desenha uma string
                ssd1306_send_data(&ssd);
                break;

            case 2:
                ssd1306_fill(&ssd, false);                          // Limpa o display
                ssd1306_draw_string(&ssd, "ATENCAO!", 30, 10); // Desenha uma string
                ssd1306_draw_string(&ssd, "Sinal Amarelo.", 10, 25); // Desenha uma string
                ssd1306_send_data(&ssd);
                break;
            }
        }

        //modo noturno
        else{
            ssd1306_fill(&ssd, false);                          // Limpa o display
            ssd1306_draw_string(&ssd, "Modo noturno ON", 3, 20); // Desenha uma string
            ssd1306_send_data(&ssd); 
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

//tarefa para mostrar configurar 7segmentos
void v7SegmentosTask(){
    uint32_t last_time = 0;
    uint8_t count = 0;
    uint8_t ultimo_estado = 3;
    gpio_init(BIT0);
    gpio_init(BIT1);
    gpio_init(BIT2);
    gpio_init(BIT3);
    gpio_set_dir(BIT0, GPIO_OUT);
    gpio_set_dir(BIT1, GPIO_OUT);
    gpio_set_dir(BIT2, GPIO_OUT);
    gpio_set_dir(BIT3, GPIO_OUT);

    while (true)
    {
        if(modo){

            if(estado !=ultimo_estado){
                last_time = to_ms_since_boot(get_absolute_time());
                ultimo_estado = estado;
                switch (estado){
                    case 0:
                        count = 9;
                        break;

                    case 1:
                        count = 6;
                        break;

                    case 2:
                        count = 3;
                        break;
                }
                valor_bin(count);
            }

            if (to_ms_since_boot(get_absolute_time())-last_time > 1000) {
                last_time = to_ms_since_boot(get_absolute_time());
                count --;
                valor_bin(count);
            }   

        }
        else{
            ultimo_estado = 3;
            valor_bin(0);
        }
        vTaskDelay(pdMS_TO_TICKS(30));

    } 

}

int main()
{
    stdio_init_all();

    //inicializa botão A
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    xTaskCreate(vSemaforoLedTask, "Semaforo Led Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vSinaisSonorosTask, "Sinal Sonoro Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vDisplayTask, "Exibe Display Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(v7SegmentosTask, "Configura Display 7 Segmentos Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);   
    vTaskStartScheduler();
    panic_unsupported();
}

//tarefa para alterar modo por flag
void gpio_irq_handler(uint gpio, uint32_t events) {
    static uint32_t last_time = 0;
    
    //debounce de 200 ms para os botões
    if (to_ms_since_boot(get_absolute_time())-last_time > 200) {
        last_time = to_ms_since_boot(get_absolute_time());
        modo = !modo;
    }
}

void valor_bin(uint8_t numero){
    gpio_put(BIT0, (numero >> 0) & 1);
    gpio_put(BIT1, (numero >> 1) & 1);
    gpio_put(BIT2, (numero >> 2) & 1);
    gpio_put(BIT3, (numero >> 3) & 1);
}