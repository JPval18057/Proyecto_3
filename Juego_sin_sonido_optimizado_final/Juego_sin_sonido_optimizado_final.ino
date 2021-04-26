//***************************************************************************************************************************************
/* Librería para el uso de la pantalla ILI9341 en modo 8 bits
 * Basado en el código de martinayotte - https://www.stm32duino.com/viewtopic.php?t=637
 * Adaptación, migración y creación de nuevas funciones: Pablo Mazariegos y José Morales
 * Con ayuda de: José Guerra
 * IE3027: Electrónica Digital 2 - 2019
 */
//***************************************************************************************************************************************
//TO DO LIST
/*
 * HACER ANIMACIÓN DE GANADOR (CON SONIDO) 
 * COMUNICAR LA TIVA CON MI ARDUINO MEGA CON SERIAL2
 * CARGAR EL CÓDIGO DE KATHY AL ARDUINO Y AJUSTAR
 */
//***************************************************************************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <TM4C123GH6PM.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"

#include "bitmaps.h"
#include "font.h"
#include "lcd_registers.h"

//LIBRERIAS PARA LA TARGETA SD
#include <SPI.h>
#include <SD.h>

//PINES PANTALLA
#define LCD_RST PD_0
#define LCD_CS PD_1
#define LCD_RS PD_2
#define LCD_WR PD_3
#define LCD_RD PE_1

//PINES SD
#define CSS PA_3 //CHIP SELECT
#define MOSI PA_5
#define MISO PA_4
#define CLOCK PA_2

// Color definitions more in https://ee-programming-notepad.blogspot.com/2016/10/16-bit-color-generator-picker.html
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x3E7C      //0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0x60DF      //0xFFE0  
#define WHITE           0xFFFF

//JUGADORES
#define UP1         PF_4 //PUSH1
#define CONTROL1    A3
#define CONTROL2    A0
#define DOWN2       PC_7
//PUSH2 = PF_0
//EFECTOS DE SONIDO
#define BUZZER      

//GLOBAL VARIABLES
int DPINS[] = {PB_0, PB_1, PB_2, PB_3, PB_4, PB_5, PB_6, PB_7};  
int start = 0;
int reset_dir[] = {-10, 10, -10, -10, 10, 10, -10, 10, 10, -10};


//VELOCIDADES
unsigned long PADDLE_RATE = 2000; //usaré la función micros();
unsigned long BALL_RATE = 10;
unsigned long currentTime = 0;    //tiene el mismo valor que millis();
unsigned long currentTime_paddle = 0;
unsigned long previousTime = 0;   //se modifica constantemente
unsigned long previousTime_paddle = 0; //se usa para comprobar el tiempo de los jugadores
unsigned long refreshRate = 100;  //tiempo que tarda en mostrar una imágen


//GRAFICOS
const uint8_t PADDLE_HEIGHT = 24;
unsigned int ball_width = 5;
unsigned int ball_height = 5;
unsigned int y_limit=239-50;
unsigned int x_limit=319-10;
unsigned short POT1 = 0;
unsigned short POT2 = 0;


//COLORES
unsigned short ColorP1 = BLUE; //DEFAULT COLOR BLUE
unsigned short ColorP2 = RED; //DEFAULT P2 IS RED
unsigned short BallColor = WHITE; //DEFAULT BALL IS WHITE



//DIRECCION DE LA PELOTA
short ball_x = 340/2, ball_y = 240/2;
short ball_dir_x = 10;
short ball_dir_y = -10;
short ball_x_old = 0;
short ball_y_old = 0;


//controles del juego
boolean gameIsRunning = true;
boolean resetBall = false;
boolean gameOverMessage = true;
boolean saveGametoMemory = false;

//PUNTAJE
int PLAYER1 = 0;
int PLAYER2 = 0;
int PLAYER1_old = 0;
int PLAYER2_old = 0;
int MAX_SCORE = 4; //PUNTAJE MÁXIMO
int WINNER = 0;

//JUGADOR1
int P1_WIDTH = 5;
int P1_HEIGHT = 60; //40 es el valor ideal
int X1 = 20;
int Y1 = 240/2;
int Y1_old = 0;


//JUGADOR2
int P2_WIDTH = 5;
int P2_HEIGHT = 60; //40 es el valor ideal
int X2 = 280; //280 es el valor correcto
int Y2 = 240/2;
int Y2_old = 0;

//MENSAJES
String winner_P1 = "GANA JUGADOR 1";
String winner_P2 = "GANA JUGADOR 2";

//***************************************************************************************************************************************
// Functions Prototypes
//***************************************************************************************************************************************
void LCD_Init(void);
void LCD_CMD(uint8_t cmd);
void LCD_DATA(uint8_t data);
void SetWindows(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2);
void LCD_Clear(unsigned int c);
void H_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c);
void V_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c);
void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c);
void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c);
void LCD_Print(String text, int x, int y, int fontSize, int color, int background);

void LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char bitmap[]);
void LCD_Sprite(int x, int y, int width, int height, unsigned char bitmap[],int columns, int index, char flip, char offset);

//motor de físicas
void update_math(void);

//jugadores
void paddle1(int x, int y, int height);
void paddle2(int x, int y, int height);
void update_paddles(void);

//ANIMACION DE GANADOR
void GAME_WINNER(int winner);

//Selección de personaje
void CharMenu(void);

//GUARDAR EN LA MEMORIA SD
void saveGame(int winner);

extern uint8_t fondo[];
String title="Dios Maya";
String title2="de la";
String title3="Funacion";
//OBJETOS
//objeto tipo FILE para leer/escribir archivos
File file;

//***************************************************************************************************************************************
// Inicialización
//***************************************************************************************************************************************
void setup() {
  SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
  //COMUNICACIÓN SERIAL
  Serial.begin(115200);
  //MODULO SPI PARA TARGETAS SD
  SPI.setModule(0); //indicamos que queremos usar el módulo 0
  pinMode(CSS, OUTPUT);
  //VERIFICAR SI LEE LA TARGETA 
  while(1){
    if (SD.begin(CSS)==0){
    Serial.println("No se pudo iniciar la targeta SD");
    } else {
    Serial.println("Se inició la targeta SD");  
    break;
    }
  }

  
  /*
   * CÓDIGO QUE COMUNICA LOS DOS MICROCONTROLADORES
  */
  /*
  Serial2.begin(250000);
  Serial2.println("C");
  */

  //INICIALIZACION DE LA PANTALLA
  
  GPIOPadConfigSet(GPIO_PORTB_BASE, 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU);
  //Serial.println("Inicio"); //no se usa en este programa
  LCD_Init();
  LCD_Clear(0x00);

  //CONFIGURACION IO
  pinMode(PUSH1, INPUT_PULLUP);
  pinMode(CONTROL1, INPUT);
  pinMode(CONTROL2, INPUT);

  //INICIO DEL JUEGO
  CharMenu();
  //BORRAMOS TODO
  FillRect(0, 0, 319, 206, 0x421b);
  String text1 = "Super Mario World!";
  LCD_Print(title, 90, 80, 2, 0xffff, 0x421b);
  LCD_Print(title2, 113, 110, 2, 0xffff, 0x421b);
  LCD_Print(title3, 90, 140, 2, 0xffff, 0x421b);
  delay(1000);
  FillRect(0, 0, 319, 228, WHITE);    
  LCD_Print(title, 90, 80, 2, 0xffff, BLACK);
  LCD_Print(title2, 113, 110, 2, 0xffff, BLACK);
  LCD_Print(title3, 90, 140, 2, 0xffff, BLACK);
  delay(1000);
  LCD_Clear(0x00);

  //DIBUJAMOS LA CANCHA
  Rect(1, 50, x_limit, y_limit, WHITE);
  //DIBUJAMOS EL TABLERO DE PUNTOS
  LCD_Print(String(PLAYER1), X1+50, 10, 2, ColorP1, WHITE);
  LCD_Print(String(PLAYER2), X2-50, 10, 2, ColorP2, WHITE);
  Rect(1, 50, x_limit, y_limit, WHITE);

  //PRUEBAS DEL MENÚ DEL JUEGO
  

}

































































