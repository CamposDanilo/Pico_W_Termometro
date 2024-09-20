#include <stdio.h>
#include "pico/stdlib.h"
#include <string.h>
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"

////////////////////////////////////////////////
#define LCD_LIMPA_TELA     0x01
#define LCD_INICIA         0x02
#define LCD_ENTRYMODESET   0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_FUNCTIONSET    0x20

#define LCD_INICIO_ESQUERDA 0x02

#define LCD_LIGA_DISPLAY 0x04

#define LCD_16x2 0x08

#define LCD_BACKLIGHT 0x08
#define LCD_ENABLE_BIT 0x04

// By default these LCD display drivers are on bus address 0x27
#define BUS_ADDR 0x27

// Modes for lcd_envia_byte
#define LCD_CARACTER  1
#define LCD_COMANDO    0

#define MAX_LINES      2
#define MAX_CHARS      16

#define DELAY_US 600

void init_i2c(){
    i2c_init(i2c_default,100*1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN,GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN,GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
}

void lcd_envia_comando(uint8_t val) {
   i2c_write_blocking(i2c_default, BUS_ADDR, &val,1, false);
   
}

void lcd_pulsa_enable(uint8_t val) {
    sleep_us(DELAY_US);
    lcd_envia_comando(val | LCD_ENABLE_BIT);
    sleep_us(DELAY_US);
    lcd_envia_comando(val | ~LCD_ENABLE_BIT);
    sleep_us(DELAY_US);
   
}

void lcd_envia_byte(uint8_t caractere, int modo) {
   uint8_t nible_alto = modo | (caractere & 0xF0) | LCD_BACKLIGHT;
   uint8_t nible_baixo = modo | ((caractere << 4) & 0xF0) | LCD_BACKLIGHT;
   lcd_envia_comando(nible_alto);
   lcd_pulsa_enable(nible_alto);
   lcd_envia_comando(nible_baixo);
   lcd_pulsa_enable(nible_baixo);
}

void lcd_limpa_tela(void) {
    lcd_envia_byte(LCD_LIMPA_TELA, LCD_COMANDO);
}

void lcd_posiciona_cursor(int linha, int coluna) {
   uint8_t aux = (linha == 0)? 0x80 + coluna : 0xC0 + coluna;
   lcd_envia_byte(aux, LCD_COMANDO);
}

void lcd_envia_caracter(char caractere) {
    lcd_envia_byte(caractere,LCD_CARACTER);
    
}

void lcd_envia_string(const char *s) {
    while(*s){
        lcd_envia_caracter(*s++);
    }
    
}

void lcd_init() {
    lcd_envia_byte(LCD_INICIA,LCD_COMANDO);
    lcd_envia_byte(LCD_INICIA | LCD_LIMPA_TELA, LCD_COMANDO);
    lcd_envia_byte(LCD_ENTRYMODESET | LCD_INICIO_ESQUERDA, LCD_COMANDO);
    lcd_envia_byte(LCD_FUNCTIONSET | LCD_16x2, LCD_COMANDO);
    lcd_envia_byte(LCD_DISPLAYCONTROL | LCD_LIGA_DISPLAY, LCD_COMANDO);
    lcd_limpa_tela();
}
/////////////////////////////////////////////////

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

void init_spi(){
    // SPI initialisation. This example will use SPI at 1MHz.
    spi_init(SPI_PORT, 5000*1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS,   GPIO_FUNC_SIO);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    
    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    // For more examples of SPI use see https://github.com/raspberrypi/pico-examples/tree/master/spi
}

float leitura_termopar(){
    uint8_t dados[4];
    gpio_put(PIN_CS, 0);
    sleep_us(10);
    spi_read_blocking(SPI_PORT,0,dados,4);
    gpio_put(PIN_CS,1);
    sleep_us(10);
    //verificar erros
    

    if (dados[1] & 0b01){
        printf("falha no sensor\n\n");
        if (dados[3] & 0b01){
            printf("circuito aberto\n\n");
            return 0;
        }

        if (dados[3] & 0b010){
            printf("Termopar em curto GND\n\n");
            return 0;
        }
        if (dados[3] & 0b0100){
            printf("Termopar em curto VCC \n\n");
            return 0;
        }
    }

    float parte_decimal = ((dados[1] & 0b00001100)>> 2)* 0.25;

    int parte_inteira = dados[1] >> 4;
    parte_inteira = parte_inteira | (dados[0] << 4);
    return parte_inteira + parte_decimal;
}


int main()
{   ////////////////////////////////////////
    stdio_init_all();
    init_i2c();
    init_spi();
    leitura_termopar();

    lcd_init();
    lcd_posiciona_cursor(0,0);
    lcd_envia_string("TERMOMETRO");
    lcd_posiciona_cursor(1,0);
    lcd_envia_string("--------------");
    sleep_ms(3000);
    /////////////////////////////////////////////
    
   
    while (true) {
        float resultado = leitura_termopar();
        lcd_limpa_tela();
        lcd_posiciona_cursor(0,0);
        lcd_envia_string("TEMPERATURA:");

        lcd_posiciona_cursor(1,0);
        char convertido[16];
        sprintf(convertido,"%.2f",resultado);
        //lcd_envia_caracter(convertido);
        printf("%s\n\n",convertido);
        int i;
        for (i=0; convertido[i] != '\0'; i++){
            lcd_envia_caracter(convertido[i]);
        }
      
        lcd_posiciona_cursor(1,6);
        lcd_envia_string("Celsius");
        
        
        sleep_ms(1000);
    }
}
