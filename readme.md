# 🍅 NU40 DK Pomodoro Timer

> A cute pomodoro timer built with NUCODE NU40 DK (nRF52840) + OLED + Active Buzzer

---

## 📸 Overview | 개요

A minimal pomodoro timer that runs on the NU40 DK board with a 128x64 OLED display and an active buzzer. Features an animated tomato face that changes expression depending on the current state.

NU40 DK 보드와 128x64 OLED 디스플레이, 능동 부저로 동작하는 미니멀한 뽀모도로 타이머입니다. 현재 상태에 따라 표정이 바뀌는 귀여운 토마토 얼굴이 특징입니다.

---

## 🛒 Hardware | 하드웨어

| Component | Spec | Note |
|-----------|------|------|
| MCU Board | NUCODE NU40 DK (nRF52840) | |
| Display | SSD1306 OLED 128x64 | I2C |
| Buzzer | Active Buzzer Module | 3-pin (S/+/-) |

---

## 🔌 Wiring | 배선

### OLED
| OLED Pin | NU40 DK Pin |
|----------|-------------|
| VCC | 3.3V |
| GND | GND |
| SDA | P0.07 |
| SCL | P0.08 |

### Active Buzzer | 능동 부저
| Buzzer Pin | NU40 DK Pin |
|------------|-------------|
| S (Signal) | P0.26 |
| + | 3.3V |
| - | GND |

---

## 🎮 Button Controls | 버튼 조작

| Button | Pin | Function |
|--------|-----|----------|
| SW1 | P0.11 | Start / Pause |
| SW2 | P0.12 | Focus time +1 min (idle screen only) |
| SW4 | P0.25 | Break time +1 min (idle screen only) |
| SW1 (2s long press) | P0.11 | Reset |

---

## 📺 Screens | 화면 구성

| Screen | Description |
|--------|-------------|
| **Idle** | Set focus / break time. SW2 and SW4 to adjust. |
| **Focus** | Countdown with tomato face + progress bar |
| **Paused** | Paused eyes tomato + blinking "paused" text |
| **Break** | Sunglasses tomato + note animation |
| **Done** | Heart eyes tomato + star particles |

| 화면 | 설명 |
|------|------|
| **초기** | 집중/쉬는 시간 설정. SW2, SW4로 조절 |
| **집중** | 토마토 얼굴 + 프로그레스 바 카운트다운 |
| **일시정지** | 일시정지 눈 토마토 + "paused" 깜빡임 |
| **쉬는 시간** | 선글라스 토마토 + 음표 애니메이션 |
| **완료** | 하트 눈 토마토 + 별 파티클 |

---

## ⚙️ Installation | 설치

### Requirements | 요구사항
- Arduino IDE
- NUCODE nRF52840 BSP
  ```
  https://raw.githubusercontent.com/Nucode01/Adafruit_nRF52_Arduino/refs/heads/master/package_nuduino_index.json
  ```
- Libraries | 라이브러리
  - `Adafruit SSD1306`
  - `Adafruit GFX`

### Steps | 설치 순서
1. Add NUCODE BSP URL to Arduino IDE → Preferences → Additional Board Manager URLs
2. Install board: Tools → Board Manager → search `nucode`
3. Select board: `NU40 DK`
4. Install libraries via Library Manager
5. Open `nu40_pomodoro.ino` and upload

---

## 📁 File Structure | 파일 구조

```
nu40_pomodoro/
└── nu40_pomodoro.ino   # Main sketch
```

---

*Built with ❤️ on NU40 DK (nRF52840)*