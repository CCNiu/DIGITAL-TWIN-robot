#include "Emakefun_MotorDriver.h"
#include "PS2X_lib.h"  //for v1.6

#define PS2_DAT     12
#define PS2_CMD     11
#define PS2_SEL     10
#define PS2_CLK     13

const uint8_t SPEED_NORMAL = 150;
const uint8_t SPEED_TURN   = 100;
const int buzzerPin = A0;
// #define pressures   true
#define pressures   false
// #define rumble      true
#define rumble      false

PS2X ps2x; // create PS2 Controller Class

Emakefun_MotorDriver mMotor = Emakefun_MotorDriver(0x60);
Emakefun_DCMotor *DCMotor_1 = mMotor.getMotor(M1);
Emakefun_DCMotor *DCMotor_2 = mMotor.getMotor(M2);
Emakefun_DCMotor *DCMotor_3 = mMotor.getMotor(M3);
Emakefun_DCMotor *DCMotor_4 = mMotor.getMotor(M4);

Emakefun_Servo *mServo1, *mServo2, *mServo3, *mServo4;
int error = 0;
byte vibrate = 0;
void (* resetFunc) (void) = 0;
uint8_t ServoBaseDegree;
int16_t frontOffset = 0;        // 目前偏移量（相對於基準 90°）
int16_t backOffset = 0;        // 目前偏移量（相對於基準 90°）
const uint8_t Step = 1;    // 每次調整幅度
const uint8_t SERVO_MIN = 10;   // 最小角度
const uint8_t SERVO_MAX = 170;  // 最大角度

void beep() {
  tone(buzzerPin, 2000, 100);  // 2000Hz，持續 100ms
  delay(120);                  // 留點時間讓聲響完整
  noTone(buzzerPin);
}

void SetServoBaseDegree(uint8_t base)
{
  ServoBaseDegree = base;
  mServo1->writeServo(ServoBaseDegree);
  mServo2->writeServo(180 - ServoBaseDegree);
  // 後側
  mServo3->writeServo(ServoBaseDegree);
  mServo4->writeServo(180 - ServoBaseDegree);
}

void InitServo(void){
  mServo1 = mMotor.getServo(1);
  mServo2 = mMotor.getServo(2);
  mServo3 = mMotor.getServo(3);
  mServo4 = mMotor.getServo(4);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  mMotor.begin(50);   /*初始化io口的輸出頻率為50Hz*/
  InitServo();
  SetServoBaseDegree(90);
  error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble);
  if(error == 0){
    Serial.print("Found Controller, configured successful ");
  }
  else if(error == 1)
    Serial.println("No controller found, check wiring, see readme.txt to enable debug. visit www.billporter.info for troubleshooting tips");
   
  else if(error == 2)
    Serial.println("Controller found but not accepting commands. see readme.txt to enable debug. Visit www.billporter.info for troubleshooting tips");

  else if(error == 3)
    Serial.println("Controller refusing to enter Pressures mode, may not support it. ");
  
}


void loop() {
  // put your main code here, to run repeatedly:
  // 前进
  if(error == 1){ //skip loop if no controller found
    resetFunc();
  }
  bool moved = false;
  ps2x.read_gamepad(false, vibrate);
  //f b l r 
  if(ps2x.Button(PSB_PAD_UP)){
    Serial.println("forward");
    goForward();
    moved = true;
  }
  if(ps2x.Button(PSB_PAD_RIGHT)){
    Serial.println("goright");
    goRight();
    moved = true;
  }

  if(ps2x.Button(PSB_PAD_LEFT)){
    Serial.println("goleft");
    goLeft();
    moved = true;
  }
  if(ps2x.Button(PSB_PAD_DOWN)){
    Serial.println("goback");
    goBack();
    moved = true;
  }
  // f_servo
  if(ps2x.Button(PSB_L1)){
    Serial.println("front_up");
    front_up();

  }
  if(ps2x.Button(PSB_L2)){
    Serial.println("front_down");
    front_down();
  }
  // b_servo
  if(ps2x.Button(PSB_R1)){
    Serial.println("back_up");
    back_up();
  }
  if(ps2x.Button(PSB_R2)){
    Serial.println("back_down");
    back_down();
  }
  //turn_around
  if(ps2x.Button(PSB_SQUARE)){
    Serial.println("turnLeft");
    turnLeft();
    moved = true;
  }
  if(ps2x.Button(PSB_CIRCLE)){
    Serial.println("turnRight");
    turnRight();
    moved = true;
  }
  // beep();
  if (!moved) {
    KeepStop();
  }
  delay(10);
}
void KeepStop(void)
{
  DCMotor_1->setSpeed(0);
  DCMotor_2->setSpeed(0);
  DCMotor_3->setSpeed(0);
  DCMotor_4->setSpeed(0);
  DCMotor_1->run(BRAKE);
  DCMotor_2->run(BRAKE);
  DCMotor_3->run(BRAKE);
  DCMotor_4->run(BRAKE);
}

