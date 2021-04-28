//***************************************************************************************************************************************
/* Universidad del Valle de Guatemala
 * Electrónica Digital II
 * Profesor: Kurt Kellner
 * Sección: 30
 * Autores:
 * Juan Pablo Valenzuela carnet: 18057
 * Katherine Caceros     carnet: 18307
 * Descripción:
 * Este es el código que controla el juego Dios Maya de la Funación, el cual es una parodia del original PONG (1972)
 * Este juego introduce sonidos personalizados al juego original, canciones populares de otros juegos clásicos, 
 * Un menú para seleccionar diferentes personajes (colores) para la pelota, jugador 1 y jugador 2. Además
 * Muestra animaciones para el ganador junto con una canción elegida al azar. Al inicio de cada partida también
 * reproduce una canción elegida al azar. Permite guardar las partidas en una memoria SD la cual cuenta con bastante
 * espacio
  */
//***************************************************************************************************************************************
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

//LIBRERIA DEL SONIDO ZELDA
#include "pitches_zelda.h"

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

//DEFINICION DE COLORES
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x3E7C      //0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0x60DF      //0xFFE0  
#define WHITE           0xFFFF

//JUGADORES
#define UP1         PF_4 //PUSH1 de la tiva
#define CONTROL1    A3 //POTENCIOMETRO JUGADOR 1
#define CONTROL2    A0 //POTENCIOMETRO JUGADOR 2
#define DOWN2       PC_7 //NO HACE NADA
//PUSH2 = PF_0





//SONIDO
//---------------------------------------------------------------------------------
//SALIDA DE AUDIO EN EL PIN PD_7
#define BUZZER      PD_7  

//NOTAS MUSICALES
#define C4 262
#define C4s 277
#define D4 294
#define D4s 311
#define E4 330
#define F4 349
#define F4s 370
#define G4 392
#define G4s 415
#define A4 440
#define A4s 466
#define B4 494
#define C5 523
#define C5s 554
#define D5 587
#define D5s 622
#define E5 659
#define F5 698
#define F5s 740
#define G5 784
#define G5s 831
#define A5 880
#define A5s 932
#define B5 988
#define C6 1109

//VARIABLES DE SONIDO
const float songSpeed = 1.0;

//CANCIONES

int notes[] = {
  E4, G4, B4, D5, E5, D5, F5s, F5s, E5,
  E4, G4, B4, D5, E5, D5, F5s, G5, F5s,
  E4, G4, B4, D5, E5, D5, F5s, F5s, E5,
  E4, G4, B4, D5, E5, D5, F5s, G5, F5s    
};

int notes2[] = {
  C5, 0, B4, 0, B4, 0, C5, 0, B4, 0, A4,
  0, A4, 0, D5, 0, D5, 0, E5, 0, 
  E5, 0, C5, 0, B4, 0, B4, 0, C5, 0, D5, 0, E5,
  0, E5, 0, G5, 0, G5, 0, A5 ,0   
};


//DURACION CANCION 1
int durations[] = {
  100, 100, 100, 100, 100, 100, 300, 300, 400,
  100, 100, 100, 100, 100, 100, 300, 300, 400,
  100, 100, 100, 100, 100, 100, 300, 300, 400,
  100, 100, 100, 100, 100, 100, 300, 300, 400
 };

//DURACION CANCION 2
int durations2[] = {
  200, 200, 200, 200, 100, 100, 100, 100, 100, 100, 200,
  200, 300, 100, 200, 200, 200, 200, 100, 100,
  100, 100, 100, 100, 200, 200, 100, 100, 100, 100, 100, 100, 300,
  100, 300, 100, 300, 100, 300, 100, 100, 0
 };

  
//---------------------------------------------------------------------------------





//GLOBAL VARIABLES
int DPINS[] = {PB_0, PB_1, PB_2, PB_3, PB_4, PB_5, PB_6, PB_7};  
int start = 0;
int reset_dir[] = {-10, 10, -10, -10, 10, 10, -10, 10, 10, -10};

