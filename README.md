# PROJECT_WATER_FAN

ภาพรวม: โค้ดนี้คือระบบควบคุม พัดลมระบบน้ำผ่าน IR + Arduino IoT Cloud โดย ESP32 ทำ 3 อย่างพร้อมกัน:

อ่านสถานะสปีดจากฮาร์ดแวร์จริง

ส่ง IR ไปสั่งพัดลม

รับคำสั่งจาก Cloud

ด้านล่างคือคำอธิบายทีละส่วนตามโครงสร้างโปรแกรม

1. ส่วน library และ IR code
#include "arduino_secrets.h"
#include "thingProperties.h"
#include <IRremote.hpp>


arduino_secrets.h → WiFi / Cloud credential

thingProperties.h → ตัวแปร IoT Cloud

IRremote → ไลบรารีส่งรีโมตอินฟราเรด

CODE_ON_OFF
CODE_SPEED
CODE_COOL
...


คือรหัส IR ของรีโมตจริง
CODE_SPEED เป็นปุ่ม step speed → 1→2→3→1

2. กำหนดขา I/O
ขา output (ไปรีเลย์)
OUTPUT_SPEED1 32
OUTPUT_SPEED2 33
OUTPUT_SPEED3 25


ใช้ควบคุมรีเลย์ให้ตรงกับสปีดจริง

ขา input (อ่านสถานะพัดลม)
CHECH_SPEED1 18
CHECH_SPEED2 19
CHECH_SPEED3 21


อ่านว่าพัดลมตอนนี้อยู่สปีดไหน
เป็น feedback จากเครื่องจริง

3. ตัวแปรสถานะหลัก
int SPEED
int SET_speed


SPEED = สปีดจริงที่อ่านได้จากเครื่อง

SET_speed = สปีดที่ผู้ใช้สั่ง

ระบบจะยิง IR จนกว่า:

SPEED == SET_speed


แล้วหยุด

4. setup()

ทำ 4 อย่างหลัก:

เริ่ม Serial

เชื่อม Arduino IoT Cloud

ตั้ง pinMode

เริ่ม IR sender

IrSender.begin(IR_SEND_PIN);


กำหนด GPIO 23 เป็น IR transmitter

5. loop()
ArduinoCloud.update();
READPIN();
OUTPUT_RELAY();
SETSPEED();


ลูปหลักทำงานเป็น cycle:

5.1 READPIN()

อ่านสถานะสปีดจริงจากฮาร์ดแวร์

if (spd1 == 0 && spd2 == 1 && spd3 == 1) SPEED = 1;


mapping pin → speed

ถ้าอ่านไม่ได้ชัด → หน่วงแล้วอ่านซ้ำกัน error

5.2 OUTPUT_RELAY()

สะท้อนสถานะ SPEED ไปยังรีเลย์

เช่น:

SPEED = 2
→ เปิดรีเลย์สปีด 2


ใช้เป็น indicator หรือควบคุมวงจรจริง

5.3 SETSPEED()

ส่วนสำคัญที่สุดของระบบ

ตรรกะ:

ถ้า SET_speed != SPEED
→ ยิง IR speed step

sendSpeedStep();


ยิงทีละ step จนกว่าจะตรง

ตัวกันยิงไม่หยุด
i_miss_SET


ถ้ายิงเกิน 5 ครั้งแล้วยังไม่ตรง:

reset SET_speed = 0


ป้องกัน loop ยิง IR ไม่จบ

auto reset คำสั่ง
if (SET_speed == SPEED)
→ นับ i_SET
→ ครบ 3 รอบ reset SET_speed


เพื่อให้ปุ่มหน้าเครื่องกลับมาคุมได้

6. ฟังก์ชัน IR
sendSpeedStep()
sendPowerToggle()
sendPumpToggle()


ทั้งหมดใช้:

IrSender.sendNEC(code, 32);


32 = bit length ของโปรโตคอล NEC

7. Cloud callback

ส่วนนี้รับคำสั่งจาก Arduino IoT Dashboard

onSPEED1Change()
SET_speed = 1;


คือสั่งให้ระบบเริ่มยิง IR จนไปถึงสปีด 1

onSwitchChange()

สั่งเปิด/ปิดเครื่อง:

sendPowerToggle();

Flow การทำงานจริง

ผู้ใช้กด Cloud → SET_speed = 3

loop ทำงาน:

อ่านสปีดจริง = 1
ยิง IR → 2
ยิง IR → 3
หยุด


จากนั้น reset SET_speed

ระบบกลับสู่โหมด manual
