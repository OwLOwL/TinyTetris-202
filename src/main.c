#undef F_CPU
#define F_CPU 20000000

#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include <avr/interrupt.h>

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

    //Data 0 - Clear display
    startMiniTinyI2C(LCD_I2C_ADDR, false);
    writeMiniTinyI2C(LCD_DATA);
    for (uint16_t i = 0; i <= 255; i++) {
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
#define BOARD_ROWS 96
#define BOARD_START_ROW 0
#define BOARD_END_ROW (BOARD_START_ROW + BOARD_ROWS)
#define BOARD_START_PAGE 0
#define BOARD_END_PAGE (BOARD_START_PAGE + 5)
#define BOARD_BASELINE_THICKNESS 3
#define BOARD_TILE_HEIGHT 4
#define BOARD_TILE_START_X 4
#define BOARD_TILE_START_Y 1
#define BOARD_TILE_START_ROT 0
#define NUM_TILES 7

#define SCORE_POS_MARGIN 6
#define SCORE_ROW_START (BOARD_END_ROW + BOARD_BASELINE_THICKNESS + SCORE_POS_MARGIN)
#define SCORE_ROW_END (SCORE_ROW_START + 4)
#define SCORE_PAGE_START 1
#define SCORE_PAGE_END (SCORE_PAGE_START + 3)

#define HISCORE_ROW_START (SCORE_ROW_END + 4)
#define HISCORE_ROW_END (HISCORE_ROW_START + 4)
#define HISCORE_PAGE_START 1
#define HISCORE_PAGE_END (HISCORE_PAGE_START + 3)

#define NEXT_TILE_PAGE_START 6
#define NEXT_TILE_PAGE_END (NEXT_TILE_PAGE_START + 1)
#define NEXT_TILE_ROW_START 78
#define NEXT_TILE_ROW_END (NEXT_TILE_ROW_START + 10)

#define LEVEL_PAGE_START 7
#define LEVEL_PAGE_END (LEVEL_PAGE_START)
#define LEVEL_ROW_START 38
#define LEVEL_ROW_END (LEVEL_ROW_START + 4)

#define LINES_PAGE_START 6
#define LINES_PAGE_END (LINES_PAGE_START + 1)
#define LINES_ROW_START 58
#define LINES_ROW_END (LINES_ROW_START + 4)

#define SCORE_ATTACHED 0xA
#define SCORE_LINE_BASE 0x64
#define SCORE_LINE_BONUS 0x32

uint8_t gGameBoard[3][10] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

int8_t gPos[2] = { BOARD_TILE_START_X, BOARD_TILE_START_Y };
uint8_t gRot = BOARD_TILE_START_ROT;
uint8_t gCurTile = 0;
uint8_t gNextTile = 0;
uint32_t gScore = 0x00000000;
uint32_t gHighScore = 0x12345678;
uint8_t gLevel = 0x01;
uint16_t gLines = 0x0000;

#define BUTTON_ADC_MARGIN    0x05
#define BUTTON_LEFT_VALUE_L  (0xD5 - BUTTON_ADC_MARGIN) 
#define BUTTON_LEFT_VALUE_H  (0xD5 + BUTTON_ADC_MARGIN) 
#define BUTTON_RIGHT_VALUE_L (0x57 - BUTTON_ADC_MARGIN)
#define BUTTON_RIGHT_VALUE_H (0x57 + BUTTON_ADC_MARGIN)
#define BUTTON_DOWN_VALUE_L  (0x7F - BUTTON_ADC_MARGIN)
#define BUTTON_DOWN_VALUE_H  (0x7F + BUTTON_ADC_MARGIN)
#define BUTTON_ROT_CCW_L     (0x7F - BUTTON_ADC_MARGIN)
#define BUTTON_ROT_CCW_H     (0x7F + BUTTON_ADC_MARGIN)
#define BUTTON_ROT_CW_L      (0xD5 - BUTTON_ADC_MARGIN)
#define BUTTON_ROT_CW_H      (0xD5 + BUTTON_ADC_MARGIN)
#define ADC_DIRECTIONAL_PIN ADC_MUXPOS_AIN6_gc 
#define ADC_ROTATIONAL_PIN  ADC_MUXPOS_AIN7_gc 
uint8_t gCurrentADCPin = ADC_DIRECTIONAL_PIN;

const uint8_t tileMap[4] = {
    0b00000000,
    0b11100000,
    0b11100000,
    0b11100000
};

const uint8_t tiles[NUM_TILES] = {
    /* X X O O
       X X O O */
    0b11001100,

    /* X X X X
       O O O O */
    0b11110000,

    /* O X X O
       X X O O */
    0b01101100,

    /* X X O O
       O X X O */
    0b11000110,

    /* O O X O
       X X X O */
    0b00101110,

    /* X O O O
       X X X O */
    0b10001110,

    /* O X O O
       X X X O */
    0b01001110,
};

const uint8_t numbers[16][3] = {
    /* 0
        0111
        0101
        0101
        0101
        0111
        0000
    */
    { 0b01110101, 0b01010101, 0b01110000 },
    /* 1
        0010
        0011
        0010
        0010
        0111
        0000
    */
    { 0b00100011, 0b00100010, 0b01110000 },
    /* 2
        0111
        0100
        0010
        0001
        0111
        0000
    */
    { 0b01110100, 0b00100001, 0b01110000 },
    /* 3
        0111
        0100
        0110
        0100
        0111
        0000
    */
    { 0b01110100, 0b01100100, 0b01110000 },
    /* 4
        0101
        0101
        0111
        0100
        0100
        0000
    */
    { 0b01010101, 0b01110100, 0b01000000 },
    /* 5
        0111
        0001
        0111
        0100
        0111
        0000
    */
    { 0b01110001, 0b01110100, 0b01110000 },
    /* 6
        0111
        0001
        0111
        0101
        0111
        0000
    */
    { 0b01110001, 0b01110101, 0b01110000 },
    /* 7
        0111
        0100
        0010
        0010
        0010
        0000
    */
    { 0b01110100, 0b00100010, 0b00100000 },
    /* 8
        0111
        0101
        0111
        0101
        0111
        0000
    */
    { 0b01110101, 0b01110101, 0b01110000 },
    /* 9
        0111
        0101
        0111
        0100
        0111
        0000
    */
    { 0b01110101, 0b01110100, 0b01110000 },
    /* A
        0111
        0101
        0111
        0101
        0101
        0000
    */
    { 0b01110101, 0b01110101, 0b01010000 },
    /* B
        0111
        0101
        0011
        0101
        0111
        0000
    */
    { 0b01110101, 0b00110101, 0b01110000 },
    /* C
        0111
        0001
        0001
        0001
        0111
        0000
    */
    { 0b01110001, 0b00010001, 0b01110000 },
    /* D
        0011
        0101
        0101
        0101
        0011
        0000
    */
    { 0b00110101, 0b01010101, 0b00110000 },
    /* E
        0111
        0001
        0011
        0001
        0111
        0000
    */
    { 0b01110001, 0b00110001, 0b01110000 },
    /* F
        0111
        0001
        0011
        0001
        0001
        0000
    */
    { 0b01110001, 0b00110001, 0b00010000 },
};

void startDrawing(uint8_t startPage, uint8_t endPage, uint8_t startRow, uint8_t endRow) {
    setDisplayArea(startPage, endPage, startRow, endRow);
    startMiniTinyI2C(LCD_I2C_ADDR, false);
    writeMiniTinyI2C(LCD_DATA);
}

bool boardIndices(uint8_t * x, uint8_t * y, uint8_t page, uint8_t row, bool left) {
    if (page == BOARD_START_PAGE && left) return false;
    if (page == BOARD_END_PAGE && !left) return false;

    *x = (page * 2) - left;
    *y = (row - BOARD_START_ROW) >> 2;

    return true;
}

bool populatedCell(uint8_t x, uint8_t y) {
    uint8_t yy = y >> 3;
    return (gGameBoard[yy][x] & (1 << (y & 0x7)));
}

void populateCell(uint8_t x, uint8_t y, bool set) {
    uint8_t yy = y >> 3;
    if (set)
        gGameBoard[yy][x] |= (1 << (y & 0x7));
    else 
        gGameBoard[yy][x] &= ~(1 << (y & 0x7));
}

void drawBoard(uint8_t start, uint8_t end) {
    uint8_t out = 0;
    startDrawing(BOARD_START_PAGE, BOARD_END_PAGE, start, end);
    for (uint8_t page = BOARD_START_PAGE; page <= BOARD_END_PAGE; page++) {
        for (uint8_t row = start; row <= end; row++) {
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

void drawTileRows() {
    int8_t startRow = BOARD_START_ROW + (gPos[1] << 2) - (2 * BOARD_TILE_HEIGHT);
    int8_t endRow = BOARD_START_ROW + (gPos[1] << 2) + (3 * BOARD_TILE_HEIGHT);
    drawBoard(startRow < 0 ? 0 : startRow, endRow > BOARD_END_ROW ? BOARD_END_ROW : endRow);
}

void drawFullBoard() {
    drawBoard(0, BOARD_END_ROW + BOARD_BASELINE_THICKNESS);
}

bool addOrRemoveTile(bool add, bool check, int8_t *pos) {
    const uint8_t *t = &tiles[gCurTile];

    int8_t posX = 0;
    int8_t posY = 0;
    for (uint8_t bit = 0; bit < 8; bit++) {
        if ((*t >> bit) & 1) {

            if (gRot == 0 || gCurTile == 0) {
                posX = pos[0] + (3-(bit & 0x03)) - 1;
                posY = pos[1] + (1-(bit >> 2));
            } else if (gRot == 1 || gCurTile < 4) {
                posX = pos[0] + (bit >> 2);
                posY = pos[1] + (3-(bit & 0x03)) - 1;
            } else if (gRot == 2) {
                posX = pos[0] + (bit & 0x03) - 1;
                posY = pos[1] + (bit >> 2);
            } else {
                posX = pos[0] + (1-(bit >> 2));
                posY = pos[1] + (bit & 0x03) - 1;
            }
            
            if ((add && populatedCell(posX, posY)) || posY >= 24 || posX < 0 || posX > 9) {
                return false; //Bail!
            }
            if (!check)
                populateCell(posX, posY, add);
        }
    }

    return true;
}

bool updateTilePos(int8_t x, int8_t y) {
    int8_t pos[2] = {gPos[0] + x, gPos[1] + y};

    //Remove old tile
    addOrRemoveTile(false, false, gPos);

    //Check if new pos is clear
    if (!addOrRemoveTile(true, true, pos)) {
        addOrRemoveTile(true, false, gPos);
        return false;
    }
    
    //Update position
    gPos[0] = pos[0];
    gPos[1] = pos[1];

    //add new tile position
    addOrRemoveTile(true, false, gPos);

    return true;
}

void plantASeed() {
    gNextTile = 4; //Find something better!
}

void drawNextSegment(bool left, bool right) {
        uint8_t out = 0x00;

        for (uint8_t i = 0; i < 4; i++) {
            if(left)
                out |= tileMap[i] >> 4;
            if(right)
                out |= tileMap[i];
            writeMiniTinyI2C(out);
        }

}

void drawNextTile() {
    startDrawing(NEXT_TILE_PAGE_START, NEXT_TILE_PAGE_END, NEXT_TILE_ROW_START, NEXT_TILE_ROW_END);

    writeMiniTinyI2C(0xFF);
    drawNextSegment(tiles[gNextTile] & 0b10000000, tiles[gNextTile] & 0b01000000);
    drawNextSegment(tiles[gNextTile] & 0b00001000, tiles[gNextTile] & 0b00000100);
    writeMiniTinyI2C(0x00);
    writeMiniTinyI2C(0xFF);
    writeMiniTinyI2C(0xFF);
    drawNextSegment(tiles[gNextTile] & 0b00100000, tiles[gNextTile] & 0b00010000);
    drawNextSegment(tiles[gNextTile] & 0b00000010, tiles[gNextTile] & 0b00000001);
    writeMiniTinyI2C(0x00);
    writeMiniTinyI2C(0xFF);

    stopMiniTinyI2C();
}

void updateNextTile() {
    gNextTile = (gNextTile + 3) % 7;
    drawNextTile(); 
}

void injectNextTile() {
    gPos[0] = BOARD_TILE_START_X;
    gPos[1] = BOARD_TILE_START_Y;
    gRot = BOARD_TILE_START_ROT;
    gCurTile = gNextTile;
    updateNextTile();
}

void drawNumberSegments(uint8_t val) {
    uint8_t out = 0x00;
    uint8_t left = (val & 0xF0) >> 4;
    uint8_t right = val & 0x0F;
    out  = (numbers[left][0] & 0xF0) >> 4;
    out |= (numbers[right][0] & 0xF0);
    writeMiniTinyI2C(out);
    out  = (numbers[left][0] & 0x0F);
    out |= (numbers[right][0] & 0x0F) << 4;
    writeMiniTinyI2C(out);
    out  = (numbers[left][1] & 0xF0) >> 4;
    out |= (numbers[right][1] & 0xF0);
    writeMiniTinyI2C(out);
    out  = (numbers[left][1] & 0x0F);
    out |= (numbers[right][1] & 0x0F) << 4;
    writeMiniTinyI2C(out);
    out  = (numbers[left][2] & 0xF0) >> 4;
    out |= (numbers[right][2] & 0xF0);
    writeMiniTinyI2C(out);
}

void drawScore(uint8_t scoreAdd) {
    gScore += scoreAdd;

    startDrawing(SCORE_PAGE_START, SCORE_PAGE_END, SCORE_ROW_START, SCORE_ROW_END);

    drawNumberSegments((gScore & 0xFF000000) >> 24);
    drawNumberSegments((gScore & 0x00FF0000) >> 16);
    drawNumberSegments((gScore & 0x0000FF00) >> 8);
    drawNumberSegments((gScore & 0x000000FF));

    stopMiniTinyI2C();     

    startDrawing(HISCORE_PAGE_START, HISCORE_PAGE_END, HISCORE_ROW_START, HISCORE_ROW_END);

    drawNumberSegments((gHighScore & 0xFF000000) >> 24);
    drawNumberSegments((gHighScore & 0x00FF0000) >> 16);
    drawNumberSegments((gHighScore & 0x0000FF00) >> 8);
    drawNumberSegments((gHighScore & 0x000000FF));

    stopMiniTinyI2C();     
}

void drawLevel() {
    startDrawing(LEVEL_PAGE_START, LEVEL_PAGE_END, LEVEL_ROW_START, LEVEL_ROW_END);

    drawNumberSegments(gLevel);

    stopMiniTinyI2C();     
}

void drawLines() {
    startDrawing(LINES_PAGE_START, LINES_PAGE_END, LINES_ROW_START, LINES_ROW_END);

    drawNumberSegments((gLines & 0xFF00) >> 8);
    drawNumberSegments((gLines & 0x00FF));

    stopMiniTinyI2C();     
}

void wait_ms(uint16_t ms) {
    _delay_ms(ms);
}

bool checkGameOver() {
    return (gPos[1] == BOARD_TILE_START_Y);
}

void drawEndSequence() {
    for (uint8_t i = 2; i >= 0; i--) {
        for (uint8_t j = 8; j > 0; j--) {
            for (uint8_t column = 0; column < 10; column++) {
                gGameBoard[i][column] |= (0x1 << (j-1));
            }
            drawFullBoard();
        }
    }
}

void checkCompletedLines() {
    uint8_t completedLines = 0;
    //scan board for complete lines
    for (uint8_t i = 2; i >= 0; i--) {
        for (uint8_t j = 8; j > 0; j--) {
            bool completeLine = true;
            for (uint8_t column = 0; (column < 10) && completeLine; column++) {
                gGameBoard[i][column] |= (0x1 << (j-1));
            }
            if (completeLine) {
                completedLines++;
                //Remove the line and shift all above down one
            }
        }
    }
    gLines += completedLines;
    drawScore(SCORE_LINE_BASE + SCORE_LINE_BONUS*completedLines);
}

void initKeys() {
    //Set ADC to VDD reference voltage and prescaler to 16 divisor (20/16 = 1.25MHz)
    ADC0.CTRLC = ADC_SAMPCAP_bm | ADC_REFSEL_VDDREF_gc | ADC_PRESC_DIV16_gc;

    //Configure ADC on pin 2 (AIN6) - direction and drop
    //Turn off Digital input buffer
    PORTA.PIN2CTRL = PORT_ISC_INPUT_DISABLE_gc;

    //Configure ADC on pin 3 (AIN7) - rotations
    //Turn off Digital input buffer
    PORTA.PIN3CTRL = PORT_ISC_INPUT_DISABLE_gc;

    //Enable global interrupts
    CPU_SREG |= 0b10000000;

    //Clear conversion interrupt (Write 1 to clear!)
    ADC0.INTFLAGS = ADC_RESRDY_bm;

    //Enable conversion interrupt
    ADC0.INTCTRL = ADC_RESRDY_bm;

    //8bit ADC and enable
    ADC0.CTRLA = ADC_RESSEL_8BIT_gc | ADC_ENABLE_bm;

    //Default to AIN6 pin
    ADC0.MUXPOS = gCurrentADCPin;

    //Start ADC loop!
    ADC0.COMMAND |= ADC_STCONV_bm;
}

ISR(ADC0_RESRDY_vect) {
    //Check if it was direction or rotation pin
    //Conversion result is in ADC0.RESL
    uint8_t result = ADC0.RESL;

    if (gCurrentADCPin == ADC_DIRECTIONAL_PIN) {
        //Direction (AIN6):
        //"Left"  (0xD5)
        //"Right" (0x57)
        //"Down"  (0x7F)
        if (result > BUTTON_LEFT_VALUE_L && result < BUTTON_LEFT_VALUE_H) {
            updateTilePos(-1, 0);
        } else if (result > BUTTON_RIGHT_VALUE_L && result < BUTTON_RIGHT_VALUE_H) {
            updateTilePos(1, 0);
        } else if (result > BUTTON_DOWN_VALUE_L && result < BUTTON_DOWN_VALUE_H) {
            updateTilePos(0, 1);
        }
        gCurrentADCPin = ADC_ROTATIONAL_PIN;
    } else { //ADC_ROTATIONAL_PIN
        //Rotation (AIN7):
        //"Rotation Left (CCW)" (0x7F)
        //"Rotation Right (CW)" (0xD5)

        gCurrentADCPin = ADC_DIRECTIONAL_PIN;
    }

    //Clear conversion interrupt (Write 1 to clear!)
    ADC0.INTFLAGS = ADC_RESRDY_bm;

    //Restart conversation again!
    ADC0.MUXPOS = gCurrentADCPin;

    //Start ADC loop!
    ADC0.COMMAND |= ADC_STCONV_bm;
}

void updateHighScore() {

}

int main() {
    initMiniTinyI2C(1100);

    initDisplay();

    initKeys();

    plantASeed();
    injectNextTile();
    addOrRemoveTile(true, false, gPos);
    drawFullBoard();
//    drawScore(0);
//    drawLevel();
//    drawLines();
    while(1) {
        wait_ms(10);
        if (!updateTilePos(0, 1)) {
            if(checkGameOver()) {
                break;
            }
//            drawScore(SCORE_ATTACHED);
            //checkCompletedLines();
            injectNextTile();
        }
        drawTileRows();
    }

    drawEndSequence();
    updateHighScore();

    while(1) {}
}