//VELOCIDADES
const unsigned long PADDLE_RATE = 33;
const unsigned long BALL_RATE = 10;
//GRAFICOS
const uint8_t PADDLE_HEIGHT = 24;
int ball_width = 5;
int ball_height = 5;
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


//controles del juego
boolean gameIsRunning = true;
boolean resetBall = false;
boolean gameOverMessage = true;
boolean saveGametoMemory = false;

//PUNTAJE
int PLAYER1 = 0;
int PLAYER2 = 0;
int MAX_SCORE = 4; //PUNTAJE MÁXIMO
int WINNER = 0;

//JUGADOR1
int P1_WIDTH = 5;
int P1_HEIGHT = 60; //40 es el valor ideal
int X1 = 20;
int Y1 = 240/2;
int Y1_old = Y1;


//JUGADOR2
int P2_WIDTH = 5;
int P2_HEIGHT = 60; //40 es el valor ideal
int X2 = 280;
int Y2 = 240/2;
int Y2_old = Y2;

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

//ANIMACION DE GANADOR
void GAME_WINNER(int winner);

//Selección de personaje
void CharMenu(void);
//ANIMACIÓN DE ENTRADA
void intro_funacion(void);

//GUARDAR EN LA MEMORIA SD
void saveGame(int winner);

//-----------------------------------------------------------------------------------
/*  ESTAS SON LAS FUNCIONES QUE MANEJAN LOS SONIDOS DEL JUEGO
 * 
 */

//REPRODUCIR CANCION
void playsong(void); 
void playsongtest(long song);
//SONIDO DE PELOTA
void crashsound(void);

//canciones almacenadas
void playSongOfTime(void);
void playSongOfStorms(void);
void playSariaSong(void);
void playZeldaLullaby(void);
void playSunSong(void);
void playEponaSong(void);

//-----------------------------------------------------------------------------------

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
  pinMode(BUZZER, OUTPUT); //se selecciona el pin del buzzer como salida
  //VERIFICAR SI LEE LA TARGETA 
  while(1){
    if (SD.begin(CSS)==0){
    Serial.println("No se pudo iniciar la targeta SD");
    } else {
    Serial.println("Se inició la targeta SD");  
    //SONIDOS DE APROVACIÓN DE LA TARGETA
    tone(BUZZER, C4, 100);
    delay(100);
    tone(BUZZER, D4, 100);
    delay(50);
    tone(BUZZER, E5, 100);
    delay(120);
    tone(BUZZER, C5, 100);
    delay(60);
    tone(BUZZER, F5, 100);
    delay(90);
    noTone(BUZZER);  
    
    break;
    }
  }
  

  //INICIALIZACION DE LA PANTALLA
  
  GPIOPadConfigSet(GPIO_PORTB_BASE, 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU);
  //Serial.println("Inicio"); //no se usa en este programa
  LCD_Init();
  LCD_Clear(0x00);

  //CONFIGURACION IO
  pinMode(PUSH1, INPUT_PULLUP); ///BOTON DE SELECCIONAR OPCIONES
  pinMode(CONTROL1, INPUT); //POT1
  pinMode(CONTROL2, INPUT); //POT2

  //adjust random seed
    pinMode(A1, INPUT); //pin QUE GENERA UN NÚMERO ALEATORIO DIFERENTE CADA VEZ
    randomSeed(analogRead(A1));
  
    //playsongtest(random(1,10)); //numero al azar del 1 al 9 solo era una prueba

  //INICIO DEL JUEGO
  //pantalla de inicio
  LCD_Bitmap(0, 0, 320, 240, fondo);
  String game_title = "DIOS MAYA";
  LCD_Print(game_title, 90, 20, 2, RED, WHITE);
  LCD_Print("de la Funacion", 105, 35, 1, RED, WHITE);
  delay(4000);
  LCD_Clear(0x00);
  
  //menu de inicio
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

  
  //CANCION DE INICIO DEL JUEGO
  playsongtest(random(0,10)); //numero al azar del 0 al 9



}












