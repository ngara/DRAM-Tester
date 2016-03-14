// -*- C -*-

/* Simple DRAM tester
 * Chris Osborn <fozztexx@fozztexx.com>
 * http://insentricity.com/a.cl/252
 *
 * Status LED is on while testing and blinks after each pass
 * Done LED will flash after all four passes
 * Status and Done LEDs will be ON after all tests pass
 * Status OFF and Done ON means testing failed
 *
 * TODO:
 *   - Rolling bit pattern to test for failed address lines
 *   - Test 4 bit nibble mode
 *   - Test read-modify-write
 *   - Test RAS-only refresh
 *   - Test CAS-before-RAS
 *   - Test limits of refresh time (8ms)
 *   - Fast testing using TF pin
 */

#define DIN             2
#define DOUT            3
#define CAS             5
#define RAS             6
#define WE              7

#define STATUS		4
#define DONE		19

#define ADDR_BITS       10

void fillSame(int val);
void fillAlternating(int start);

void setup()
{
  int mask;

  
  pinMode(DIN, OUTPUT);
  pinMode(DOUT, INPUT);

  pinMode(CAS, OUTPUT);
  pinMode(RAS, OUTPUT);
  pinMode(WE, OUTPUT);

  /* 10 is max address bits, even if chip is smaller */
  mask = (1 << 10) - 1; 
  DDRB = mask & 0x3f;
  mask >>= 6;
  DDRC = mask & 0x0f;
  
  digitalWrite(CAS, HIGH);
  digitalWrite(RAS, HIGH);
  digitalWrite(WE, HIGH);

  pinMode(STATUS, OUTPUT);
  pinMode(DONE, OUTPUT);

  digitalWrite(STATUS, HIGH);
  digitalWrite(DONE, LOW);
}

void loop()
{
  int i;


  fillSame(0);
  digitalWrite(STATUS, LOW);
  delay(250);
  digitalWrite(STATUS, HIGH);

  fillSame(1);
  digitalWrite(STATUS, LOW);
  delay(250);
  digitalWrite(STATUS, HIGH);

  fillAlternating(0);
  digitalWrite(STATUS, LOW);
  delay(250);
  digitalWrite(STATUS, HIGH);

  fillAlternating(1);
  digitalWrite(STATUS, LOW);
  delay(250);
  digitalWrite(STATUS, HIGH);

  for (i = 4; i; i--) {
    digitalWrite(DONE, HIGH);
    delay(500);
    digitalWrite(DONE, LOW);
    delay(500);
  }

  digitalWrite(DONE, HIGH);
}

static inline int setAddress(int row, int col, int wrt)
{
  int val = 0;


  PORTB = row & 0x3f;
  PORTC = (PORTC & 0xf0) | (row >> 6) & 0x0f;
  digitalWrite(RAS, LOW);

  if (wrt)
    digitalWrite(WE, LOW);

  PORTB = col & 0x3f;
  PORTC = (PORTC & 0xf0) | (col >> 6) & 0x0f;
  digitalWrite(CAS, LOW);

  if (wrt)
    digitalWrite(WE, HIGH);
  else
    val = digitalRead(DOUT);
 
  digitalWrite(CAS, HIGH);
  digitalWrite(RAS, HIGH);

  return val;
}

void fail(int row, int col)
{
  digitalWrite(STATUS, LOW);
  digitalWrite(DONE, HIGH);

  while (1)
    ;
}

void fillSame(int val)
{
  int row, col;


  digitalWrite(DIN, val);
  
  for (col = 0; col < (1 << ADDR_BITS); col++)
    for (row = 0; row < (1 << ADDR_BITS); row++)
      setAddress(row, col, 1);

  /* Reverse DIN in case DOUT is floating */
  digitalWrite(DIN, !val);
 
  for (col = 0; col < (1 << ADDR_BITS); col++)
    for (row = 0; row < (1 << ADDR_BITS); row++)
      if (setAddress(row, col, 0) != val)
        fail(row, col);

  return;
}

void fillAlternating(int start)
{
  int row, col, i;


  i = start;
  for (col = 0; col < (1 << ADDR_BITS); col++) {
    for (row = 0; row < (1 << ADDR_BITS); row++) {
      digitalWrite(DIN, i);
      i = !i;
      setAddress(row, col, 1);
    }
  }

  for (col = 0; col < (1 << ADDR_BITS); col++) {
    for (row = 0; row < (1 << ADDR_BITS); row++) { 
      if (setAddress(row, col, 0) != i)
        fail(row, col);

      i = !i;
    }
  }

  return;
}
