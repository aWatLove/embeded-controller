#include <Arduino.h>
#include <EZButton.h>
#include "painlessMesh.h"

// настройка mesh сети
#define MESH_PREFIX "picturka"
#define MESH_PASSWORD "picturka"
#define MESH_PORT 5555

// метод-заглушка
void sendMessage() ; // чтобы PlatformIO работал

Scheduler userScheduler; // для управления задачами
Task taskSendMessage (TASK_SECOND * 0.2, TASK_FOREVER, & sendMessage);
painlessMesh  mesh;


String formattedString; // для сообщения


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

// Кнопки
#define BUTTON_PIN_1 26
#define BUTTON_PIN_2 25
#define BUTTON_PIN_3 32
#define BUTTON_PIN_4 33

int buttnValue1 = 0;
int buttnValue2 = 0;
int buttnValue3 = 0;
int buttnValue4 = 0;

// настройка кнопок
void ReadButtons(bool *states, int num);

EZButton _ezb(7, ReadButtons, 1000, 200, 15); // конфигурация кнопок (<кол-во>, func, <hold ms>, <press ms>, <debounce ms>)

void Btn1Pressed();
void Btn1Released();
void Btn2Pressed();
void Btn2Released();
void Btn3Pressed();
void Btn3Released();

void Button1_Pressed();
void Button1_Released();
void Button2_Pressed();
void Button2_Released();
void Button3_Pressed();
void Button3_Released();
void Button4_Pressed();
void Button4_Released();


// Требуется для painlessMesh
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}
void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}
void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}
void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

void setup() {
  Serial.begin(115200);


  pinMode(SW_PIN, INPUT_PULLUP); // указываем вывод SW_PIN как вход и включаем подтягивающий резистор
  pinMode(ALONE_BUTTON_PIN, INPUT_PULLUP); // указываем вывод ALONE_BUTTON_PIN как вход и включаем подтягивающий резистор
  pinMode(LED_BUILTIN, OUTPUT); // указываем вывод LED_BUILTIN как выход

  // кнопки
  pinMode(BUTTON_PIN_1, INPUT_PULLUP); // указываем вывод ALONE_BUTTON_PIN как вход и включаем подтягивающий резистор
  pinMode(BUTTON_PIN_2, INPUT_PULLUP); // указываем вывод ALONE_BUTTON_PIN как вход и включаем подтягивающий резистор
  pinMode(BUTTON_PIN_3, INPUT_PULLUP); // указываем вывод ALONE_BUTTON_PIN как вход и включаем подтягивающий резистор
  pinMode(BUTTON_PIN_4, INPUT_PULLUP); // указываем вывод ALONE_BUTTON_PIN как вход и включаем подтягивающий резистор


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

  // кнопки
  _ezb.Subscribe(3, Button1_Pressed, PRESSED);
  _ezb.Subscribe(3, Button1_Released, RELEASED);
  _ezb.Subscribe(4, Button2_Pressed, PRESSED);
  _ezb.Subscribe(4, Button2_Released, RELEASED);
  _ezb.Subscribe(5, Button3_Pressed, PRESSED);
  _ezb.Subscribe(5, Button3_Released, RELEASED);
  _ezb.Subscribe(6, Button4_Pressed, PRESSED);
  _ezb.Subscribe(6, Button4_Released, RELEASED);
  // Set the ADC attenuation to 11 dB (up to ~3.3V input)
  analogSetAttenuation(ADC_11db);
  _ezb.Loop();

  // MESH 
  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // выбираем типы
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // установите перед функцией init() чтобы выдавались приветственные сообщения
 
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
 
  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();
}

void loop() {
mesh.update ();

  loopEncoder();
  
  // джойстик
  // read X and Y analog values
  loopJoystick();

    char buffer[50]; // Буфер для `sprintf`

    // Форматируем строку с помощью sprintf
    sprintf(buffer, "%d%d%d%d%d%d%d%d%d", directionJoystick, bValue, bAloneValue, counterEncoder, bEncoderValue, buttnValue1, buttnValue2, buttnValue3, buttnValue4);

    // Преобразуем буфер в String
    formattedString = String(buffer);

    // Выводим результат
    Serial.println(formattedString);


  // // print data to Serial Monitor on Arduino IDE
  // Serial.print("js direction = ");
  // Serial.print(directionJoystickStr);
  // Serial.print(", val dir = ");
  // Serial.print(directionJoystick);
  // Serial.print(" : x = ");
  // Serial.print(valueX);
  // Serial.print(", y = ");
  // Serial.print(valueY);
  // Serial.print(" : button = ");
  // Serial.print(bValue);
  // Serial.print(" : button2 = ");
  // Serial.print(bAloneValue);
  // Serial.print(" : Direction: ");
  // Serial.print(currentDir);
  // Serial.print(" | Counter: ");
  // Serial.print(counterEncoder);
  // Serial.print(" : button3encoder = ");
  // Serial.print(bEncoderValue);
  // Serial.print(":");
  // Serial.print(buttnValue1);
  // Serial.print(":");
  // Serial.print(buttnValue2);
  // Serial.print(":");
  // Serial.print(buttnValue3);
  // Serial.print(":");
  // Serial.println(buttnValue4);

  _ezb.Loop();
}

// mesh send message
void sendMessage() {
  String msg = "Hi from node1";
  
  msg += mesh.getNodeId();

  mesh.sendBroadcast( formattedString ); // отправка сообщения
}

void loopEncoder() {
  // энкодер поворотный
  currentStateCLK = digitalRead(CLK); // Считываем значение с CLK
  
  // проверяем изменилось ли состояние CLK
  if (currentStateCLK != lastStateCLK && currentStateCLK == 1) {
    if (digitalRead(DT) != currentStateCLK) {
      if ( counterEncoder < 9 ) {
        counterEncoder ++;
        currentDir = "CCW";
      }
    } else {
      if ( counterEncoder > 0 ) {
        counterEncoder --;
        currentDir = "CW";
      }
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
  states[3] = !digitalRead(BUTTON_PIN_1);
  states[4] = !digitalRead(BUTTON_PIN_2);
  states[5] = !digitalRead(BUTTON_PIN_3);
  states[6] = !digitalRead(BUTTON_PIN_4);
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

// кнопки
void Button1_Pressed() {
  buttnValue1 = 1;
}

void Button1_Released() {
  buttnValue1 = 0;
}

void Button2_Pressed() {
  buttnValue2 = 1;
}

void Button2_Released() {
  buttnValue2 = 0;
}

void Button3_Pressed() {
  buttnValue3 = 1;
}

void Button3_Released() {
  buttnValue3 = 0;
}

void Button4_Pressed() {
  buttnValue4 = 1;
}

void Button4_Released() {
  buttnValue4 = 0;
}


