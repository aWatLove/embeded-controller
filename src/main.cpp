#include <Arduino.h>
#include <EZButton.h>

/*
 * This ESP32 code is created by esp32io.com
 *
 * This ESP32 code is released in the public domain
 *
 * For more detail (instruction and wiring diagram), visit https://esp32io.com/tutorials/esp32-joystick
 */

// настройки для энкодера
#define CLK 18
#define DT 19
#define SW_ENCODER 21

int counter = 0;
int currentStateCLK;
int lastStateCLK;
String currentDir;
int bEncoderValue = 0;


// настройка для джойстика
#define VRX_PIN  39 // ESP32 pin GPIO39 (ADC3) connected to VRX pin
#define VRY_PIN  36 // ESP32 pin GPIO36 (ADC0) connected to VRY pin
#define SW_PIN   23 // ESP32 pin GPIO17 connected to SW  pin

int valueX = 0; // to store the X-axis value
int valueY = 0; // to store the Y-axis value
int bValue = 0; // To store value of the button

// настройка для одиночной кнопки
#define ALONE_BUTTON_PIN 22 

int bAloneValue = 0;

// настройка кнопок
void ReadButtons(bool *states, int num) {
  //Read all button states however you want
  states[0] = !digitalRead(SW_PIN);
  states[1] = !digitalRead(ALONE_BUTTON_PIN);
  states[2] = !digitalRead(SW_ENCODER);
}

EZButton _ezb(3, ReadButtons, 1000, 200, 15);

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


void setup() {
  Serial.begin(115200);


  pinMode(SW_PIN, INPUT_PULLUP); // указываем вывод SW_PIN как вход и включаем подтягивающий резистор
  pinMode(ALONE_BUTTON_PIN, INPUT_PULLUP); // указываем вывод ALONE_BUTTON_PIN как вход и включаем подтягивающий резистор
  pinMode(LED_BUILTIN, OUTPUT); // указываем вывод LED_BUILTIN как выход

  pinMode(CLK, INPUT); // указываем вывод CLK как вход
  pinMode(DT, INPUT); // указываем вывод DT как вход
  pinMode(SW_ENCODER, INPUT_PULLUP); // Указываем вывод SW как вход и включаем подтягивающий резистор
  
  lastStateCLK = digitalRead(CLK);    // Считываем значение с CL

  //subscribe to needed events
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

  // энкодер поворотный
  currentStateCLK = digitalRead(CLK); // Считываем значение с CLK
  
  // проверяем изменилось ли состояние CLK
  if (currentStateCLK != lastStateCLK && currentStateCLK == 1) {
    if (digitalRead(DT) != currentStateCLK) {
      counter --;
      currentDir = "CCW";
    } else {
      counter ++;
      currentDir = "CW";
    }
  }

  lastStateCLK = currentStateCLK; // Запопоследнее состояние CLK
  
  // джойстик
  // read X and Y analog values
  valueX = analogRead(VRX_PIN);
  valueY = analogRead(VRY_PIN);


  // print data to Serial Monitor on Arduino IDE
  Serial.print("x = ");
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
  Serial.print(counter);
  Serial.print(" : button3encoder = ");
  Serial.println(bEncoderValue);
  _ezb.Loop();
}
