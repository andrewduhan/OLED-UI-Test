#define OLED_DC 11
#define OLED_CS 12
#define OLED_CLK 10
#define OLED_MOSI 9
#define OLED_RESET 13
#define BUTTON_DOWN 7
#define BUTTON_UP 8
#define BUTTON_SEL 6
#define BUTTON_MODE 5
#define START_CHAR 32 // " "

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "dpfont_3x5.c"
#include "dpfont_6x10.c"

Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
//

static uint8_t screen_resolution[] = {128,32};
static uint8_t control_box_positions[] = {1, 22, 43, 64, 107};
static uint8_t control_box_height = 32;
static uint8_t control_box_width = 20;

// pre-assign these values for testing
float measures[5] = {5.27, 3.12, 16.4, 1.68, 3.09};
char* measure_str[] = {"5.27", "3.12", "16.4", "1.68", "3.09"};

unsigned long keypress_timer;
unsigned long selection_timer;

bool editMode = false;

int8_t current_selection = -1;
uint16_t battery_level = 512;

void setup(){
  Serial.begin(115200);
  Serial.println("boom hot dog.");

  pinMode(BUTTON_DOWN, INPUT);
  digitalWrite(BUTTON_DOWN, HIGH);
  pinMode(BUTTON_UP, INPUT);
  digitalWrite(BUTTON_UP, HIGH);
  pinMode(BUTTON_SEL, INPUT);
  digitalWrite(BUTTON_SEL, HIGH);
  pinMode(BUTTON_MODE, INPUT);
  digitalWrite(BUTTON_MODE, HIGH);

  keypress_timer = millis();

  // generate the high voltage from the 3.3v line
  display.begin(SSD1306_SWITCHCAPVCC);
  display.clearDisplay();

  // init done

//bg lines
  yLine(0,   B10101010);
  yLine(21,  B10101010);
  yLine(42,  B10101010);
  yLine(63,  B10101010);
  yLine(84,  B10101010);
  yLine(127, B10101010);

  for (uint8_t i = 0; i < 5; i++){
    paintBox(i, false);
  }

//battery
  display.drawRect(87,1, 12,29, WHITE);
  display.drawRect(90,29, 6,3, WHITE);
  display.drawLine(91,29, 94, 29, BLACK);
  setBattery(1023);

  sStringVert(100, 29, false, "LIFEPO4");

  display.display();
}

// void sString(int8_t x, int8_t y, char stuff[]){
//   for ( uint8_t s = 0; s < strlen(stuff); s++ ){
//     int letter_width = smallLetterMap[stuff[s] - START_CHAR][1];
//     uint8_t letter = smallLetterMap[stuff[s] - START_CHAR][0];
//     for (int8_t i=0; i<letter_width; i++) {
//       uint8_t line;
//       line = smallLetters[letter + i];
//       for (uint8_t j=0; j<5; j++) {
//         if (line & 0x1){
//           display.drawPixel(x+i, y+j, WHITE);
//         } else {
//           display.drawPixel(x+i, y+j, BLACK);
//         }
//         line >>= 1;
//       }
//     }
//     x = x + letter_width + 1;
//   }
// }


//TODO: combine sStringVert and lStringVert
void sStringVert(int8_t x, int8_t y, bool inverted, char stuff[]){
  uint16_t bg = BLACK;
  uint16_t fg = WHITE;
  if (inverted){
    bg = WHITE;
    fg = BLACK;
  }
  for ( uint8_t s = 0; s < strlen(stuff); s++ ){
    int letter_width = smallLetterMap[stuff[s] - START_CHAR][1];
    uint8_t letter = smallLetterMap[stuff[s] - START_CHAR][0];
    for (int8_t i=0; i<letter_width; i++) {
      uint8_t line;
      line = smallLetters[letter + i];
      for (uint8_t j=0; j<5; j++) {
        if (line & 0x1)
          display.drawPixel(x+j, y-i, fg);
        else
          display.drawPixel(x+j, y-i, bg);
        line >>= 1;
      }
    }
    y = y - letter_width - 1;
  }
}

void lStringVert(int8_t x, int8_t y, bool inverted, char stuff[]){
  uint16_t bg = BLACK;
  uint16_t fg = WHITE;
  if (inverted){
    bg = WHITE;
    fg = BLACK;
  }
  for ( uint8_t s = 0; s < strlen(stuff); s++ ){
    int letter_width = largeLetterMap[stuff[s] - START_CHAR][1];
    uint8_t letter = largeLetterMap[stuff[s] - START_CHAR][0];
    for (int8_t i=0; i<letter_width; i++) {
      uint8_t line, lineIndex;
      lineIndex = (letter*2) + (i*2);
      line = largeLetters[lineIndex];
      for (uint8_t j=0; j<8; j++) {
        if (line & 0x1)
          display.drawPixel(x+j, y-i, fg);
        else
          display.drawPixel(x+j, y-i, bg);
        line >>= 1;
      }
      lineIndex++;
      line = largeLetters[lineIndex];
      for (uint8_t j=0; j<2; j++) {
        if (line & 0x1)
          display.drawPixel( (x-2)+j, y - i, fg);
        else
          display.drawPixel( (x-2)+j, y - i, bg);
        line >>= 1;
      }
    }
    y = y - letter_width - 1;
  }
}