//***************************************************************************************************************************************
// Loop Infinito
//***************************************************************************************************************************************
void loop() {    
    if (gameIsRunning==true){
      //LO PRIMERO QUE HACEMOS ES CONTAR EL TIEMPO
      currentTime = millis();//contamos el tiempo siempre    
      currentTime_paddle = micros();
        //320 x 240 dimensiones máximas pantalla
      if (Y1_old!=Y1){
        //CUANDO HAY UN CAMBIO DE COORDENADAS BORRAMOS EL JUGADOR VIEJO
        FillRect(X1, Y1_old, P1_WIDTH, P1_HEIGHT, BLACK);
        //DIBUJAMOS JUGADOR1 NUEVO
        FillRect(X1, Y1, P1_WIDTH, P1_HEIGHT, ColorP1);
        //ACTUALIZAMOS LA POSICION VIEJA
        Y1_old = Y1; 
      }
      if (Y2_old!=Y2){
        //BORRAMOS EL JUGADOR VIEJO CUANDO HAY UN CAMBIO
        FillRect(X2, Y2_old, P2_WIDTH, P2_HEIGHT, BLACK);
        //DIBUJAMOS JUGADOR2 NUEVO
        FillRect(X2, Y2, P2_WIDTH, P2_HEIGHT, ColorP2);
        //ACTUALIZAMOS LA POSICION VIEJA
        Y2_old = Y2;
      }
      //Solo se dibuja la pelota cuando la posición cambia (cualquier cambio en las coordenadas)
      if (ball_x_old!=ball_x | ball_y_old!=ball_y){
        //CUANDO HAY UN CAMBIO EN LAS COORDENADAS BORRAMOS LA PELOTA VIEJA
        FillRect(ball_x_old, ball_y_old, ball_width, ball_height, BLACK); //borramos la pelota
        //Dibujamos la pelota nueva
        FillRect(ball_x, ball_y, ball_width, ball_height, BallColor);
        ball_x_old = ball_x;
        ball_y_old = ball_y;
      }
      if (PLAYER1_old!=PLAYER1){
        //DIBUJAMOS EL PUNTEO ACTUAL SI HAY UN CAMBIO
        LCD_Print(String(PLAYER1), X1+50, 10, 2, ColorP1, WHITE);
        PLAYER1_old=PLAYER1; //ACTUALIZAMOS EL PUNTEO VIEJO
      }
      if (PLAYER2_old!=PLAYER2){
        //DIBUJAMOS EL PUNTEO ACTUAL SI HAY UN CAMBIO EN EL PUNTEO
        LCD_Print(String(PLAYER2), X2-50, 10, 2, ColorP2, WHITE);
        PLAYER2_old = PLAYER2; //ACTUALIZAMOS EL PUNTEO VIEJO
      }
      //ACTUALIZAMOS LOS DATOS DE ÚLTIMO PARA QUE AL INICIAR EL OTRO JUEGO NO INICIEN MAL
      if ((currentTime - previousTime)>=refreshRate){
        //este código se ejecuta cada 100ms        
        update_math();//actualizamos cada 100ms

        previousTime = currentTime;
      }


      //ACTUALIZAMOS LA POSICIÓN DE LOS JUGADORES
      if ((currentTime_paddle - previousTime_paddle)>=PADDLE_RATE){
        update_paddles();//ACTUALIZAMOS LA POSICIÓN DE LOS JUGADORES
        
        previousTime_paddle = currentTime;
      }

      
      
    } else {
      if (resetBall==true){
        //GAME_WINNER(WINNER);
        resetBall=false;
      }      
      if (gameOverMessage==true){
        LCD_Clear(0x00);
        LCD_Print("Presiona el boton 1", 20, 80, 2, 0xffff, BLACK);
        LCD_Print("para reiniciar", 30, 100, 2, 0xffff, BLACK);
        gameOverMessage = false;
      }
     if (resetBall==false & gameIsRunning==false & digitalRead(PUSH1)==0){
      //reiniciar el juego si se presiona PUSH1
      resetBall=false;
      gameIsRunning=true;
      gameOverMessage=true;
      LCD_Clear(BLACK);
      //Reiniciar la pelota
      ball_x = 320/2;
      ball_y = 240/2;
      ball_dir_x = reset_dir[random(0,5)];
      ball_dir_y = reset_dir[random(0,5)];
      //Reinicio del juego
      CharMenu(); //MOSTRAMOS EL MENÚ DE NUEVO
      //ANIMACION DE INICIO
      LCD_Print(title, 90, 80, 2, 0xffff, 0x421b);
      LCD_Print(title2, 113, 110, 2, 0xffff, 0x421b);
      LCD_Print(title3, 90, 140, 2, 0xffff, 0x421b);
      delay(1000);
      FillRect(0, 0, 319, 228, WHITE);    
      LCD_Print(title, 90, 80, 2, 0xffff, BLACK);
      LCD_Print(title2, 113, 110, 2, 0xffff, BLACK);
      LCD_Print(title3, 90, 140, 2, 0xffff, BLACK);
      delay(1000);
      LCD_Clear(0x00);
    
      //DIBUJAMOS LA CANCHA
      Rect(1, 50, x_limit, y_limit, WHITE);
      //DIBUJAMOS EL TABLERO DE PUNTOS
      LCD_Print(String(PLAYER1), X1+50, 10, 2, ColorP1, WHITE);
      LCD_Print(String(PLAYER2), X2-50, 10, 2, ColorP2, WHITE);
      Rect(1, 50, x_limit, y_limit, WHITE);
      
    } 
    }
    
    
    
}















































