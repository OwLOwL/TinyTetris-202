#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>

#include <MiniTinyI2C.h>

#define LCD_I2C_ADDR            0x3C
#define LCD_COMMAND             0x00
#define LCD_COMMAND_PAGE_ADDR   0x22
#define LCD_COMMAND_COLUMN_ADDR 0x21
#define LCD_DATA                0x40

#define INIT_LENGTH 28

const uint8_t DisplayInit[INIT_LENGTH] = {
  0xE4,             // Soft Reset
  0xAE,             // Display OFF
  0xA8, 0x3F,       // set multiplex (HEIGHT-1): 0x1F for 128x32, 0x3F for 128x64
  0xD3, 0x00,       // Display offset to 0
  0x40,             // Set display start line to 0
  0x8D, 0x14,       // Charge pump enabled
  0x20, 0x00,       // Memory addressing mode 0x00 Horizontal 0x01 Vertical
  0xDA, 0x12,       // Set COM Pins hardware configuration to sequential
  0x81, 0xA0,       // Set contrast
  0xD9, 0xFF,       // Set pre-charge period
  0xDB, 0x20,       // Set vcom detect
  
  LCD_COMMAND_PAGE_ADDR, 0x00, 0x07, // Page min to max
  LCD_COMMAND_COLUMN_ADDR, 0x00, 0x7F, // Column min to max

  0xA1,             //Column mapping reversed

  0xA4,             //RAM Content mode

  0xAF  // Display on
};

void displayData(uint8_t data) {
    startMiniTinyI2C(LCD_I2C_ADDR, false);
    writeMiniTinyI2C(LCD_DATA);
    writeMiniTinyI2C(data);
    stopMiniTinyI2C();    
}

void initDisplay() {
    //Command Init
    startMiniTinyI2C(LCD_I2C_ADDR, false);
    writeMiniTinyI2C(LCD_COMMAND);
    for (uint8_t i = 0; i < INIT_LENGTH; i++) {
        writeMiniTinyI2C(DisplayInit[i]);
    }
    stopMiniTinyI2C();

    //Data 0 - Clear display (TODO - FIX! Don't know why <= 255 and 4 rows does not work?)
    startMiniTinyI2C(LCD_I2C_ADDR, false);
    writeMiniTinyI2C(LCD_DATA);
    for (uint8_t i = 0; i < 128; i++) {
        writeMiniTinyI2C(0x00);
        writeMiniTinyI2C(0x00);
        writeMiniTinyI2C(0x00);
        writeMiniTinyI2C(0x00);
        writeMiniTinyI2C(0x00);
        writeMiniTinyI2C(0x00);
        writeMiniTinyI2C(0x00);
        writeMiniTinyI2C(0x00);
    }
    stopMiniTinyI2C();    
}

//Set Page and Column Display update constraints
void setDisplayArea(uint8_t startPage, uint8_t endPage, uint8_t startColumn, uint8_t endColumn) {
    startMiniTinyI2C(LCD_I2C_ADDR, false);
    writeMiniTinyI2C(LCD_COMMAND);
    writeMiniTinyI2C(LCD_COMMAND_PAGE_ADDR);
    writeMiniTinyI2C(startPage);
    writeMiniTinyI2C(endPage);
    writeMiniTinyI2C(LCD_COMMAND_COLUMN_ADDR);
    writeMiniTinyI2C(startColumn);
    writeMiniTinyI2C(endColumn);
    stopMiniTinyI2C();

}

// Game Data and defines
#define BOARD_LEFT_BORDER 0x0E 
#define BOARD_RIGHT_BORDER 0xE0
#define BOARD_START_ROW 22
//#define BOARD_START_ROW 44
#define BOARD_END_ROW (BOARD_START_ROW + 80)
#define BOARD_START_PAGE 0
#define BOARD_END_PAGE (BOARD_START_PAGE + 5)
#define BOARD_BASELINE_THICKNESS 3
uint8_t gGameBoard[3][10] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

const uint8_t tileMap[4] = {
    0b00000000,
    0b11100000,
    0b11100000,
    0b11100000
};

bool boardIndices(uint8_t * x, uint8_t * y, uint8_t page, uint8_t row, bool left) {
    if (page == BOARD_START_PAGE && left) return false;
    if (page == BOARD_END_PAGE && !left) return false;

    // x
    // Page 0 left  -> invalid
    // Page 0 right -> x = 0
    // Page 1 left  -> x = 1
    // Page 1 right -> x = 2
    // Page 2 left  -> x = 3 
    // Page 2 right -> x = 4
    // Page xx left -> x = (page * 2) - 1
    *x = (page * 2) - left;

    // y
    // row 44 -> y = 0
    // row 45 -> y = 0
    // row 46 -> y = 0
    // row 47 -> y = 0
    // row 48 -> y = 1
    // row 49 -> y = 1
    // row 50 -> y = 1
    // row 51 -> y = 1
    // row yy -> y = (row - BOARD_START_ROW) >> 2
    *y = (row - BOARD_START_ROW) >> 2;

    return true;
}

bool populatedCell(uint8_t x, uint8_t y) {
    uint8_t yy = y >> 3;
    return (gGameBoard[yy][x] & (1 << (y & 0x7)));
}

void drawFullBoard() {
    uint8_t out = 0;
    setDisplayArea(BOARD_START_PAGE, BOARD_END_PAGE, 0, (BOARD_END_ROW + BOARD_BASELINE_THICKNESS)); //5*8 pixels width and 128 pixels height
    startMiniTinyI2C(LCD_I2C_ADDR, false);
    writeMiniTinyI2C(LCD_DATA);
    for (uint8_t page = BOARD_START_PAGE; page <= BOARD_END_PAGE; page++) {
        for (uint8_t row = 0; row <= (BOARD_END_ROW + BOARD_BASELINE_THICKNESS); row++) {
            out = 0x00;
            if (row <= BOARD_END_ROW) {
                if (page == BOARD_START_PAGE) { //Or in Left border
                    out |= BOARD_LEFT_BORDER;
                } else if (page == BOARD_END_PAGE) { //Right border
                    out |= BOARD_RIGHT_BORDER;
                }
                if (row >= BOARD_START_ROW) { //actual gamefield
                    uint8_t x = 0;
                    uint8_t y = 0;
                    if (boardIndices(&x, &y, page, row, true) && populatedCell(x, y)) { //Draw stuff (left part of page)
                        //Check current sub-row
                        out |= tileMap[(row - BOARD_START_ROW) & 0x03] >> 4;
                    }
                    if (boardIndices(&x, &y, page, row, false) && populatedCell(x, y)) { //Draw stuff (right part of page)
                        //Check current sub-row
                        out |= (tileMap[(row - BOARD_START_ROW) & 0x03]);
                    }
                }
            } else {
                out = (page == 0)? 0xFE : 0xFF;
            }
            writeMiniTinyI2C(out);
        }
    } 
    stopMiniTinyI2C();    
}

int main() {
    initMiniTinyI2C(1100);

    initDisplay();

    uint8_t temp = 0;
    while(1) {
//        _delay_ms(5000);
        gGameBoard[1][4] = temp;
        drawFullBoard();
        temp++;
    }
}