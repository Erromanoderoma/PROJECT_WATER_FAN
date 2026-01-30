#include "arduino_secrets.h"
#include "thingProperties.h"
#include <IRremote.hpp>

// ==================== IR settings ====================
const int IR_SEND_PIN = 23;

const uint32_t CODE_ON_OFF = 0x807FE817;
const uint32_t CODE_SPEED = 0x807FF00F;  // step speed (cycles 1->2->3->1)
const uint32_t CODE_SWING = 0x807F6897;
const uint32_t CODE_COOL = 0x807F08F7;  // mapped to pump toggle
const uint32_t CODE_MODE = 0x807F28D7;
const uint32_t CODE_TIMER = 0x807F906F;
// =====================================================

#define OUTPUT_SPEED1 32
#define OUTPUT_SPEED2 33
#define OUTPUT_SPEED3 25

#define CHECH_PUMP 5
#define CHECH_SPEED1 18
#define CHECH_SPEED2 19
#define CHECH_SPEED3 21

/* ==================== STATE ==================== */
int spd1;
int spd2;
int spd3;
int pump;

int SPEED = 0;
int SET_speed = 0;

bool switch_power = false;
bool switch_water_pump = false;
String message;

int i_SET;
int i_miss_SET;


void setup() {

  Serial.begin(115200);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500);
  /* ===== Arduino IoT Cloud ===== */
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  pinMode(CHECH_PUMP, INPUT);
  pinMode(CHECH_SPEED1, INPUT_PULLUP);
  pinMode(CHECH_SPEED2, INPUT_PULLUP);
  pinMode(CHECH_SPEED3, INPUT_PULLUP);


  pinMode(OUTPUT_SPEED1, OUTPUT);
  pinMode(OUTPUT_SPEED2, OUTPUT);
  pinMode(OUTPUT_SPEED3, OUTPUT);

  IrSender.begin(IR_SEND_PIN);





  Serial.println("System Ready (IR + Cloud) V1.8");
}


void loop() {
  ArduinoCloud.update();  // ต้องอยู่ CPU0
  //debug();
  READPIN();
  OUTPUT_RELAY();
  SETSPEED();
}




void debug() {
  if (Serial.available()) {
    message = Serial.readString();
    SET_speed = message.toInt();
    Serial.print("SET Speed = ");
    Serial.println(SET_speed);
  }
}

void SETSPEED() {  /// ทำให้ SET_speed เป็น 0 เมื่อไม่ได้รับความสั่งในเวลาที่กำหนด เพื่อให้กดปุ่มน่าบอร์ดได้โดยไม่มีปัญหา

  if (SET_speed == SPEED) {
    i_SET++;
    if (i_SET >= 3) {
      i_SET = 0;
      SET_speed = 0;
    }
  } else if (i_miss_SET >= 5) {  // ถ้าส่ง IR พลาดเกินจำนวนจะหยุดส่ง
    i_miss_SET = 0;
    i_SET = 0;
    SET_speed = 0;
  }

  if (SET_speed != 0 && SPEED != SET_speed) {
    sendSpeedStep();
    i_miss_SET++;
    delay(300);
  }


  /* debug
  Serial.print("======================= ");
  Serial.println("           ");
  Serial.print("             SET_speed                ");
  Serial.println(SET_speed);
  Serial.print("====================== ");
  delay(500);
  */
}

void READPIN() {  // double check
  read_pin();
  delay(50);
  if (spd1 == 0 && spd2 == 1 && spd3 == 1) SPEED = 1;
  else if (spd1 == 1 && spd2 == 0 && spd3 == 1) SPEED = 2;
  else if (spd1 == 1 && spd2 == 1 && spd3 == 0) SPEED = 3;
  else {
    delay(300);
    read_pin();
    if (spd1 == 1 && spd2 == 1 && spd3 == 1) { SPEED = 0; }

  }  //ถ้า = 0 หน่วงเวลาแล้วเซ็คใหม่อีกรอบ กันพลาด
}

void read_pin() {
  delay(100);
  spd1 = digitalRead(CHECH_SPEED1);
  spd2 = digitalRead(CHECH_SPEED2);
  spd3 = digitalRead(CHECH_SPEED3);
/*
  Serial.print("spd1 = ");
  Serial.println(spd1);
  Serial.print("spd2 = ");
  Serial.println(spd2);
  Serial.print("spd3 = ");
  Serial.println(spd3);
  */
}


void OUTPUT_RELAY() {
  if (SPEED == 0) {
    digitalWrite(OUTPUT_SPEED1, LOW);
    digitalWrite(OUTPUT_SPEED2, LOW);
    digitalWrite(OUTPUT_SPEED3, LOW);
    Serial.print("OUTPUT = ");
    Serial.println("0");
  } else if (SPEED == 1) {
    digitalWrite(OUTPUT_SPEED1, HIGH);
    digitalWrite(OUTPUT_SPEED2, LOW);
    digitalWrite(OUTPUT_SPEED3, LOW);
    Serial.print("OUTPUT = ");
    Serial.println("1");
  } else if (SPEED == 2) {
    digitalWrite(OUTPUT_SPEED2, HIGH);
    digitalWrite(OUTPUT_SPEED1, LOW);
    digitalWrite(OUTPUT_SPEED3, LOW);
    Serial.print("OUTPUT = ");
    Serial.println("2");
  } else if (SPEED == 3) {
    digitalWrite(OUTPUT_SPEED3, HIGH);
    digitalWrite(OUTPUT_SPEED1, LOW);
    digitalWrite(OUTPUT_SPEED2, LOW);

    Serial.print("OUTPUT = ");
    Serial.println("3");
  }
  delay(100);
}


// ==================== IR send helper functions ====================
void sendPowerToggle() {
  Serial.print("[IR] SEND POWER (ON/OFF) 0x");
  Serial.println(CODE_ON_OFF, HEX);
  IrSender.sendNEC(CODE_ON_OFF, 32);
  delay(200);
}

void sendSpeedStep() {
  Serial.print("[IR] SEND SPEED STEP 0x");
  Serial.println(CODE_SPEED, HEX);
  IrSender.sendNEC(CODE_SPEED, 32);
  delay(200);
}

void sendPumpToggle() {
  Serial.print("[IR] SEND PUMP (mapped COOL) 0x");
  Serial.println(CODE_COOL, HEX);
  IrSender.sendNEC(CODE_COOL, 32);
  delay(200);
}

/* ==================== CLOUD CALLBACK ==================== */
void onSwitchChange() {
  Serial.print("[CLOUD] POWER = ");
  Serial.println(Switch);
  if (Switch == true && SPEED == 0) {
    sPEED1 = 0;
    sPEED2 = 0;
    sPEED3 = 0;
    SET_speed = 0;
    i_SET = 0;
    sendPowerToggle();

  } else if (Switch == false && SPEED != 0) {
    sPEED1 = 0;
    sPEED2 = 0;
    sPEED3 = 0;
    SET_speed = 0;
    i_SET = 0;
    sendPowerToggle();  // ยิง IR Power
  }
}


void onSPEED1Change() {
  sPEED2 = 0;
  sPEED3 = 0;
  if (sPEED1 == 1) {
    SET_speed = 1;
  }
}

void onSPEED2Change() {
  sPEED1 = 0;
  sPEED3 = 0;
  if (sPEED2 == 1) {
    SET_speed = 2;
  }
}

void onSPEED3Change() {
  sPEED1 = 0;
  sPEED2 = 0;
  if (sPEED3 == 1) {
    SET_speed = 3;
  }
}

void onWATERChange() {
  switch_water_pump = WATER;
  sendPumpToggle();
}