//***************************************************************************************************************************************
// Función para inicializar LCD
//***************************************************************************************************************************************
void LCD_Init(void) {
  pinMode(LCD_RST, OUTPUT);
  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_RS, OUTPUT);
  pinMode(LCD_WR, OUTPUT);
  pinMode(LCD_RD, OUTPUT);
  for (uint8_t i = 0; i < 8; i++){
    pinMode(DPINS[i], OUTPUT);
  }
  //****************************************
  // Secuencia de Inicialización
  //****************************************
  digitalWrite(LCD_CS, HIGH);
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_WR, HIGH);
  digitalWrite(LCD_RD, HIGH);
  digitalWrite(LCD_RST, HIGH);
  delay(5);
  digitalWrite(LCD_RST, LOW);
  delay(20);
  digitalWrite(LCD_RST, HIGH);
  delay(150);
  digitalWrite(LCD_CS, LOW);
  //****************************************
  LCD_CMD(0xE9);  // SETPANELRELATED
  LCD_DATA(0x20);
  //****************************************
  LCD_CMD(0x11); // Exit Sleep SLEEP OUT (SLPOUT)
  delay(100);
  //****************************************
  LCD_CMD(0xD1);    // (SETVCOM)
  LCD_DATA(0x00);
  LCD_DATA(0x71);
  LCD_DATA(0x19);
  //****************************************
  LCD_CMD(0xD0);   // (SETPOWER) 
  LCD_DATA(0x07);
  LCD_DATA(0x01);
  LCD_DATA(0x08);
  //****************************************
  LCD_CMD(0x36);  // (MEMORYACCESS)
  LCD_DATA(0x40|0x80|0x20|0x08); // LCD_DATA(0x19);
  //****************************************
  LCD_CMD(0x3A); // Set_pixel_format (PIXELFORMAT)
  LCD_DATA(0x05); // color setings, 05h - 16bit pixel, 11h - 3bit pixel
  //****************************************
  LCD_CMD(0xC1);    // (POWERCONTROL2)
  LCD_DATA(0x10);
  LCD_DATA(0x10);
  LCD_DATA(0x02);
  LCD_DATA(0x02);
  //****************************************
  LCD_CMD(0xC0); // Set Default Gamma (POWERCONTROL1)
  LCD_DATA(0x00);
  LCD_DATA(0x35);
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0x02);
  //****************************************
  LCD_CMD(0xC5); // Set Frame Rate (VCOMCONTROL1)
  LCD_DATA(0x04); // 72Hz
  //****************************************
  LCD_CMD(0xD2); // Power Settings  (SETPWRNORMAL)
  LCD_DATA(0x01);
  LCD_DATA(0x44);
  //****************************************
  LCD_CMD(0xC8); //Set Gamma  (GAMMASET)
  LCD_DATA(0x04);
  LCD_DATA(0x67);
  LCD_DATA(0x35);
  LCD_DATA(0x04);
  LCD_DATA(0x08);
  LCD_DATA(0x06);
  LCD_DATA(0x24);
  LCD_DATA(0x01);
  LCD_DATA(0x37);
  LCD_DATA(0x40);
  LCD_DATA(0x03);
  LCD_DATA(0x10);
  LCD_DATA(0x08);
  LCD_DATA(0x80);
  LCD_DATA(0x00);
  //****************************************
  LCD_CMD(0x2A); // Set_column_address 320px (CASET)
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0x3F);
  //****************************************
  LCD_CMD(0x2B); // Set_page_address 480px (PASET)
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0xE0);
//  LCD_DATA(0x8F);
  LCD_CMD(0x29); //display on 
  LCD_CMD(0x2C); //display on

  LCD_CMD(ILI9341_INVOFF); //Invert Off
  delay(120);
  LCD_CMD(ILI9341_SLPOUT);    //Exit Sleep
  delay(120);
  LCD_CMD(ILI9341_DISPON);    //Display on
  digitalWrite(LCD_CS, HIGH);
}
//***************************************************************************************************************************************
// Función para enviar comandos a la LCD - parámetro (comando)
//***************************************************************************************************************************************
void LCD_CMD(uint8_t cmd) {
  digitalWrite(LCD_RS, LOW);
  digitalWrite(LCD_WR, LOW);
  GPIO_PORTB_DATA_R = cmd;
  digitalWrite(LCD_WR, HIGH);
}
//***************************************************************************************************************************************
// Función para enviar datos a la LCD - parámetro (dato)
//***************************************************************************************************************************************
void LCD_DATA(uint8_t data) {
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_WR, LOW);
  GPIO_PORTB_DATA_R = data;
  digitalWrite(LCD_WR, HIGH);
}
//***************************************************************************************************************************************
// Función para definir rango de direcciones de memoria con las cuales se trabajara (se define una ventana)
//***************************************************************************************************************************************
void SetWindows(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2) {
  LCD_CMD(0x2a); // Set_column_address 4 parameters
  LCD_DATA(x1 >> 8);
  LCD_DATA(x1);   
  LCD_DATA(x2 >> 8);
  LCD_DATA(x2);   
  LCD_CMD(0x2b); // Set_page_address 4 parameters
  LCD_DATA(y1 >> 8);
  LCD_DATA(y1);   
  LCD_DATA(y2 >> 8);
  LCD_DATA(y2);   
  LCD_CMD(0x2c); // Write_memory_start
}
//***************************************************************************************************************************************
// Función para borrar la pantalla - parámetros (color)
//***************************************************************************************************************************************
void LCD_Clear(unsigned int c){  
  unsigned int x, y;
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW);   
  SetWindows(0, 0, 319, 239); // 479, 319);
  for (x = 0; x < 320; x++)
    for (y = 0; y < 240; y++) {
      LCD_DATA(c >> 8); 
      LCD_DATA(c); 
    }
  digitalWrite(LCD_CS, HIGH);
} 
//***************************************************************************************************************************************
// Función para dibujar una línea horizontal - parámetros ( coordenada x, cordenada y, longitud, color)
//*************************************************************************************************************************************** 
void H_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {  
  unsigned int i, j;
  LCD_CMD(0x02c); //write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW);
  l = l + x;
  SetWindows(x, y, l, y);
  j = l;// * 2;
  for (i = 0; i < l; i++) {
      LCD_DATA(c >> 8); 
      LCD_DATA(c); 
  }
  digitalWrite(LCD_CS, HIGH);
}
//***************************************************************************************************************************************
// Función para dibujar una línea vertical - parámetros ( coordenada x, cordenada y, longitud, color)
//*************************************************************************************************************************************** 
void V_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {  
  unsigned int i,j;
  LCD_CMD(0x02c); //write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW);
  l = l + y;
  SetWindows(x, y, x, l);
  j = l; //* 2;
  for (i = 1; i <= j; i++) {
    LCD_DATA(c >> 8); 
    LCD_DATA(c);
  }
  digitalWrite(LCD_CS, HIGH);  
}
//***************************************************************************************************************************************
// Función para dibujar un rectángulo - parámetros ( coordenada x, cordenada y, ancho, alto, color)
//***************************************************************************************************************************************
void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c) {
  H_line(x  , y  , w, c);
  H_line(x  , y+h, w, c);
  V_line(x  , y  , h, c);
  V_line(x+w, y  , h, c);
}
//***************************************************************************************************************************************
// Función para dibujar un rectángulo relleno - parámetros ( coordenada x, cordenada y, ancho, alto, color)
//***************************************************************************************************************************************
void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c) {
  unsigned int i;
  //H_line(x  , y  , w, c);
  for (i = 0; i < h; i++) {    
    H_line(x  , y+i, w, c);
  }
}


