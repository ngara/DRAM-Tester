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

#define ADDR_BITS       9

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

  digitalWrite(STATUS, HIGH);

  Serial.begin(9600);
}

void loop()
{
  int i;

  Serial.println("Starting Test");

  Serial.println("fillSame(0)");
  signalBetweenTests();
  fillSame(0);

  Serial.println("fillSame(1)");
  signalBetweenTests();
  fillSame(1);
  digitalWrite(STATUS, HIGH);

  Serial.println("fillAlternating(0)");
  signalBetweenTests();
  fillAlternating(0);
  digitalWrite(STATUS, LOW);

  Serial.println("fillAlternating(1)");
  signalBetweenTests();
  fillAlternating(1);
  digitalWrite(STATUS, LOW);
  delay(1000);

  Serial.println("Done and Passed!");

  for (i = 6; i; i--) {
    digitalWrite(STATUS, HIGH);
    delay(250);
    digitalWrite(STATUS, LOW);
    delay(250);
  }

  digitalWrite(STATUS, HIGH);
  while(1) {
    signalPass();
  }
}

void signalBetweenTests() {
  digitalWrite(STATUS, HIGH);
  delay(150);
  digitalWrite(STATUS, LOW);
  delay(150);
  digitalWrite(STATUS, HIGH);
  delay(150);
  digitalWrite(STATUS, LOW);
}

void signalPass() {
  digitalWrite(STATUS, HIGH);
  delay(2000);
  digitalWrite(STATUS, LOW);
  delay(250);
}

void signalFail() {
  digitalWrite(STATUS, HIGH);
  delay(250);
  digitalWrite(STATUS, LOW);
  delay(250);
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

  Serial.print("Fail at row ");
  Serial.print(row);
  Serial.print(" col ");
  Serial.println(col);

  while (1) {
    signalFail();
  }

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
