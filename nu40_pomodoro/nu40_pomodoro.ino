/*
 * NU40 DK Pomodoro Timer
 *
 * Hardware:
 *   - OLED SSD1306 128x64 (I2C: SDA=P0.07, SCL=P0.08)
 *   - Active Buzzer (P0.26 = Arduino pin 26)
 *   - SW1=P0.11, SW2=P0.12, SW4=P0.25
 *
 * Button Functions:
 *   - SW1 (P0.11) : Start / Pause
 *   - SW2 (P0.12) : Focus Time Adjustment (1-minute increments, only on initial screen)
 *   - SW1 Long Press (2 seconds) : Reset
 *   - SW4 (P0.25) : Break Time Adjustment (1-minute increments, only on initial screen)
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── OLED ──────────────────────────────────────────
TwoWire myWire(NRF_TWIM1, NRF_TWIS1, SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQn, 7, 8);
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &myWire, OLED_RESET);

// ── Pin ────────────────────────────────────────────
#define SW1_PIN 11 // Start/Pause
#define SW2_PIN 12 // Focus time adjustment
// SW3 Not Used (Hardware Defect)
#define SW4_PIN 25    // Break time adjustment
#define BUZZER_PIN 26 // Active buzzer (P0.26)

// ── State ──────────────────────────────────────────
enum State
{
  STATE_IDLE,
  STATE_FOCUS,
  STATE_PAUSED,
  STATE_BREAK,
  STATE_DONE
};
State currentState = STATE_IDLE;

// ── Timer Settings ────────────────────────────────────
int focusMinutes = 55; // Default focus time (minutes)
int breakMinutes = 25;  // Default break time (minutes)
const int MIN_FOCUS = 1;
const int MAX_FOCUS = 60;
const int MIN_BREAK = 1;
const int MAX_BREAK = 30;

// ── Timer Variables ────────────────────────────────────
unsigned long timerStartMillis = 0;
unsigned long pausedElapsed = 0; // Elapsed time until pause (ms)
int totalSeconds = 0;            // Total session time (seconds)
int sessionCount = 0;            // Number of completed focus sessions

// ── Button Debounce ──────────────────────────────────
unsigned long lastSW1 = 0, lastSW2 = 0, lastSW4 = 0;
unsigned long sw1PressStart = 0; // SW1 long press detection
const unsigned long DEBOUNCE = 200;

// ── Animation ─────────────────────────────────────
unsigned long lastAnimFrame = 0;
int animFrame = 0;

// ── Function Declarations ──────────────────────────────────────
void drawIdle();
void drawFocus(int remainSec);
void drawPaused(int remainSec);
void drawBreak(int remainSec);
void drawDone();
void drawTomato(int cx, int cy, int r, bool sunglasses, bool paused, bool heart);
void vibrateMotor(int times);
int getRemainingSeconds();

// ══════════════════════════════════════════════════
void setup()
{
  pinMode(SW1_PIN, INPUT_PULLUP);
  pinMode(SW2_PIN, INPUT_PULLUP);
  pinMode(SW4_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  myWire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();

  drawIdle();
}

// ══════════════════════════════════════════════════
void loop()
{
  unsigned long now = millis();

  // ── Button Processing ──────────────────────────────────

  // SW1: Start / Pause
  if (digitalRead(SW1_PIN) == LOW && now - lastSW1 > DEBOUNCE)
  {
    lastSW1 = now;
    if (currentState == STATE_IDLE)
    {
      // Focus session start
      totalSeconds = focusMinutes * 60;
      timerStartMillis = millis();
      pausedElapsed = 0;
      currentState = STATE_FOCUS;
    }
    else if (currentState == STATE_FOCUS)
    {
      // Pause
      pausedElapsed = millis() - timerStartMillis;
      currentState = STATE_PAUSED;
    }
    else if (currentState == STATE_PAUSED)
    {
      // Resume
      timerStartMillis = millis() - pausedElapsed;
      currentState = STATE_FOCUS;
    }
    else if (currentState == STATE_BREAK)
    {
      // Pause during break time
      pausedElapsed = millis() - timerStartMillis;
      currentState = STATE_PAUSED;
    }
    else if (currentState == STATE_DONE)
    {
      // From done screen → idle screen
      currentState = STATE_IDLE;
    }
  }

  // SW2: Focus time adjustment (only on idle screen)
  if (digitalRead(SW2_PIN) == LOW && now - lastSW2 > DEBOUNCE)
  {
    lastSW2 = now;
    if (currentState == STATE_IDLE)
    {
      focusMinutes += 1;
      if (focusMinutes > MAX_FOCUS)
        focusMinutes = MIN_FOCUS;
      drawIdle();
    }
  }

  // SW1 long press (2 seconds): Reset
  if (digitalRead(SW1_PIN) == LOW)
  {
    if (sw1PressStart == 0)
      sw1PressStart = now;
    if (now - sw1PressStart > 2000)
    {
      currentState = STATE_IDLE;
      sessionCount = 0;
      pausedElapsed = 0;
      sw1PressStart = 0;
      lastSW1 = now;
      drawIdle();
      return;
    }
  }
  else
  {
    sw1PressStart = 0;
  }

  // SW4: Break time adjustment (only on idle screen)
  if (digitalRead(SW4_PIN) == LOW && now - lastSW4 > DEBOUNCE)
  {
    lastSW4 = now;
    if (currentState == STATE_IDLE)
    {
      breakMinutes += 1;
      if (breakMinutes > MAX_BREAK)
        breakMinutes = MIN_BREAK;
      drawIdle();
    }
  }

  // ── Timer Logic ────────────────────────────────

  if (currentState == STATE_FOCUS || currentState == STATE_BREAK)
  {
    int remain = getRemainingSeconds();

    // Timer end check
    if (remain <= 0)
    {
      if (currentState == STATE_FOCUS)
      {
        sessionCount++;
        currentState = STATE_BREAK;
        totalSeconds = breakMinutes * 60;
        timerStartMillis = millis();
        pausedElapsed = 0;
      }
      else
      {
        currentState = STATE_DONE;
      }
      vibrateMotor(3);
      return;
    }

    // Update screen (every 1 second)
    if (now - lastAnimFrame >= 1000)
    {
      lastAnimFrame = now;
      animFrame++;
      if (currentState == STATE_FOCUS)
        drawFocus(remain);
      if (currentState == STATE_BREAK)
        drawBreak(remain);
    }
  }

  if (currentState == STATE_PAUSED)
  {
    if (now - lastAnimFrame >= 500)
    {
      lastAnimFrame = now;
      int remain = totalSeconds - (int)(pausedElapsed / 1000);
      drawPaused(remain);
    }
  }

  if (currentState == STATE_DONE)
  {
    if (now - lastAnimFrame >= 600)
    {
      lastAnimFrame = now;
      animFrame++;
      drawDone();
    }
  }
}

// ══════════════════════════════════════════════════
// Calculate remaining time (seconds)
int getRemainingSeconds()
{
  unsigned long elapsed = (millis() - timerStartMillis) / 1000;
  int remain = totalSeconds - (int)elapsed;
  return max(remain, 0);
}

// ══════════════════════════════════════════════════
// Active buzzer alert
void vibrateMotor(int times)
{
  for (int i = 0; i < times; i++)
  {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(120);
  }
}

// ══════════════════════════════════════════════════
// Draw tomato face
// cx, cy: center, r: radius
// sunglasses: sunglasses status, paused: paused eyes, heart: heart eyes
void drawTomato(int cx, int cy, int r, bool sunglasses, bool paused, bool heart)
{
  // body
  display.fillCircle(cx, cy, r, WHITE);

  // leaf (tip)
  display.fillTriangle(cx - 4, cy - r, cx, cy - r - 7, cx + 1, cy - r + 2, WHITE);
  display.fillTriangle(cx + 4, cy - r, cx, cy - r - 7, cx - 1, cy - r + 2, WHITE);

  int eyeY = cy - 3;
  int eyeLX = cx - r / 3;
  int eyeRX = cx + r / 3;

  if (paused)
  {
    // paused bar eyes
    display.fillRect(eyeLX - 4, eyeY - 5, 3, 10, BLACK);
    display.fillRect(eyeLX + 1, eyeY - 5, 3, 10, BLACK);
    display.fillRect(eyeRX - 4, eyeY - 5, 3, 10, BLACK);
    display.fillRect(eyeRX + 1, eyeY - 5, 3, 10, BLACK);
  }
  else if (heart)
  {
    // heart eyes
    display.fillCircle(eyeLX - 2, eyeY - 2, 2, BLACK);
    display.fillCircle(eyeLX + 2, eyeY - 2, 2, BLACK);
    display.fillTriangle(eyeLX - 4, eyeY - 1, eyeLX + 4, eyeY - 1, eyeLX, eyeY + 4, BLACK);
    display.fillCircle(eyeRX - 2, eyeY - 2, 2, BLACK);
    display.fillCircle(eyeRX + 2, eyeY - 2, 2, BLACK);
    display.fillTriangle(eyeRX - 4, eyeY - 1, eyeRX + 4, eyeY - 1, eyeRX, eyeY + 4, BLACK);
  }
  else if (sunglasses)
  {
    // sunglasses
    display.fillRoundRect(eyeLX - 6, eyeY - 4, 12, 9, 2, BLACK);
    display.fillRoundRect(eyeRX - 6, eyeY - 4, 12, 9, 2, BLACK);
    display.drawLine(eyeLX + 6, eyeY, eyeRX - 6, eyeY, BLACK);
  }
  else
  {
    // normal eyes
    display.fillEllipse(eyeLX, eyeY, 4, 5, BLACK);
    display.fillEllipse(eyeRX, eyeY, 4, 5, BLACK);
    display.fillCircle(eyeLX + 1, eyeY + 1, 2, WHITE);
    display.fillCircle(eyeRX + 1, eyeY + 1, 2, WHITE);
  }

  // blush
  display.drawPixel(eyeLX - r / 3 - 1, eyeY + 5, BLACK);
  display.drawPixel(eyeRX + r / 3 + 1, eyeY + 5, BLACK);

  // mouse
  for (int i = -5; i <= 5; i++)
  {
    display.drawPixel(cx + i, cy + r / 2 - (i * i) / 6, BLACK);
  }
}

// ══════════════════════════════════════════════════
// display idle screen
void drawIdle()
{
  display.clearDisplay();

  // focus time pill
  display.drawRoundRect(14, 2, 50, 13, 6, WHITE);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(18, 5);
  display.print("focus");

  // focus time number
  display.setTextSize(2);
  display.setCursor(10, 18);
  char buf[6];
  sprintf(buf, "%02d:00", focusMinutes);
  display.print(buf);

  // divider
  display.drawFastHLine(0, 36, 128, WHITE);

  // break time pill
  display.drawRoundRect(14, 40, 50, 13, 6, WHITE);
  display.setTextSize(1);
  display.setCursor(18, 43);
  display.print("break");

  // break time number
  display.setTextSize(2);
  display.setCursor(10, 50);
  sprintf(buf, "%02d:00", breakMinutes);
  display.print(buf);

  // right side instructions
  display.setTextSize(1);
  display.setCursor(75, 10);
  display.print("SW2+");
  display.setCursor(75, 22);
  display.print("focus");
  display.setCursor(75, 46);
  display.print("SW4+");
  display.setCursor(75, 56);
  display.print("break");

  display.display();
}

// ══════════════════════════════════════════════════
// focus screen
void drawFocus(int remainSec)
{
  display.clearDisplay();

  int mins = remainSec / 60;
  int secs = remainSec % 60;

  // left: min
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(2, 20);
  char buf[3];
  sprintf(buf, "%02d", mins);
  display.print(buf);
  display.setTextSize(1);
  display.setCursor(6, 50);
  display.print("min");

  // right: sec
  display.setTextSize(3);
  display.setCursor(90, 20);
  sprintf(buf, "%02d", secs);
  display.print(buf);
  display.setTextSize(1);
  display.setCursor(94, 50);
  display.print("sec");

  // middle: tomato
  drawTomato(64, 30, 20, false, false, false);

  // bottom progress bar
  int totalSec = focusMinutes * 60;
  int progress = map(remainSec, totalSec, 0, 0, 108);
  display.drawRoundRect(10, 57, 108, 6, 3, WHITE);
  if (progress > 0)
    display.fillRoundRect(10, 57, progress, 6, 3, WHITE);

  display.display();
}

// ══════════════════════════════════════════════════
// paused screen
void drawPaused(int remainSec)
{
  display.clearDisplay();

  int mins = remainSec / 60;
  int secs = remainSec % 60;

  // number dimmed
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(2, 20);
  char buf[3];
  sprintf(buf, "%02d", mins);
  display.print(buf);
  display.setTextSize(1);
  display.setCursor(6, 50);
  display.print("min");

  display.setTextSize(3);
  display.setCursor(90, 20);
  sprintf(buf, "%02d", secs);
  display.print(buf);
  display.setTextSize(1);
  display.setCursor(94, 50);
  display.print("sec");

  // middle: tomato (paused eyes)
  drawTomato(64, 30, 20, false, true, false);

  // paused text blinking
  if (animFrame % 2 == 0)
  {
    display.setTextSize(1);
    display.setCursor(38, 57);
    display.print("- paused -");
  }

  display.display();
}

// ══════════════════════════════════════════════════
// break time screen
void drawBreak(int remainSec)
{
  display.clearDisplay();

  int mins = remainSec / 60;
  int secs = remainSec % 60;

  // left: min
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(2, 20);
  char buf[3];
  sprintf(buf, "%02d", mins);
  display.print(buf);
  display.setTextSize(1);
  display.setCursor(6, 50);
  display.print("min");

  // right: sec
  display.setTextSize(3);
  display.setCursor(90, 20);
  sprintf(buf, "%02d", secs);
  display.print(buf);
  display.setTextSize(1);
  display.setCursor(94, 50);
  display.print("sec");

  // middle: sunglass tomato
  drawTomato(64, 30, 20, true, false, false);

  // note animation (ASCII)
  display.setTextSize(1);
  if (animFrame % 3 == 0)
  {
    display.setCursor(42, 2);
    display.print("J  ");
  }
  else if (animFrame % 3 == 1)
  {
    display.setCursor(42, 2);
    display.print(" J ");
  }
  else
  {
    display.setCursor(42, 2);
    display.print("J J");
  }

  // bottom progress bar
  int totalSec = breakMinutes * 60;
  int progress = map(remainSec, totalSec, 0, 0, 108);
  display.drawRoundRect(10, 57, 108, 6, 3, WHITE);
  if (progress > 0)
    display.fillRoundRect(10, 57, progress, 6, 3, WHITE);

  display.display();
}

// ══════════════════════════════════════════════════
// done screen
void drawDone()
{
  display.clearDisplay();

  // star particle blinking
  if (animFrame % 2 == 0)
  {
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("*");
    display.setCursor(120, 0);
    display.print("*");
    display.setCursor(0, 56);
    display.print("*");
    display.setCursor(120, 56);
    display.print("*");
    display.setCursor(10, 28);
    display.print(".");
    display.setCursor(115, 28);
    display.print(".");
  }

  // middle: heart eyes tomato
  drawTomato(64, 28, 20, false, false, true);

  // text
  display.setTextSize(1);
  display.setCursor(22, 52);
  display.print("session done!");

  // session count
  display.setCursor(48, 0);
  for (int i = 0; i < min(sessionCount, 4); i++)
  {
    display.print("@ ");
  }

  display.display();
}