/*void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c) {
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW); 
  
  unsigned int x2, y2;
  x2 = x+w;
  y2 = y+h;
  SetWindows(x, y, x2-1, y2-1);
  unsigned int k = w*h*2-1;
  unsigned int i, j;
  for (int i = 0; i < w; i++) {
    for (int j = 0; j < h; j++) {
      LCD_DATA(c >> 8);
      LCD_DATA(c);
      
      //LCD_DATA(bitmap[k]);    
      k = k - 2;
     } 
  }
  digitalWrite(LCD_CS, HIGH);
}
*/
//***************************************************************************************************************************************
// Función para dibujar texto - parámetros ( texto, coordenada x, cordenada y, color, background) 
//***************************************************************************************************************************************
void LCD_Print(String text, int x, int y, int fontSize, int color, int background) {
  int fontXSize ;
  int fontYSize ;
  
  if(fontSize == 1){
    fontXSize = fontXSizeSmal ;
    fontYSize = fontYSizeSmal ;
  }
  if(fontSize == 2){
    fontXSize = fontXSizeBig ;
    fontYSize = fontYSizeBig ;
  }
  
  char charInput ;
  int cLength = text.length();
  Serial.println(cLength,DEC);
  int charDec ;
  int c ;
  int charHex ;
  char char_array[cLength+1];
  text.toCharArray(char_array, cLength+1) ;
  for (int i = 0; i < cLength ; i++) {
    charInput = char_array[i];
    Serial.println(char_array[i]);
    charDec = int(charInput);
    digitalWrite(LCD_CS, LOW);
    SetWindows(x + (i * fontXSize), y, x + (i * fontXSize) + fontXSize - 1, y + fontYSize );
    long charHex1 ;
    for ( int n = 0 ; n < fontYSize ; n++ ) {
      if (fontSize == 1){
        charHex1 = pgm_read_word_near(smallFont + ((charDec - 32) * fontYSize) + n);
      }
      if (fontSize == 2){
        charHex1 = pgm_read_word_near(bigFont + ((charDec - 32) * fontYSize) + n);
      }
      for (int t = 1; t < fontXSize + 1 ; t++) {
        if (( charHex1 & (1 << (fontXSize - t))) > 0 ) {
          c = color ;
        } else {
          c = background ;
        }
        LCD_DATA(c >> 8);
        LCD_DATA(c);
      }
    }
    digitalWrite(LCD_CS, HIGH);
  }
}
//***************************************************************************************************************************************
// Función para dibujar una imagen a partir de un arreglo de colores (Bitmap) Formato (Color 16bit R 5bits G 6bits B 5bits)
//***************************************************************************************************************************************
void LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char bitmap[]){  
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW); 
  
  unsigned int x2, y2;
  x2 = x+width;
  y2 = y+height;
  SetWindows(x, y, x2-1, y2-1);
  unsigned int k = 0;
  unsigned int i, j;

  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      LCD_DATA(bitmap[k]);
      LCD_DATA(bitmap[k+1]);
      //LCD_DATA(bitmap[k]);    
      k = k + 2;
     } 
  }
  digitalWrite(LCD_CS, HIGH);
}
//***************************************************************************************************************************************
// Función para dibujar una imagen sprite - los parámetros columns = número de imagenes en el sprite, index = cual desplegar, flip = darle vuelta
//***************************************************************************************************************************************
void LCD_Sprite(int x, int y, int width, int height, unsigned char bitmap[],int columns, int index, char flip, char offset){
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW); 

  unsigned int x2, y2;
  x2 =   x+width;
  y2=    y+height;
  SetWindows(x, y, x2-1, y2-1);
  int k = 0;
  int ancho = ((width*columns));
  if(flip){
  for (int j = 0; j < height; j++){
      k = (j*(ancho) + index*width -1 - offset)*2;
      k = k+width*2;
     for (int i = 0; i < width; i++){
      LCD_DATA(bitmap[k]);
      LCD_DATA(bitmap[k+1]);
      k = k - 2;
     } 
  }
  }else{
     for (int j = 0; j < height; j++){
      k = (j*(ancho) + index*width + 1 + offset)*2;
     for (int i = 0; i < width; i++){
      LCD_DATA(bitmap[k]);
      LCD_DATA(bitmap[k+1]);
      k = k + 2;
     } 
  }
    
    
    }
  digitalWrite(LCD_CS, HIGH);
}






