//***************************************************************************************************************************************
// Loop Infinito
//***************************************************************************************************************************************
void loop() {
    if (gameIsRunning==true){
        //320 x 240 dimensiones máximas pantalla
        //ÚNICAMENTE SI HAY UN CAMBIO DIBUJAMOS A LOS JUGADORES EN SU NUEVA POSICIÓN
      if (Y1_old!=Y1){
        FillRect(X1, Y1_old, P1_WIDTH, P1_HEIGHT, BLACK);
        
      }
      if (Y2_old!=Y2){
        FillRect(X2, Y2_old, P2_WIDTH, P2_HEIGHT, BLACK);
      }
      //Dibujamos la pelota
      FillRect(ball_x, ball_y, ball_width, ball_height, BallColor);
      
      //DIBUJAMOS JUGADOR2
      FillRect(X2, Y2, P2_WIDTH, P2_HEIGHT, ColorP2);
      //DIBUJAMOS JUGADOR1
      FillRect(X1, Y1, P1_WIDTH, P1_HEIGHT, ColorP1);
      Y1_old = Y1;
      Y2_old = Y2;
      //delay(BALL_RATE);
      FillRect(ball_x, ball_y, ball_width, ball_height, BLACK); //BORRAMOS LA PELOTA
      //DIBUJAMOS EL PUNTEO
      LCD_Print(String(PLAYER1), X1+50, 10, 2, ColorP1, WHITE);
      LCD_Print(String(PLAYER2), X2-50, 10, 2, ColorP2, WHITE);
  
  
      update_math();
    } else {
      if (resetBall==true){
        //SE REINICIA EL JUEGO Y SE MUESTRA LA ANIMACIÓN DE VICTORIA
        GAME_WINNER(WINNER);
        resetBall=false;
      }      
      if (gameOverMessage==true){
        //MENÚ PARA QUE EL JUGADOR REINICIE EL JUEGO
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
      //pantalla de inicio
      LCD_Bitmap(0, 0, 320, 240, fondo);
      String game_title = "DIOS MAYA";
      LCD_Print(game_title, 90, 20, 2, RED, WHITE);
      LCD_Print("de la Funacion", 105, 35, 1, RED, WHITE);
      delay(4000);
      LCD_Clear(0x00);
      //MENU DE INICIO
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
      //CANCION DE INICIO
      playsongtest(random(0,10)); //numero al azar del 0 al 9
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
        //SONIDO DE GOL 1
        tone(BUZZER, _B3, 90);
        delay(90);
        noTone(BUZZER);
        tone(BUZZER, _B2, 120);
        delay(120);
        noTone(BUZZER);
        tone(BUZZER, _A1, 105);
        delay(105);
        noTone(BUZZER);
      }

      //Detectamos si golpeó la pared izquierda
      if (ball_x<20){
        ball_dir_x = -ball_dir_x;
        PLAYER2++;
        ball_x = 320/2;
        ball_y = 240/2;
        ball_dir_x = reset_dir[random(0,5)];
        ball_dir_y = reset_dir[random(0,5)];
        //SONIDO DE GOL 2
        tone(BUZZER, _B3, 90);
        delay(90);
        noTone(BUZZER);
        tone(BUZZER, _E2, 120);
        delay(120);
        noTone(BUZZER);
        tone(BUZZER, _GS1, 105);
        delay(105);
        noTone(BUZZER);
      }

      //Detectamos si golpeó la pared de arriba
      if (ball_y>220 | ball_y<65) {
        ball_dir_y = -1*ball_dir_y;
        //sonido de golpe
        tone(BUZZER, E4, 90);
        delay(90);
        noTone(BUZZER);
      }
      ball_x = ball_x + ball_dir_x;
      ball_y = ball_y + ball_dir_y;      
    }

    if (PLAYER1==MAX_SCORE){
      gameIsRunning=false;  //paramos el juego
      WINNER=1; //gana jugador1
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
      //ESCRIBIR A LA TARGETA SD (GUARDAR PARTIDA)
      saveGame(WINNER, PLAYER1, PLAYER2);
      //reiniciar juego
      PLAYER1=0;
      PLAYER2=0;
      resetBall = true;
    }

    //DETECTAMOS SI GOLPEÓ JUGADOR2
    if (ball_x==X2 && ball_y>=Y2 && ball_y<=(Y2+ P2_HEIGHT)){
      ball_dir_x = -ball_dir_x;
      //SONIDO DE GOLPE
      tone(BUZZER, _E3, 90);
      delay(90);
      noTone(BUZZER);
    }
    //DETECTAMOS SI GOLPEÓ JUGADOR1
    if (ball_x==X1 && ball_y>=Y1 && ball_y<=(Y1+ P1_HEIGHT)){
      ball_dir_x = -ball_dir_x;
      //SONIDO DE GOLPE
      tone(BUZZER, _E3, 90);
      delay(90);
      noTone(BUZZER);
      
    }

    //ADAPTAMOS EL VALOR DEL POTENCIÓMETRO A UN RANGO ADECUADO
    POT1 = analogRead(CONTROL1);
    POT1 = POT1>>7; //LE QUITAMOS BITS PARA QUE SEA MÁS ESTABLE
    POT2 = analogRead(CONTROL2);
    POT2 = POT2>>7; //LES HACEMOS UN CORRIMIENTO DE BITS A LA DERECHA PARA ESTABILIZAR LA MEDICIÓN
    Y1 = map(POT1, 0, 4095/(2*2*2*2*2*2*2), 55, 240-P1_HEIGHT);
    Y2 = map(POT2, 0, 4095/(2*2*2*2*2*2*2), 55, 240-P2_HEIGHT);
}