void yLine(int8_t x, byte dashes) {
  Serial.println("in yLine");
  if ( x > screen_resolution[0] - 1 ){
    Serial.println("xmax hit");
    return;
  }
  for (uint8_t i = 0; i<screen_resolution[1]/8; i++){
    Serial.print(x);
    Serial.print(", ");
    Serial.println(i);
    byte segment = dashes;
    for (int8_t y=0; y<8; y++){
      if (segment & 0x1)
        display.drawPixel(x, (i*8)+y, WHITE);
      else
        display.drawPixel(x, (i*8)+y, BLACK);
      segment >>= 1;
    }
  }
}

void selectionChange( int8_t direction ){
  if (current_selection < 0){
    current_selection = 0;
  } else {
    paintBox(current_selection, false);
      //add five because arduino doesn't handle negative modulo
    current_selection = ((current_selection + 5 + direction) % 5) ;
    if (current_selection == 3) // skip selection of Ohms
      current_selection = current_selection + direction;
  }
  paintBox(current_selection, true);
  selection_timer = millis();
  display.display();
}

void deselectBox(uint8_t box){
  paintBox(box, false);
  current_selection = -1 ;
};

void paintBox(uint8_t box, bool invert){
  uint16_t bg = BLACK;
  uint16_t fg = WHITE;
  uint8_t measure_pos[] = {5,26,47,68,111};
  uint8_t label_pos[] = {14,35,56,77,120};
  char* labels[] = {"VOLTS", "AMPS", "WATTS", "OHMS", "VOLTS"};
  if (invert){
    bg = WHITE;
    fg = BLACK;
  }
  display.fillRect(control_box_positions[box], 0, control_box_width, control_box_height, bg);
  lStringVert(measure_pos[box], 29, invert, measure_str[box]);
  sStringVert(label_pos[box], 29, invert, labels[box]);
}

void paintInfoBox(){
  display.fillRect(control_box_positions[3], 0, control_box_width, control_box_height, WHITE);
  lStringVert(68, 29, true, "SET#");
  sStringVert(77, 29, true, "aEXIT");
}

void setBattery(uint16_t reading){
  display.fillRect(89, 3, 8, 25, BLACK);
  display.fillRect(89, 3, 8, map(reading, 0, 1023, 1, 25), WHITE);
  display.display();
  battery_level = reading;
}


void loop() {

  if (digitalRead(BUTTON_DOWN) == 0){
    if (millis() > keypress_timer + 150){
      Serial.println("down");
      selectionChange(1);
      keypress_timer = millis();
    }
  }
  if (digitalRead(BUTTON_UP) == 0){
    if (millis() > keypress_timer + 150){
      Serial.println("up");
      selectionChange(-1);
      keypress_timer = millis();
    }
  }

  if (current_selection < 0 ){
    if (digitalRead(BUTTON_SEL) == 0){
      if (millis() > keypress_timer + 150){
        Serial.println("sel");
        keypress_timer = millis();
      }
    }
    if (digitalRead(BUTTON_MODE) == 0){
      if (millis() > keypress_timer + 150){
        Serial.println("mode");
        setBattery(random(1023));
        keypress_timer = millis();
      }
    }

  } else {
    if (digitalRead(BUTTON_SEL) == 0){
      // enter param edit mode
      paintInfoBox();
      keypress_timer = millis();
    }
    if (selection_timer + 10000 < millis()){
      deselectBox(current_selection);
      paintBox(3, false); //reset the instruction box
      display.display();
    }
  }

}