//MOTOR DE FÍSICAS
void update_math(void){
  if (gameIsRunning==true){

      //Detectamos si golpeó la pared derecha
      if (ball_x>=300){
        ball_dir_x = -ball_dir_x;
        PLAYER1++;
        ball_x = 320/2;
        ball_y = 240/2;
        ball_dir_x = reset_dir[random(0,9)];
        ball_dir_y = reset_dir[random(0,9)];
      }

      //Detectamos si golpeó la pared izquierda
      if (ball_x<20){
        ball_dir_x = -ball_dir_x;
        PLAYER2++;
        ball_x = 320/2;
        ball_y = 240/2;
        ball_dir_x = reset_dir[random(0,5)];
        ball_dir_y = reset_dir[random(0,5)];
      }

      //Detectamos si golpeó la pared de arriba
      if (ball_y>220 | ball_y<65) {
        ball_dir_y = -1*ball_dir_y;
      }
      ball_x = ball_x + ball_dir_x;
      ball_y = ball_y + ball_dir_y;
    }

    if (PLAYER1==MAX_SCORE){
      gameIsRunning=false;  //paramos el juego
      WINNER=1; //gana jugador1
      GAME_WINNER(WINNER);
      //ESCRIBIR A LA TARGETA SD
      saveGame(WINNER, PLAYER1, PLAYER2);
      //reiniciar juego
      PLAYER1=0;
      PLAYER2=0;
      resetBall = true;
      
    }
    if (PLAYER2==MAX_SCORE){
      gameIsRunning=false;  //paramos el juego
      WINNER=2; //gana jugador2
      GAME_WINNER(WINNER);
      //ESCRIBIR A LA TARGETA SD
      saveGame(WINNER, PLAYER1, PLAYER2);
      //reiniciar juego
      PLAYER1=0;
      PLAYER2=0;
      resetBall = true;
      
    }

    //DETECTAMOS SI GOLPEÓ JUGADOR2
    if (ball_x==X2 && ball_y>=Y2 && ball_y<=(Y2+ P2_HEIGHT)){
      ball_dir_x = -ball_dir_x;
    }
    //DETECTAMOS SI GOLPEÓ JUGADOR1
    if (ball_x==X1 && ball_y>=Y1 && ball_y<=(Y1+ P1_HEIGHT)){
      ball_dir_x = -ball_dir_x;
    }
    
}

