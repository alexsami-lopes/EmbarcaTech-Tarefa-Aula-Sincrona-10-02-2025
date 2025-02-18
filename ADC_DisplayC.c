// Embarcatec
// Tarefa - Aula Sincrona - 10/02/2025
// Discente: Alexsami Lopes

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/adc.h"     
#include "hardware/pwm.h"  
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define JOYSTICK_X_PIN 27  // GPIO para eixo X
#define JOYSTICK_Y_PIN 26  // GPIO para eixo Y
#define JOYSTICK_PB 22 // GPIO para botão do Joystick
#define Botao_A 5 // GPIO para botão A

#define VRX_PIN 26  
#define LED_PIN_GREEN 11  
#define LED_PIN_BLUE 12 
#define LED_PIN_RED 13  

#define BORDA_NORMAL 0  
#define BORDA_ZIGZAG 1 


//Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoB 6


static volatile uint32_t last_time = 0; // Armazena o tempo do último evento (em microssegundos)
uint pwm_slice_red = 0;  
uint pwm_slice_blue = 0; 
ssd1306_t ssd; // Inicializa a estrutura do display
bool cor = true;
bool leds_pwm_ativados = true;
bool leds_pwm_mudaram_de_estado = true;
bool tipo_borda = false;
uint borda_atual = 90;
// Função para lidar com a interrupção de ambos os botões
void gpio_irq_handler(uint gpio, uint32_t events)
{

  // Obtém o tempo atual em microssegundos
  uint32_t current_time = to_us_since_boot(get_absolute_time());
  
  // Verifica se passou tempo suficiente desde o último evento
  if (current_time - last_time > 200000) // 200 ms de debouncing
  {
    last_time = current_time; // Atualiza o tempo do último evento
    if (gpio == JOYSTICK_PB)
    { // Verifica se o botão A foi pressionado
      gpio_put(LED_PIN_GREEN, !gpio_get(LED_PIN_GREEN));
      printf("LED verde alternado!\n");
      if(borda_atual < 94) {
        borda_atual++; 
      } else {
        borda_atual = 90;
      }
    }
    if (gpio == Botao_A)
    { // Verifica se o botão B foi pressionado

      pwm_set_enabled(pwm_slice_red, 0);
      pwm_set_enabled(pwm_slice_blue, 0);
      leds_pwm_ativados = false;
      leds_pwm_mudaram_de_estado = true;

    }
    if (gpio == botaoB)
    { // Verifica se o botão B foi pressionado

      pwm_set_enabled(pwm_slice_red, 1);
      pwm_set_enabled(pwm_slice_blue, 1);
      leds_pwm_ativados = true;
      leds_pwm_mudaram_de_estado = true;

    }

    
  }
}

uint pwm_init_gpio(uint gpio, uint wrap) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);

    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_set_wrap(slice_num, wrap);
    
    pwm_set_enabled(slice_num, true);  
    return slice_num;  
}

