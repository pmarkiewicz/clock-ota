#include <Arduino.h>
#include <U8g2lib.h>

U8G2_MAX7219_32X8_1_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ D6, /* data=*/ D8, /* cs=*/ D7, /* dc=*/ U8X8_PIN_NONE, /* reset=*/ U8X8_PIN_NONE);



/*
 *  x
 *  0         1         2         3
 *  01234567|89012345|67890123|45678901|
 *          |
 *          |
 *  22222233|11112222|00111111|00000000|
 *  45678901|67890123|89012345|01234567|
 *                   
 *                   
 */

const uint8_t brightness[] = {5, 5, 5, 5, 5, 40, 60, 70, 80, 90, 200, 200, 200, 200, 200, 200, 180, 150, 80, 80, 70, 60, 50, 30, 0};

int map_x(int x) {
  int x1 = 3 - x / 8;  
  int x2 = x1 * 8;
  int x3 = x % 8;

  return x2 + x3;
}

void display_init()
{
  u8g2.begin();
  u8g2.setFont(u8g2_font_amstrad_cpc_extended_8r);  // choose a suitable font
}

void spin()
{
  static int led_no = 0;

  u8g2.clearBuffer();

  u8g2.drawPixel(map_x(led_no), 3);
  u8g2.drawPixel(map_x(led_no), 4);

  u8g2.sendBuffer();

  led_no++;
  led_no %= 32;
}

void render_time(int h, int m, int s)
{
  char tm[] = "    ";
  char c;

  u8g2.clearBuffer();
  
  if (h / 10) {
    tm[0] = '0' + h / 10;
  }

  tm[1] = '0' + h % 10;
  tm[2] = '0' + m / 10;
  tm[3] = '0' + m % 10;

  c = tm[0];
  tm[0] = tm[3];
  tm[3] = c;

  c = tm[1];
  tm[1] = tm[2];
  tm[2] = c;
    
  u8g2.drawStr(0, 8, tm);

  if (s & 1) {
    u8g2.drawPixel(map_x(15), 7);
    u8g2.drawPixel(map_x(16), 7);    
  }
}

void display_time(int h)
{
  uint8_t br = brightness[h];
  u8g2.setContrast(br);
  u8g2.sendBuffer();
}

void display_clean()
{
  u8g2.clearBuffer();
  u8g2.sendBuffer();
}

void display_msg(const char* msg) {
  char str[] = "    ";
  char c;

  strlcpy(str, msg, sizeof(str));
  c = str[0];
  str[0] = str[3];
  str[3] = c;

  c = str[1];
  str[1] = str[2];
  str[2] = c;
  
  u8g2.drawStr(0, 8, str);
  u8g2.sendBuffer();  
}

void fill_display() {

  for (int i = 0; i < 32; i++) {
    for (int j=0; j < 8; j++) {
      u8g2.drawPixel(i, j);
    }
  }

  u8g2.sendBuffer();  
}