//FUNCION QUE CONTROLA LA VELOCIDAD DE LOS JUGADORES
void update_paddles(void){
    //ADAPTAMOS EL VALOR DEL POTENCIÓMETRO A UN RANGO ADECUADO
    POT1 = analogRead(CONTROL1);
    POT1 = POT1>>6;
    POT2 = analogRead(CONTROL2);
    POT2 = POT2>>6; //LES HACEMOS UN CORRIMIENTO DE BITS A LA DERECHA PARA ESTABILIZAR LA MEDICIÓN
    Y1 = map(POT1, 0, 63, 55, 240-P1_HEIGHT-1);//(2*2*2*2*2*2*2*2)
    Y2 = map(POT2, 0, 63, 55, 240-P2_HEIGHT-1);
}









//funcion que controla la animación del ganador
void GAME_WINNER(int winner){
  if (winner==1){
    //ANIMACIÓN GANADOR 1
    LCD_Clear(WHITE);
    LCD_Print(winner_P1, 50, 50, 2, ColorP1, WHITE);
    delay(4000);
    resetBall = false; //se reinicia el juego
  }
  if (winner==2){
    //ANIMACIÓN GANADOR 2
    LCD_Clear(WHITE);
    LCD_Print(winner_P1, 50, 50, 2, ColorP2, WHITE);
    delay(4000);
    resetBall = false; //se reinicia el juego
  }
  if (winner!=1 & winner!=2){
    //Imprimir no hay ganador
    LCD_Clear(BLACK);
    LCD_Print("NO HAY GANADOR", 10, 50, 2, WHITE, BLACK);
    LCD_Print("Game Over", 40, 150, 2, WHITE, BLACK);
    delay(2000);
    resetBall = false; //se reinicia el juego
  }
}





//ANIMACION DEL MENÚ DE JUGADORES
void CharMenu(void){
  //IMPRIME EL MENÚ GENERAL
  LCD_Clear(BLACK);
  //INSTRUCCIONES
  LCD_Print("Presiona el boton", 20, 80, 2, 0xffff, BLACK);
  LCD_Print("para seleccionar", 30, 100, 2, 0xffff, BLACK);
  delay(4000);
  LCD_Print("Presiona el boton", 20, 80, 2, BLACK, BLACK);
  LCD_Print("para seleccionar", 30, 100, 2, BLACK, BLACK);
  //Titulo
  LCD_Print("Dios Maya de", 60, 10, 2, 0xffff, BLACK);
  LCD_Print("La Funacion", 65, 40, 2, 0xffff, BLACK);
  //IMPRIME LA INSTRUCCIÓN
  LCD_Print("Selecciona el personaje", 60, 90, 1, 0xffff, BLACK);

  //DIBUJAMOS EL PERSONAJE
  FillRect(110, 120, 80, 80, BallColor);
  
  //delay(2000); //while falso
  while(1){ //este ciclo nos permite seleccionar el personaje
    POT1 = analogRead(CONTROL1); //va de 0 a 4095
    if (digitalRead(PUSH1)==0){
      delay(10); //Antirrebote
      break; //se rompe el ciclo si se presiona el boton
    }
    if (POT1>=0 & POT1<580){
      BallColor = WHITE;
    }
    if (POT1>=580 & POT1<1160){
      BallColor = YELLOW;
    }
    if (POT1>=1160 & POT1<1740){
      BallColor = MAGENTA;
    }
    if (POT1>=1740 & POT1<2320){
      BallColor = CYAN;
    }
    if (POT1>=2320 & POT1<2900){
      BallColor = GREEN;
    }
    if (POT1>=2900 & POT1<3480){
      BallColor = RED;
    }
    if (POT1>=3480 & POT1<4060){
      BallColor = BLUE;
    }

    //DIBUJAMOS LA PELOTA
    FillRect(110, 120, 80, 80, BallColor);
  }
  
  //PONER ACÁ UN WHILE QUE NO DEJE PASAR HASTA QUE SE PRESIONE PUSH1
  LCD_Print("Selecciona el personaje", 60, 90, 1, BLACK, BLACK); //BORAMOS EL TEXTO
  LCD_Print("Color jugador 1", 90, 90, 1, 0xffff, BLACK);
  FillRect(110, 120, 80, 80, ColorP1);
  //delay(2000);

  while(1){ //este ciclo nos permite seleccionar el COLOR DE JUGADOR
    POT1 = analogRead(CONTROL1); //va de 0 a 4095
    if (digitalRead(PUSH1)==0){
      delay(10); //Antirrebote
      break; //se rompe el ciclo si se presiona el boton
    }
    if (POT1>=0 & POT1<580){
      ColorP1 = 0xF364;
    }
    if (POT1>=580 & POT1<1160){
      ColorP1 = YELLOW;
    }
    if (POT1>=1160 & POT1<1740){
      ColorP1 = MAGENTA;
    }
    if (POT1>=1740 & POT1<2320){
      ColorP1 = CYAN;
    }
    if (POT1>=2320 & POT1<2900){
      ColorP1 = GREEN;
    }
    if (POT1>=2900 & POT1<3480){
      ColorP1 = RED;
    }
    if (POT1>=3480 & POT1<4060){
      ColorP1 = BLUE;
    }

    //DIBUJAMOS EL PERSONAJE
    FillRect(110, 120, 80, 80, ColorP1);
  }


  //PONER ACÁ OTRO WHILE QUE NO DEJE PASAR HASTA QUE SE PRESIONE PUSH1
  LCD_Print("Selecciona el personaje", 60, 90, 1, BLACK, BLACK); //BORAMOS EL TEXTO
  LCD_Print("Color jugador 2", 90, 90, 1, 0xffff, BLACK);
  FillRect(110, 120, 80, 80, ColorP2);
  //delay(2000);

  while(1){ //este ciclo nos permite seleccionar el COLOR DE JUGADOR
    POT1 = analogRead(CONTROL1); //va de 0 a 4095
    if (digitalRead(PUSH1)==0){
      delay(10); //Antirrebote
      break; //se rompe el ciclo si se presiona el boton
    }
    if (POT1>=0 & POT1<580){
      ColorP2 = 0xF364;
    }
    if (POT1>=580 & POT1<1160){
      ColorP2 = YELLOW;
    }
    if (POT1>=1160 & POT1<1740){
      ColorP2 = MAGENTA;
    }
    if (POT1>=1740 & POT1<2320){
      ColorP2 = CYAN;
    }
    if (POT1>=2320 & POT1<2900){
      ColorP2 = GREEN;
    }
    if (POT1>=2900 & POT1<3480){
      ColorP2 = RED;
    }
    if (POT1>=3480 & POT1<4060){
      ColorP2 = BLUE;
    }

    //DIBUJAMOS EL PERSONAJE
    FillRect(110, 120, 80, 80, ColorP2);
  }
  LCD_Clear(0x00); //BORRAMOS LA PANTALLA AL FINALIZAR
}

