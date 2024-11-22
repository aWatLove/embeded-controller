#include <Arduino.h>
#include <EZButton.h>

// настройки для энкодера
#define CLK 18
#define DT 19
#define SW_ENCODER 21

int counterEncoder = 0;
int currentStateCLK;
int lastStateCLK;
String currentDir;
int bEncoderValue = 0;

void loopEncoder(); // обработчик энкдера


// настройка для джойстика
enum JoystickDirectionTypes
{
  CENTER,
  DOWN,
  UP,
  LEFT,
  RIGHT,
  DOWN_LEFT,
  DOWN_RIGHT,
  UP_LEFT,
  UP_RIGHT
};

enum JoystickValueTypes
{
  NONE,
  MIN,
  MAX
};

#define VRX_PIN  39 // ESP32 pin GPIO39 (ADC3) connected to VRX pin
#define VRY_PIN  36 // ESP32 pin GPIO36 (ADC0) connected to VRY pin
#define SW_PIN   23 // ESP32 pin GPIO17 connected to SW  pin

int valueX = 0; // to store the X-axis value
int valueY = 0; // to store the Y-axis value
int bValue = 0; // To store value of the button

int defaultValJoystick = 2000;
int thresholdJoystick = 500; // пороговое значение

String directionJoystickStr;
int directionJoystick = CENTER;

void loopJoystick(); // обработчик джойстика

// настройка для одиночной кнопки
#define ALONE_BUTTON_PIN 22 

int bAloneValue = 0;

// настройка кнопок
void ReadButtons(bool *states, int num);

EZButton _ezb(3, ReadButtons, 1000, 200, 15); // конфигурация кнопок (<кол-во>, func, <hold ms>, <press ms>, <debounce ms>)

void Btn1Pressed();
void Btn1Released();
void Btn2Pressed();
void Btn2Released();
void Btn3Pressed();
void Btn3Released();


void setup() {
  Serial.begin(115200);


  pinMode(SW_PIN, INPUT_PULLUP); // указываем вывод SW_PIN как вход и включаем подтягивающий резистор
  pinMode(ALONE_BUTTON_PIN, INPUT_PULLUP); // указываем вывод ALONE_BUTTON_PIN как вход и включаем подтягивающий резистор
  pinMode(LED_BUILTIN, OUTPUT); // указываем вывод LED_BUILTIN как выход

  pinMode(CLK, INPUT); // указываем вывод CLK как вход
  pinMode(DT, INPUT); // указываем вывод DT как вход
  pinMode(SW_ENCODER, INPUT_PULLUP); // Указываем вывод SW как вход и включаем подтягивающий резистор
  
  lastStateCLK = digitalRead(CLK);    // Считываем значение с CL

  // подписка на ивенты для кнопок (<индекс кнопки>, <функа обработки>, <ивент>)
  _ezb.Subscribe(0, Btn1Pressed, PRESSED);
  _ezb.Subscribe(0, Btn1Released, RELEASED);
  _ezb.Subscribe(1, Btn2Pressed, PRESSED);
  _ezb.Subscribe(1, Btn2Released, RELEASED);
  _ezb.Subscribe(2, Btn3Pressed, PRESSED);
  _ezb.Subscribe(2, Btn3Released, RELEASED);
  // Set the ADC attenuation to 11 dB (up to ~3.3V input)
  analogSetAttenuation(ADC_11db);
  _ezb.Loop();
}

void loop() {
  loopEncoder();
  
  // джойстик
  // read X and Y analog values
  loopJoystick();


  // print data to Serial Monitor on Arduino IDE
  Serial.print("js direction = ");
   Serial.print(directionJoystickStr);
   Serial.print(", val dir = ");
   Serial.print(directionJoystick);
  Serial.print(" : x = ");
  Serial.print(valueX);
  Serial.print(", y = ");
  Serial.print(valueY);
  Serial.print(" : button = ");
  Serial.print(bValue);
  Serial.print(" : button2 = ");
  Serial.print(bAloneValue);
  Serial.print(" : Direction: ");
  Serial.print(currentDir);
  Serial.print(" | Counter: ");
  Serial.print(counterEncoder);
  Serial.print(" : button3encoder = ");
  Serial.println(bEncoderValue);
  _ezb.Loop();
}


void loopEncoder() {
  // энкодер поворотный
  currentStateCLK = digitalRead(CLK); // Считываем значение с CLK
  
  // проверяем изменилось ли состояние CLK
  if (currentStateCLK != lastStateCLK && currentStateCLK == 1) {
    if (digitalRead(DT) != currentStateCLK) {
      counterEncoder ++;
      currentDir = "CCW";
    } else {
      counterEncoder --;
      currentDir = "CW";
    }
  }

  lastStateCLK = currentStateCLK; // Запопоследнее состояние CLK
}

// joystick
void loopJoystick() {
  valueX = analogRead(VRX_PIN);
  valueY = analogRead(VRY_PIN);

  if (valueX-thresholdJoystick > defaultValJoystick) {
    if (valueY-thresholdJoystick > defaultValJoystick) {
      directionJoystick = UP_RIGHT;
    } else if (valueY+thresholdJoystick < defaultValJoystick) {
      directionJoystick = DOWN_RIGHT;
    } else {
      directionJoystick = RIGHT;
    }
  } else if (valueX+thresholdJoystick < defaultValJoystick) {
    if (valueY-thresholdJoystick > defaultValJoystick) {
      directionJoystick = UP_LEFT;
    } else if (valueY+thresholdJoystick < defaultValJoystick) {
      directionJoystick = DOWN_LEFT;
    } else {
      directionJoystick = LEFT;
    }
  } else {
      if (valueY-thresholdJoystick > defaultValJoystick) {
      directionJoystick = UP;
    } else if (valueY+thresholdJoystick < defaultValJoystick) {
      directionJoystick = DOWN;
    } else {
       directionJoystick = CENTER;
    }
  }

  switch (directionJoystick)
  {
  case DOWN:
    directionJoystickStr = "D_";
    break;
  case UP:
    directionJoystickStr = "U_";
    break;
  case LEFT:
    directionJoystickStr = "_L";
    break;
  case RIGHT:
    directionJoystickStr = "_R";
    break;
  case DOWN_LEFT:
    directionJoystickStr = "DL";
    break;
  case DOWN_RIGHT:
    directionJoystickStr = "DR";
    break;
  case UP_LEFT:
    directionJoystickStr = "UL";
    break;
  case UP_RIGHT:
    directionJoystickStr = "UR";
    break;
  default:
    directionJoystickStr = "CC";
    break;
  }
}

//buttons
void ReadButtons(bool *states, int num) {
  states[0] = !digitalRead(SW_PIN);
  states[1] = !digitalRead(ALONE_BUTTON_PIN);
  states[2] = !digitalRead(SW_ENCODER);
}

void Btn1Pressed() {
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("The button is pressed");
  bValue = 1;
}

void Btn1Released() {
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("The button is released");
  bValue = 0;
}

void Btn2Pressed() {
  bAloneValue = 1;
}

void Btn2Released() {
  bAloneValue = 0;
}

void Btn3Pressed() {
  bEncoderValue = 1;
}

void Btn3Released() {
  bEncoderValue = 0;
}