int main()
{
  stdio_init_all();
  // Para ser utilizado o modo BOOTSEL com botão B
  gpio_init(botaoB);
  gpio_set_dir(botaoB, GPIO_IN);
  gpio_pull_up(botaoB);
  gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

  gpio_init(JOYSTICK_PB);
  gpio_set_dir(JOYSTICK_PB, GPIO_IN);
  gpio_pull_up(JOYSTICK_PB); 
  gpio_set_irq_enabled_with_callback(JOYSTICK_PB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

  gpio_init(Botao_A);
  gpio_set_dir(Botao_A, GPIO_IN);
  gpio_pull_up(Botao_A);
  gpio_set_irq_enabled_with_callback(Botao_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

  // I2C Initialisation. Using it at 400Khz.
  i2c_init(I2C_PORT, 400 * 1000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_pull_up(I2C_SDA); // Pull up the data line
  gpio_pull_up(I2C_SCL); // Pull up the clock line
  //ssd1306_t ssd; // Inicializa a estrutura do display
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(&ssd); // Configura o display
  ssd1306_send_data(&ssd); // Envia os dados para o display

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  adc_init();
  adc_gpio_init(JOYSTICK_X_PIN);
  adc_gpio_init(JOYSTICK_Y_PIN);  

  //adc_gpio_init(VRX_PIN); 
  gpio_init(LED_PIN_RED);
  gpio_set_dir(LED_PIN_RED, GPIO_OUT);
  pwm_set_gpio_level(LED_PIN_RED, 0);

  gpio_init(LED_PIN_GREEN);
  gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);  
  pwm_set_gpio_level(LED_PIN_GREEN, 0); 

  gpio_init(LED_PIN_BLUE);
  gpio_set_dir(LED_PIN_BLUE, GPIO_OUT);  
  pwm_set_gpio_level(LED_PIN_BLUE, 0); 

  uint pwm_wrap = 4096;  
  pwm_slice_red = pwm_init_gpio(LED_PIN_RED, pwm_wrap);  
  pwm_slice_blue = pwm_init_gpio(LED_PIN_BLUE, pwm_wrap);  
  
  uint32_t last_print_time = 0; 
  


  uint16_t adc_value_x = 0;
  uint16_t adc_value_y = 0;  
  uint8_t screen_x = 60;
  uint8_t screen_y = 29;
  char str_x[5];  // Buffer para armazenar a string
  char str_y[5];  // Buffer para armazenar a string  
  char str_xx[5];  // Buffer para armazenar a string
  char str_yy[5];  // Buffer para armazenar a string 
  
 //bool cor = true;

  while (true)
  {

    adc_select_input(0); // Seleciona o ADC para eixo Y. O pino 27 como entrada analógica
    adc_value_y = adc_read(); 
    adc_select_input(1); // Seleciona o ADC para eixo X. O pino 26 como entrada analógica
    adc_value_x = adc_read();   
    sprintf(str_x, "%d", adc_value_x);  // Converte o inteiro em string
    sprintf(str_y, "%d", adc_value_y);  // Converte o inteiro em string

    ssd1306_fill(&ssd, !cor); // Limpa o display
    if(borda_atual == 90) {
      ssd1306_rect(&ssd, 6, 6, 115, 51, cor, !cor); // Desenha um retângulo
    } else {
      ssd1306_border_character(&ssd, 0, 0, 128, 64, cor, (char) borda_atual); // Desenha um retângulo

    }    

    

    if(leds_pwm_mudaram_de_estado) {
      if(leds_pwm_ativados) {
        //ssd1306_artistic_border_triangles(&ssd, 0, 0, 122, 60, cor); // Desenha um retângulo
        printf("LEDs PWM ativados!\n");
        ssd1306_draw_string(&ssd, "LEDs PWM", 32, 8); // Desenha uma string
        ssd1306_draw_string(&ssd, "ativados", 32, 18); // Desenha uma string
        ssd1306_send_data(&ssd); // Atualiza o display
        sleep_ms(2000);
        leds_pwm_mudaram_de_estado = false;

      } else {

        printf("LEDs PWM desativados!\n");
        ssd1306_draw_string(&ssd, "LEDs PWM", 32, 8); // Desenha uma string
        ssd1306_draw_string(&ssd, "desativados", 20, 18); // Desenha uma string
        ssd1306_send_data(&ssd); // Atualiza o display
        sleep_ms(2000);
        leds_pwm_mudaram_de_estado = false;

      }
      
    }

    double screen_x16 = (((((double) adc_value_x)) / 4095) * 106) + 7;
    double screen_y16 = (((4095 - ((double) adc_value_y)) / 4095) * 42) + 7;  
    uint8_t screen_x = (uint8_t) screen_x16;
    uint8_t screen_y = (uint8_t) screen_y16;
    printf("adc_value_x: %u\n", adc_value_x); 
    printf("adc_value_y: %u\n", adc_value_y); 
    printf("screen_x: %u\n", screen_x); 
    printf("screen_y: %u\n", screen_y); 

    if (screen_x > 54 && screen_x < 64) {
      pwm_set_gpio_level(LED_PIN_RED, 0); 
    } else {
      pwm_set_gpio_level(LED_PIN_RED, adc_value_x); 
    }
    
    if (screen_y > 22 && screen_y < 32) {
      pwm_set_gpio_level(LED_PIN_BLUE, 0); 
    } else {
      pwm_set_gpio_level(LED_PIN_BLUE, 4095 - adc_value_y); 
    }

    // PARA TESTES:
    // sprintf(str_x, "%d", adc_value_x);  // Converte o inteiro em string
    // sprintf(str_y, "%d", adc_value_y);  // Converte o inteiro em string

    // sprintf(str_xx, "%d", screen_x);  // Converte o inteiro em string
    // sprintf(str_yy, "%d", screen_y);  // Converte o inteiro em string

    // ssd1306_draw_string(&ssd, str_x, 8, 44); // Desenha uma string          
    // ssd1306_draw_string(&ssd, str_y, 49, 44); // Desenha uma string   

    // ssd1306_draw_string(&ssd, str_xx, 8, 52); // Desenha uma string         
    // ssd1306_draw_string(&ssd, str_yy, 49, 52); // Desenha uma string   

    ssd1306_rect(&ssd, screen_y, screen_x, 8, 8, cor, 1); // Desenha um retângulo  
       
    ssd1306_send_data(&ssd); // Atualiza o display

    sleep_ms(100);
  }
}