//GUARDAR LA PARTIDA EN LA MEMORIA
void saveGame(int winner, int P1, int P2){
  //ANIMACION PARA PREGUNTAR SI GUARDAMOS EL JUEGO
  LCD_Clear(0x00); //BORRAMOS LA PANTALLA
  LCD_Print("Guardar partida", 40, 80, 2, 0xffff, BLACK);
  LCD_Print("Si", 100, 100, 2, 0xffff, BLACK);
  LCD_Print("No", 140, 100, 2, 0xffff, BLACK);
  while (1){
    POT1 = analogRead(CONTROL1); //va de 0 a 4095, selecciona opciones
    if (POT1>=0 & POT1<=2048){
      LCD_Print("Si", 100, 100, 2, GREEN, BLACK);
      LCD_Print("No", 140, 100, 2, 0xffff, BLACK);
      saveGametoMemory = true;
    }
    if (POT1>=2048 & POT1<=4095){
      LCD_Print("Si", 100, 100, 2, 0xffff, BLACK);
      LCD_Print("No", 140, 100, 2, GREEN, BLACK);
      saveGametoMemory = false;
    }
    if (digitalRead(PUSH1)==0){
      LCD_Clear(0x00); //BORRAMOS LA PANTALLA
      break;
    }
  }

  if (saveGametoMemory==true){
    //GUARDAR EL JUEGO EN LA MEMORIA
    //si gana jugador 1
    if (winner==1){
      file = SD.open("Puntajes.txt", FILE_WRITE); //abrimos Puntajes.txt para escribirles
      if (file) { //escribimos los datos de la partida
        file.println("Gano Jugador 1");
        file.print("Puntaje J1: ");      
        file.println(String(P1));
        file.print("Puntaje J2: ");
        file.println(String(P2));
      } else {
        Serial.println("No se pudo abrir el archivo"); //si no se puede abrir el archivo genera este error
      }
      file.close(); //cerramos el archivo
    }
    //si gana jugador 2
    if (winner==2){
      file = SD.open("Puntajes.txt", FILE_WRITE); //abrimos Puntajes.txt para escribirles
      if (file) { //escribimos los datos de la partida
        file.println("Gano Jugador 2");
        file.print("Puntaje J1: ");      
        file.println(String(P1));
        file.print("Puntaje J2: ");
        file.println(String(P2));
      } else {
        Serial.println("No se pudo abrir el archivo"); //si no se puede abrir el archivo genera este error
      }
      file.close(); //cerramos el archivo
    }
  }
  
  
  
}