// 往前
void goForward() {
  DCMotor_1->setSpeed(SPEED_NORMAL); DCMotor_1->run(FORWARD);   // 左前
  DCMotor_2->setSpeed(SPEED_NORMAL); DCMotor_2->run(FORWARD);   // 右前
  DCMotor_3->setSpeed(SPEED_NORMAL); DCMotor_3->run(FORWARD);   // 左後
  DCMotor_4->setSpeed(SPEED_NORMAL); DCMotor_4->run(FORWARD);   // 右後
}

// 往後
void goBack() {
  DCMotor_1->setSpeed(SPEED_NORMAL); DCMotor_1->run(BACKWARD);
  DCMotor_2->setSpeed(SPEED_NORMAL); DCMotor_2->run(BACKWARD);
  DCMotor_3->setSpeed(SPEED_NORMAL); DCMotor_3->run(BACKWARD);
  DCMotor_4->setSpeed(SPEED_NORMAL); DCMotor_4->run(BACKWARD);
}

// 向左平移（左邊滑行）
void goLeft() {
  // 麥克納姆輪平移邏輯：左右對角輪相反方向
  DCMotor_1->setSpeed(SPEED_NORMAL); DCMotor_1->run(BACKWARD);  // 左前 往後
  DCMotor_2->setSpeed(SPEED_NORMAL); DCMotor_2->run(FORWARD);   // 右前 往前
  DCMotor_3->setSpeed(SPEED_NORMAL); DCMotor_3->run(FORWARD);   // 左後 往前
  DCMotor_4->setSpeed(SPEED_NORMAL); DCMotor_4->run(BACKWARD);  // 右後 往後
}

// 向右平移（右邊滑行）
void goRight() {
  DCMotor_1->setSpeed(SPEED_NORMAL); DCMotor_1->run(FORWARD);
  DCMotor_2->setSpeed(SPEED_NORMAL); DCMotor_2->run(BACKWARD);
  DCMotor_3->setSpeed(SPEED_NORMAL); DCMotor_3->run(BACKWARD);
  DCMotor_4->setSpeed(SPEED_NORMAL); DCMotor_4->run(FORWARD);
}

// 原地左轉（逆時針旋轉）
void turnLeft() {
  DCMotor_1->setSpeed(SPEED_TURN);   DCMotor_1->run(BACKWARD);
  DCMotor_2->setSpeed(SPEED_TURN);   DCMotor_2->run(FORWARD);
  DCMotor_3->setSpeed(SPEED_TURN);   DCMotor_3->run(BACKWARD);
  DCMotor_4->setSpeed(SPEED_TURN);   DCMotor_4->run(FORWARD);
}

// 原地右轉（順時針旋轉）
void turnRight() {
  DCMotor_1->setSpeed(SPEED_TURN);   DCMotor_1->run(FORWARD);
  DCMotor_2->setSpeed(SPEED_TURN);   DCMotor_2->run(BACKWARD);
  DCMotor_3->setSpeed(SPEED_TURN);   DCMotor_3->run(FORWARD);
  DCMotor_4->setSpeed(SPEED_TURN);   DCMotor_4->run(BACKWARD);
}

// 把共用的計算抽成一個函式
void writeFrontServo() {
  // ServoBaseDegree + frontOffset → 真實輸出角度
  int16_t raw = ServoBaseDegree + frontOffset;
  int16_t angle1 = constrain(raw, SERVO_MIN, SERVO_MAX);
  int16_t angle2 = 180 - angle1;  
  mServo1->writeServo(angle1);
  mServo2->writeServo(angle2);
}

void front_up() {
  int16_t maxOffset = SERVO_MAX - ServoBaseDegree;  
  // offset 往負方向，最小到 -maxOffset
  frontOffset = max(frontOffset - Step, -maxOffset);
  writeFrontServo();
}

void front_down() {
  int16_t maxOffset = SERVO_MAX - ServoBaseDegree;
  // offset 往正方向，最大到 +maxOffset
  frontOffset = min(frontOffset + Step, maxOffset);
  writeFrontServo();
}

void writeBackServo() {
  int16_t raw = ServoBaseDegree + backOffset;
  int16_t angle3 = constrain(raw, SERVO_MIN, SERVO_MAX);
  int16_t angle4 = 180 - angle3;  
  mServo3->writeServo(angle3);
  mServo4->writeServo(angle4);
}

void back_down() {
  int16_t maxOffset = SERVO_MAX - ServoBaseDegree;
  // 這裡「up」可以選正或負，看你要哪邊是 up
  backOffset = max(backOffset - Step, -maxOffset);
  writeBackServo();
}

void back_up() {
  int16_t maxOffset = SERVO_MAX - ServoBaseDegree;
  backOffset = min(backOffset + Step, maxOffset);
  writeBackServo();
}
