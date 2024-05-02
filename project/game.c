#include <msp430.h>
#include <libTimer.h>
#include "lcdutils.h"
#include "lcddraw.h"
#include <stdio.h>

// WARNING: LCD DISPLAY USES P1.0.  Do not touch!!! 

#define LED BIT6		/* note that bit zero req'd for display */

#define SW1 1
#define SW2 2
#define SW3 4
#define SW4 8

#define SWITCHES 15

char blue = 31, green = 0, red = 31;
unsigned char step = 0;

static char 
switch_update_interrupt_sense()
{
  char p2val = P2IN;
  /* update switch interrupt to detect changes from current buttons */
  P2IES |= (p2val & SWITCHES);	/* if switch up, sense down */
  P2IES &= (p2val | ~SWITCHES);	/* if switch down, sense up */
  return p2val;
}

void 
switch_init()			/* setup switch */
{  
  P2REN |= SWITCHES;		/* enables resistors for switches */
  P2IE |= SWITCHES;		/* enable interrupts from switches */
  P2OUT |= SWITCHES;		/* pull-ups for switches */
  P2DIR &= ~SWITCHES;		/* set switches' bits for input */
  switch_update_interrupt_sense();
}

int switches = 0;

void
switch_interrupt_handler()
{
  char p2val = switch_update_interrupt_sense();
  switches = ~p2val & SWITCHES;
}


// axis zero for col, axis 1 for row

short drawPos[2] = {1,10}, controlPos[2] = {2, 10};
short colVelocity = 3, colLimits[2] = {1, screenWidth+1};
short rowVelocity = 6, rowLimits[2] = {0, screenHeight+1};

void
draw_ball(int col, int row, unsigned short color)
{
  fillRectangle(col-1, row-1, 3, 3, color);
}


void
screen_update_ball()
{
  for (char axis = 0; axis < 2; axis ++) 
    if (drawPos[axis] != controlPos[axis]) /* position changed? */
      goto redraw;
  return;			/* nothing to do */
 redraw:
  draw_ball(drawPos[0], drawPos[1], COLOR_BLUE); /* erase */
  for (char axis = 0; axis < 2; axis ++) 
    drawPos[axis] = controlPos[axis];
  draw_ball(drawPos[0], drawPos[1], COLOR_WHITE); /* draw */
}
  

short redrawScreen = 1;
u_int controlFontColor = COLOR_GREEN;
int barrier[4] = {0, 0, 0, 0}; // barrierLeft, barrierUp, barrierDown, barrierRight
short delay = 5;
short points = 0;

void drawBarrier(int dir, int toggle);
void drawPoints(int num);
void wdt_c_handler()
{
  static int secCount = 0;

  secCount ++;
  if (secCount >= 25) {		/* 10/sec */
   
    {				/* move ball */
      short oldCol = controlPos[0];
      short newCol = oldCol + colVelocity;
      if (newCol <= colLimits[0]) {
	if (colLimits[0] == 1) {points = 0; drawPoints(0);}
	colVelocity = -colVelocity;
	points++;
	drawPoints(points);
      }	else if (newCol >= colLimits[1]) {
	if (colLimits[1] == screenWidth+1) {points = 0; drawPoints(0);}
	colVelocity = -colVelocity;
        points++;
        drawPoints(points);
      } else {
	controlPos[0] = newCol;
      }

      short oldRow = controlPos[1];
      short newRow = oldRow + rowVelocity;
      if (newRow <= rowLimits[0]) {
	if (rowLimits[0] == 0) {points = 0; drawPoints(0);}
        rowVelocity = -rowVelocity;
	points++;
	drawPoints(points);
      } else if (newRow >= rowLimits[1]) {
	if (rowLimits[1] == screenHeight+1) {points = 0; drawPoints(0);}
        rowVelocity = -rowVelocity;
        points++;
        drawPoints(points);
      } else {
        controlPos[1] = newRow;
      }
    }

    {
      if (switches & SW1) {
	drawBarrier(1, 0);
	colLimits[0] = 4;
	barrier[0] = delay;
      }
      if (switches & SW2) {
	drawBarrier(2, 0);
	rowLimits[0] = 3;
	barrier[1] = delay;
      }
      if (switches & SW3) {
	drawBarrier(3, 0);
	rowLimits[1] = screenHeight-4;
	barrier[2] = delay;
      }
      if (switches & SW4) {
	drawBarrier(4, 0);
	colLimits[1] = screenWidth-4;
	barrier[3] = delay;
      }

      for (int i = 0; i < 4; i++) {
	if (barrier[i] > 0) {
	  barrier[i]--;
	  if (barrier[i] == 0) {
	    drawBarrier(i+1, 1);
          } 
	}
      }

      if (step <= 30)
	step ++;
      else
	step = 0;
      secCount = 0;
    }
    redrawScreen = 1;
  }
}
  
void update_shape();

void main()
{
  
  P1DIR |= LED;		/**< Green led on when CPU on */
  P1OUT |= LED;
  configureClocks();
  lcd_init();
  switch_init();
  
  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */
  
  clearScreen(COLOR_BLUE);
  while (1) {			/* forever */
    if (redrawScreen) {
      redrawScreen = 0;
      update_shape();
    }
    P1OUT &= ~LED;	/* led off */
    or_sr(0x10);	/**< CPU OFF */
    P1OUT |= LED;	/* led on */
  }
}

void
drawBarrier(int dir, int toggle) {
  u_int barColor = COLOR_RED;
  if (toggle) {
    barColor = COLOR_BLUE;
  }
  
  switch(dir) {
  case 1:
    fillRectangle(0,0, 5, screenHeight, barColor);
    colLimits[0] = 1;
    break;
  case 2:
    fillRectangle(0,0, screenWidth, 5, barColor);
    rowLimits[0] = 0;
    break;
  case 3:
    fillRectangle(0,screenHeight-5, screenWidth, 5, barColor);
    rowLimits[1] = screenHeight+1;
    break;
  case 4:
    fillRectangle(screenWidth-5,0, 5, screenHeight, barColor);
    colLimits[1] = screenWidth+1;
    break;
  }
  
}  

void drawPoints(int num) {
  char str[5];
  sprintf(str, "%d", num);
  fillRectangle(screenWidth/2, screenHeight/2, 10, 10, COLOR_BLUE);
  drawString5x7(screenWidth/2, screenHeight/2, str, COLOR_GREEN, COLOR_RED);
}
    
void
update_shape()
{
  screen_update_ball();
}
   


void
__interrupt_vec(PORT2_VECTOR) Port_2(){
  if (P2IFG & SWITCHES) {	      /* did a button cause this interrupt? */
    P2IFG &= ~SWITCHES;		      /* clear pending sw interrupts */
    switch_interrupt_handler();	/* single handler for all switches */
  }
}