//void testdrawbitmap(const uint8_t *bitmap, uint8_t w, uint8_t h) {
//  uint8_t icons[NUMFLAKES][3];
//  srandom(666);     // whatever seed
//
//  // initialize
//  for (uint8_t f=0; f< NUMFLAKES; f++) {
//    icons[f][XPOS] = random() % display.width();
//    icons[f][YPOS] = 0;
//    icons[f][DELTAY] = random() % 5 + 1;
//
//    Serial.print("x: ");
//    Serial.print(icons[f][XPOS], DEC);
//    Serial.print(" y: ");
//    Serial.print(icons[f][YPOS], DEC);
//    Serial.print(" dy: ");
//    Serial.println(icons[f][DELTAY], DEC);
//  }
//
//  while (1) {
//    // draw each icon
//    for (uint8_t f=0; f< NUMFLAKES; f++) {
//      display.drawBitmap(icons[f][XPOS], icons[f][YPOS], logo16_glcd_bmp, w, h, WHITE);
//    }
//    display.display();
//    delay(200);
//
//    // then erase it + move it
//    for (uint8_t f=0; f< NUMFLAKES; f++) {
//      display.drawBitmap(icons[f][XPOS], icons[f][YPOS],  logo16_glcd_bmp, w, h, BLACK);
//      // move it
//      icons[f][YPOS] += icons[f][DELTAY];
//      // if its gone, reinit
//      if (icons[f][YPOS] > display.height()) {
//	icons[f][XPOS] = random() % display.width();
//	icons[f][YPOS] = 0;
//	icons[f][DELTAY] = random() % 5 + 1;
//      }
//    }
//   }
//}
//
//
//void testdrawchar(void) {
//  display.setTextSize(1);
//  display.setTextColor(WHITE);
//  display.setCursor(0,0);
//
//  for (uint8_t i=0; i < 168; i++) {
//    if (i == '\n') continue;
//    display.write(i);
//    if ((i > 0) && (i % 21 == 0))
//      display.println();
//  }
//  display.display();
//}
//
//void testdrawcircle(void) {
//  for (int16_t i=0; i<display.height(); i+=2) {
//    display.drawCircle(display.width()/2, display.height()/2, i, WHITE);
//    display.display();
//  }
//}
//
//void testfillrect(void) {
//  uint8_t color = 1;
//  for (int16_t i=0; i<display.height()/2; i+=3) {
//    // alternate colors
//    display.fillRect(i, i, display.width()-i*2, display.height()-i*2, color%2);
//    display.display();
//    color++;
//  }
//}
//
//void testdrawtriangle(void) {
//  for (int16_t i=0; i<min(display.width(),display.height())/2; i+=5) {
//    display.drawTriangle(display.width()/2, display.height()/2-i,
//                     display.width()/2-i, display.height()/2+i,
//                     display.width()/2+i, display.height()/2+i, WHITE);
//    display.display();
//  }
//}
//
//void testfilltriangle(void) {
//  uint8_t color = WHITE;
//  for (int16_t i=min(display.width(),display.height())/2; i>0; i-=5) {
//    display.fillTriangle(display.width()/2, display.height()/2-i,
//                     display.width()/2-i, display.height()/2+i,
//                     display.width()/2+i, display.height()/2+i, WHITE);
//    if (color == WHITE) color = BLACK;
//    else color = WHITE;
//    display.display();
//  }
//}
//
//void testdrawroundrect(void) {
//  for (int16_t i=0; i<display.height()/2-2; i+=2) {
//    display.drawRoundRect(i, i, display.width()-2*i, display.height()-2*i, display.height()/4, WHITE);
//    display.display();
//  }
//}
//
//void testfillroundrect(void) {
//  uint8_t color = WHITE;
//  for (int16_t i=0; i<display.height()/2-2; i+=2) {
//    display.fillRoundRect(i, i, display.width()-2*i, display.height()-2*i, display.height()/4, color);
//    if (color == WHITE) color = BLACK;
//    else color = WHITE;
//    display.display();
//  }
//}
//
//void testdrawrect(void) {
//  for (int16_t i=0; i<display.height()/2; i+=2) {
//    display.drawRect(i, i, display.width()-2*i, display.height()-2*i, WHITE);
//    display.display();
//  }
//}
//
//void testdrawline() {
//  for (int16_t i=0; i<display.width(); i+=4) {
//    display.drawLine(0, 0, i, display.height()-1, WHITE);
//    display.display();
//  }
//  for (int16_t i=0; i<display.height(); i+=4) {
//    display.drawLine(0, 0, display.width()-1, i, WHITE);
//    display.display();
//  }
//  delay(250);
//
//  display.clearDisplay();
//  for (int16_t i=0; i<display.width(); i+=4) {
//    display.drawLine(0, display.height()-1, i, 0, WHITE);
//    display.display();
//  }
//  for (int16_t i=display.height()-1; i>=0; i-=4) {
//    display.drawLine(0, display.height()-1, display.width()-1, i, WHITE);
//    display.display();
//  }
//  delay(250);
//
//  display.clearDisplay();
//  for (int16_t i=display.width()-1; i>=0; i-=4) {
//    display.drawLine(display.width()-1, display.height()-1, i, 0, WHITE);
//    display.display();
//  }
//  for (int16_t i=display.height()-1; i>=0; i-=4) {
//    display.drawLine(display.width()-1, display.height()-1, 0, i, WHITE);
//    display.display();
//  }
//  delay(250);
//
//  display.clearDisplay();
//  for (int16_t i=0; i<display.height(); i+=4) {
//    display.drawLine(display.width()-1, 0, 0, i, WHITE);
//    display.display();
//  }
//  for (int16_t i=0; i<display.width(); i+=4) {
//    display.drawLine(display.width()-1, 0, i, display.height()-1, WHITE);
//    display.display();
//  }
//  delay(250);
//}