void GAME_WINNER(int winner){
  if (winner==1){
    //ANIMACIÓN GANADOR 1
    LCD_Clear(WHITE);
    LCD_Print(winner_P1, 50, 50, 2, ColorP1, WHITE);
    delay(1000); //por si acaso es muy corta la canción
    playsongtest(random(0,10)); //delay
    resetBall = false; //se reinicia el juego
  }
  if (winner==2){
    //ANIMACIÓN GANADOR 2
    LCD_Clear(WHITE);
    LCD_Print(winner_P2, 50, 50, 2, ColorP2, WHITE);
    delay(1000);
    //CANCION AL AZAR PARA EL GANADOR
    playsongtest(random(0,10)); //delay
    resetBall = false; //se reinicia el juego
  }
  if (winner!=1 & winner!=2){
    //Imprimir no hay ganador
    LCD_Clear(BLACK);
    LCD_Print("NO HAY GANADOR", 10, 50, 2, WHITE, BLACK);
    LCD_Print("Game Over", 40, 150, 2, WHITE, BLACK);
    delay(2000);
    //CANCION ALEATORIA PARA EL GANADOR
    playsongtest(random(0,10)); //delay
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






//EFECTOS DE SONIDO CON LA TIVAC
//ESTO ES UNA PRUEBA DE SONIDO EN LA TIVA
void playsong(void){
  const int totalNotes = sizeof(notes2) / sizeof(int);
  for (int i = 0; i < totalNotes; i++)
  {
    int currentNote = notes2[i];
    int wait = durations2[i] / songSpeed;
    if (currentNote != 0)
    {
      tone(BUZZER, notes2[i], wait); 
    }
    else
    {
      noTone(BUZZER);
    }
    delay(wait);
  }
  
}













//seleccionador de cancion a reproducir
/*
 * 1. Canción de inicio
 * 2. Cancion 1 ganador
 * 3. Cancion 2 ganador
 * 4. Cancion 3 ganador
 * 5. Cancion 4 ganador
 * 6. Cancion 6 ganador
 * 7. Cancion 7
 * 8. Cancion 8
 */
void playsongtest(long song){
  if (song==0){ //EASTER EGG SONG
    // Play 1-up sound (MARIO)
    tone(BUZZER,_E6,125);
    delay(130);
    tone(BUZZER,_G6,125);
    delay(130);
    tone(BUZZER,_E7,125);
    delay(130);
    tone(BUZZER,_C7,125);
    delay(130);
    tone(BUZZER,_D7,125);
    delay(130);
    tone(BUZZER,_G7,125);
    delay(125);
    noTone(BUZZER);
  
    delay(2000);  // pause 2 seconds
  }
  //CANCIÓN 1 COMPUESTA POR KATHY
  if (song==1){
    const int totalNotes = sizeof(notes2) / sizeof(int);
    for (int i = 0; i < totalNotes; i++)
    {
      int currentNote = notes2[i];
      int wait = durations2[i] / songSpeed;
      if (currentNote != 0)
      {
        tone(BUZZER, notes2[i], wait); 
      }
      else
      {
        noTone(BUZZER);
      }
      delay(wait);
    }
  }
  //CANCIÓN 2 COMPUESTA POR KATHY
  if (song==2){
    const int totalNotes = sizeof(notes) / sizeof(int);
    for (int i = 0; i < totalNotes; i++)
    {
      int currentNote = notes[i];
      int wait = durations[i] / songSpeed;
      if (currentNote != 0)
      {
        tone(BUZZER, notes[i], wait); 
      }
      else
      {
        noTone(BUZZER);
      }
      delay(wait);
    }
  }

  if (song==3){ //CANCION DE SARIA (ZELDA)
    playSariaSong();
  }

  if (song==4){ //CANCION DE ZELDA'S LULLABY (OCARINA OF TIME)
    playZeldaLullaby();
  }

  if (song==5){ //CANCIÓN DEL SOL
    playSunSong();
  }

  if (song==6){ //CANCION DE LAS TORMENTAS
    playSongOfStorms();
  }

  if (song==7){ //CANCIÓN DEL TEMPLO DEL TIEMPO EN ZELDA OOT
    playSongOfTime();
  }

  if (song==8){ //CANCIÓN PARA LLAMAR AL CABALLO DE LINK
    playEponaSong();
  }

  if (song==9){ //CANCIÓN REPETIDA DE MARIO PARA MÁS PROBABILIDAD
    // Play 1-up sound
    tone(BUZZER,_E6,125);
    delay(130);
    tone(BUZZER,_G6,125);
    delay(130);
    tone(BUZZER,_E7,125);
    delay(130);
    tone(BUZZER,_C7,125);
    delay(130);
    tone(BUZZER,_D7,125);
    delay(130);
    tone(BUZZER,_G7,125);
    delay(125);
    noTone(BUZZER);
  
    delay(2000);  // pause 2 seconds
  }


}



//-------------------------------------------------------------------------------------------------------------------------------
//*******************************************************************************************************************************
//Canciones zelda OCARINA OF TIME

void playSongOfTime(void){

      //Reproducimos el sonido
      unsigned short songOfTimeArray[] = {_A4, _D4, _F4, _A4, _D4, _F4, _A4, _C5, _B4, _G4, _F4, _G4, _A4, _D4, _C4, _E4};
      unsigned short songOfTimeDelay[] = {500, 1000, 500, 500, 1000, 500, 250, 250, 500, 500, 250, 250, 500, 500, 250, 250};
      for(unsigned short i = 0;i <= (sizeof(songOfTimeArray)/sizeof(songOfTimeArray[0]));i++){
          tone(BUZZER, songOfTimeArray[i]);
          delay(songOfTimeDelay[i]);
          //sistema de seguridad por si acaso 
          if (songOfTimeArray[i]==_E4){
            break;
          }
      }
      //PARAMOS EL SONIDO
      tone(BUZZER, _D4, 500);
      delay(500);
      noTone(BUZZER);
      delay(500);
  }


void playEponaSong(void){ //CANCION PARA LLAMAR A EPONA

      //REPRODUCIR CANCION
      unsigned short EponaSongArray[] = {_D5, _B4, _A4, _D5, _B4, _A4, _D5, _B4, _A4, _B4};
      unsigned short EponaSongDelay[] = {250, 250, 1125, 250, 250, 1125, 250, 250, 625, 625};
      for(int i = 0;i < (sizeof(EponaSongArray)/sizeof(EponaSongArray[0]));i++){
          tone(BUZZER, EponaSongArray[i]);
          delay(EponaSongDelay[i]);
          //sistema de seguridad por si acaso 
          if (i==9){
            break;
          }
      }
      tone(BUZZER, _A4, 750);
      delay(750);
      noTone(BUZZER);
      delay(500);
  }



void playSongOfStorms(void){ //CANCION PARA QUE LLUEVA

      //reproducir
      unsigned short songOfStromArray[] = {_D4, _F4, _D5, _D4, _F4, _D5, _E5, _F5, _E5, _F5, _E5, _C5, _A4, _A4, _D4, _F4, _G4, _A4, _A4, _D4, _F4, _G4};
      unsigned short songOfStormDelay[] = {250, 125, 600, 250, 125, 600, 500, 125, 125, 125, 125, 125, 625, 250, 250, 125, 125, 625, 250, 375, 125, 250};
      for(int i = 0;i < (sizeof(songOfStromArray)/sizeof(songOfStromArray[0]));i++){
          tone(BUZZER, songOfStromArray[i]);
          delay(songOfStormDelay[i]);
          if(i == 12 || i == 17){
            noTone(BUZZER);
          }
          if (i==sizeof(songOfStromArray)){
            break;
          }
      }
      tone(BUZZER, _E4,750);
      delay(750);
      noTone(BUZZER);
      delay(500);
  }





void playSunSong(void){ //CANCION PARA CONVERTIR LA NOCHE EN DIA

      //reproducir
      unsigned short sunSongArray[] = {_A4, _F4, _D5, _A4, _F4, _D5, _A4, _B4, _C5, _D5, _E5, _F5};
      unsigned short sunSongDelay[] = {250, 250, 625, 250, 125, 500, 125, 125, 125, 125, 125, 125};
      for(int i = 0;i < (sizeof(sunSongArray)/sizeof(sunSongArray[0]));i++){
          tone(BUZZER, sunSongArray[i]);
          delay(sunSongDelay[i]);

          if (i==sizeof(sunSongArray)){
            break;
          }
      }
      //PARAR LA CANCION
      tone(BUZZER, _G5, 750);
      delay(750);
      noTone(BUZZER);
      delay(500);
  }




void playZeldaLullaby(void){ //CANCION QUE ABRE LA DIMENSION SAGRADA DE LA ESPADA MAESTRA Y PUEDE HACER QUE LINK VIAJE EN EL TIEMPO
      //REPRODUCCION
      unsigned short zeldaLullabyArray[] = {_B4, _D5, _A4, _B4, _D5, _A4, _B4, _D5, _A5, _G5, _D5, _C5, _B4};
      unsigned short zeldaLullabyDelay[] = {875, 500, 1000, 875, 500, 1000, 875, 500, 1000, 500, 750, 250, 250};
      for(int i = 0;i < (sizeof(zeldaLullabyArray)/sizeof(zeldaLullabyArray[0]));i++){
          tone(BUZZER, zeldaLullabyArray[i]);
          delay(zeldaLullabyDelay[i]);
          if (i==sizeof(zeldaLullabyArray)){
            break;
          }
      }
      //PARAR LA CANCION
      tone(BUZZER, _E4, 750);
      delay(750);
  }




  void playSariaSong(void){ //CANCION DE LA AMIGA DE LINK

      //reproducir
      unsigned short sariaSongArray[] = {_F4, _A4, _B4, _F4, _A4, _B4, _F4, _A4, _B4, _E5, _D5, _B4, _C5, _B4, _G4, _E4, _D4, _E4, _G4};
      unsigned short sariaSongDelay[] = {250, 250, 500, 250, 250, 500, 250, 250, 250, 250, 500, 250, 250, 250, 250, 625, 250, 250, 250};
      for(int i = 0;i < (sizeof(sariaSongArray)/sizeof(sariaSongArray[0]));i++){
          tone(BUZZER, sariaSongArray[i]);
          delay(sariaSongDelay[i]);
          if (i==sizeof(sariaSongArray)){
            break;
          }
      }
      //PARAR LA CANCION
      tone(BUZZER, _E4, 750);
      delay(750);
  }










//-------------------------------------------------------------------------------------------------------------------------------
//*******************************************************************************************************************************
