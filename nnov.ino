#include <Nextion.h>
#include <AccelStepper.h>
#include <MultiStepper.h>

// INPUTS:
//limits
#define lim_x_min 38
#define lim_x_max 39
#define lim_z_min 40
#define lim_z_max 41
//touch probe sensors
#define probe1 37
//#define probe2 42
//encoder
//#define enc_A 22
//#define enc_B 23
//#define enc_Z 24
//phisical buttons
//#define emergency_real_btn 34
//#define start_real_btn 35

// OUTPUTS:
//step dir ena
#define x_step 30
#define x_dir 31
#define z_step 32
#define z_dir 33
#define ena1 25
#define ena2 24


//pipe rotate
#define pipe_cw 26
#define pipe_ccw 27
//cmd to start print
#define print_start 28
//alarms
//#define alarm_lamp 30
//#define alarm_buzzer 44
#define system_ready 22
#define enable_touch 29

//MOTORS:
AccelStepper x_drive(AccelStepper::DRIVER, x_step, x_dir);
AccelStepper z_drive(AccelStepper::DRIVER, z_step, z_dir);

//NEXTION UI ELEMENTS:
//JOG:
NexButton b_jog_x_min = NexButton(0, 4, "x_minus");
NexButton b_jog_x_plus = NexButton(0, 5, "x_plus");
NexButton b_jog_z_min = NexButton(0, 2, "z_minus");
NexButton b_jog_z_plus = NexButton(0, 3, "z_plus");
NexSlider n_jog_speed = NexSlider(0, 11, "speed");
NexButton b_pipe_cw = NexButton(0, 7, "pipe_cw");
NexButton b_pipe_ccw = NexButton(0, 6, "pipe_ccw");
NexText x_pos = NexText(0, 9, "x_pos");
NexText z_pos = NexText(0, 10, "z_pos");
//AUTO:
// NexSlider n_len = NexSlider(0, 29, "n_len");
NexSlider n_speed = NexSlider(1, 6, "speed");
// NexSlider n_count = NexSlider(0, 31, "n_count");
// NexSlider n_fromEage = NexSlider(0, 32, "n_fromEage");
// NexSlider n_between = NexSlider(0, 33, "n_between");
NexProgressBar progress = NexProgressBar(1, 5, "progress");
NexButton b_print = NexButton(1, 3, "start");
//MENU:
NexButton b_onOff = NexButton(0, 11, "onOff");
NexButton b_home = NexButton(0, 12, "home");
NexButton b_edge = NexButton(0, 14, "edge");
NexButton b_probe = NexButton(0, 13, "probe");
NexButton b_printMan = NexButton(0, 27, "manPrint");

NexPage main_page = NexPage(0, 0, "manPage");
NexPage wait_page = NexPage(3, 0, "waitPage");

NexTouch *nex_listen_list[] = 
{
  &b_jog_x_min,
  &b_jog_x_plus,
  &b_jog_z_min,
  &b_jog_z_plus,
  &b_pipe_cw,
  &b_pipe_ccw,
  &b_print,
  &b_onOff,
  &b_home,
  &b_edge,
  &b_probe,
  &b_printMan,
  NULL
};

uint32_t jog_speed;
float steps_per_mm_x = 21.73;//1 rev - 200 step, reduction 1:10, 180 mm/rev
float steps_per_mm_z = 65.2;//1 rev - 400 step, reduction 1:30, 200 mm/rev
float soft_lim_x_min = -1000000;//-2.0 * steps_per_mm_x;
float soft_lim_x_max = 1000000;// 500.0 * steps_per_mm_x;
float soft_lim_z_min = -1000000;//-500.0 * steps_per_mm_z;
float soft_lim_z_max = 1000000;//2.0 * steps_per_mm_z;
float speed_homing_x = 4.0;
float speed_homing_z = 4.0;
int jog_state = 0;
long probe_position = 0;
long edge_position = 0;
bool main_page_state = true;

int powerStatus = LOW;
int printStatus = LOW;
//FUNCTIONS PROTOTYPES:
//NEXTION UI callbacks
//jog
void jog_Popcallback(void *ptr) {
   jog_state = 0;
}
void jog_x_min_Pushcallback(void *ptr) {
  // n_jog_speed.getValue(&jog_speed);
  x_drive.setMaxSpeed(90 * steps_per_mm_x);
  x_drive.moveTo(soft_lim_x_min);
  jog_state = 1;
  jog();
}
void jog_x_plus_Pushcallback(void *ptr) {
  //  n_jog_speed.getValue(&jog_speed);
   x_drive.setMaxSpeed(90 * steps_per_mm_x);
   x_drive.moveTo(soft_lim_x_max);
   
   jog_state = 1;
   jog();
}
void jog_z_min_Pushcallback(void *ptr) {
  //  n_jog_speed.getValue(&jog_speed);
   z_drive.setMaxSpeed(30 * steps_per_mm_z);
   z_drive.moveTo(soft_lim_z_min);
   jog_state = 2;
   jog();
}
void jog_z_plus_Pushcallback(void *ptr) {
  //  n_jog_speed.getValue(&jog_speed);
   z_drive.setMaxSpeed(15 * steps_per_mm_z);
   z_drive.moveTo(soft_lim_z_max);
   jog_state = 2;
   jog();
}
void pipe_Popcallback(void *ptr) {
  digitalWrite(pipe_cw, LOW);
  digitalWrite(pipe_ccw, LOW);
}
void pipe_cw_Pushcallback(void *ptr) {
  digitalWrite(pipe_cw, HIGH);
}
void pipe_ccw_Pushcallback(void *ptr) {
  digitalWrite(pipe_ccw, HIGH);
}
void printMan_Popcallback(void *ptr) {
  if(printStatus == LOW) {
    digitalWrite(print_start, HIGH);
    printStatus = HIGH;
  } else {
    digitalWrite(print_start, LOW);
    printStatus = LOW;
  }
}
//auto
void print_Popcallback(void *ptr) {
  dbSerialPrintln("edge");
  digitalWrite(enable_touch, HIGH);
  wait_page.show();
  main_page_state = false;
  z_drive.setSpeed(60 * steps_per_mm_z);

  //rev pipe
  digitalWrite(pipe_cw, HIGH);
  delay(1000);
  digitalWrite(pipe_cw, LOW);

  //move to pipe
  int state = 0;
  z_drive.moveTo(soft_lim_z_min);
  while(state < 100) {
    if(digitalRead(probe1) == LOW) {
      state++;
    }
    else {
      state = 0;
    }
    z_drive.run();
  }
  z_drive.stop();
  z_drive.runToPosition();
  probe_position = z_drive.currentPosition();
  delay(500);
  x_drive.setSpeed(60*steps_per_mm_x);
  x_drive.moveTo(soft_lim_x_min);
  state = 0;
  while(state < 100) {
    if(digitalRead(probe1) == HIGH) {
      state++;
    }else {
      state=0;
    }
    x_drive.run();
  }
  x_drive.stop();
  x_drive.runToPosition();
  edge_position = x_drive.currentPosition();
  digitalWrite(enable_touch, LOW);
  x_drive.runToNewPosition(edge_position + 450 * steps_per_mm_x);
  digitalWrite(print_start, HIGH);
  x_drive.runToNewPosition(edge_position + 850 * steps_per_mm_x);
  z_drive.runToNewPosition(probe_position + 200 * steps_per_mm_z);
  x_drive.runToNewPosition(edge_position + 200 * steps_per_mm_x);
}
//menu
void onOff_Popcallback(void *ptr) {
  dbSerialPrint("onOff");
  if(powerStatus == LOW){
    digitalWrite(system_ready, HIGH);
    powerStatus = HIGH;
    digitalWrite(ena1, LOW);
    digitalWrite(ena2, LOW);
  } else {
    digitalWrite(system_ready, LOW);
    powerStatus = LOW;
    digitalWrite(ena1, HIGH);
    digitalWrite(ena2, HIGH);
  }
}
void home_Popcallback(void *ptr) {
  wait_page.show();
  main_page_state = false;
  x_drive.setMaxSpeed(speed_homing_x * steps_per_mm_x);
  z_drive.setMaxSpeed(speed_homing_z * steps_per_mm_z);

  z_drive.moveTo(10000000);
  int state = 0;
  while(state < 5){
    if(digitalRead(lim_z_max) == LOW) {
      state++;
    } else {
      state = 0;
    }
    z_drive.run();
  }
  z_drive.stop();
  z_drive.runToPosition();
  z_drive.runToNewPosition(z_drive.currentPosition() - 5 * steps_per_mm_z);
  z_drive.setCurrentPosition(0);

  x_drive.moveTo(-10000000);
  state = 0;
  while(state < 5) {
    if(digitalRead(lim_x_min) == LOW) {
      state++;
    } else {
      state = 0;
    }
    x_drive.run();
  }
  x_drive.stop();
  x_drive.runToPosition();
  z_drive.runToNewPosition(x_drive.currentPosition() - 5 * steps_per_mm_x);
  x_drive.setCurrentPosition(0);  
}
void edge_Popcallback(void *ptr) {
  dbSerialPrintln("edge");
  digitalWrite(enable_touch, HIGH);
  wait_page.show();
  main_page_state = false;
  z_drive.setSpeed(60 * steps_per_mm_z);

  //rev pipe
  digitalWrite(pipe_cw, HIGH);
  delay(1000);
  digitalWrite(pipe_cw, LOW);

  //move to pipe
  int state = 0;
  z_drive.moveTo(soft_lim_z_min);
  while(state < 100) {
    if(digitalRead(probe1) == LOW) {
      state++;
    }
    else {
      state = 0;
    }
    z_drive.run();
  }
  z_drive.stop();
  z_drive.runToPosition();
  probe_position = z_drive.currentPosition();
  delay(500);
  x_drive.setSpeed(60*steps_per_mm_x);
  x_drive.moveTo(soft_lim_x_min);
  state = 0;
  while(state < 100) {
    if(digitalRead(probe1) == HIGH) {
      state++;
    }else {
      state=0;
    }
    x_drive.run();
  }
  x_drive.stop();
  x_drive.runToPosition();
  edge_position = x_drive.currentPosition();
  digitalWrite(enable_touch, LOW);
  z_drive.runToNewPosition(probe_position + 200 * steps_per_mm_z);
}
void probe_Popcallback(void *ptr) {
  dbSerialPrintln("prode");
  digitalWrite(enable_touch, HIGH);
  wait_page.show();
  main_page_state = false;
  z_drive.setSpeed(60 * steps_per_mm_z);

  //rev pipe
  digitalWrite(pipe_cw, HIGH);
  delay(1000);
  digitalWrite(pipe_cw, LOW);

  //move to pipe
  int state = 0;
  z_drive.moveTo(soft_lim_z_min);
  while(state < 100) {
    if(digitalRead(probe1) == LOW) {
      state++;
    }
    else {
      state = 0;
    }
    z_drive.run();
  }
  z_drive.stop();
  z_drive.runToPosition();
  probe_position = z_drive.currentPosition();
  digitalWrite(enable_touch, LOW);
  z_drive.runToNewPosition(probe_position + 200 * steps_per_mm_z);
}

void jog() {
  while(true) {
    if(Serial2.available() > 0) {
      nexLoop(nex_listen_list);
    }
    switch(jog_state) {
      case 0:
        if(x_drive.isRunning()){
          x_drive.stop();
          x_drive.runToPosition();
        }
        if(z_drive.isRunning()){
          z_drive.stop();
          z_drive.runToPosition();
        }
        dbSerialPrintln(x_drive.currentPosition());
        dbSerialPrintln(z_drive.currentPosition());
        return;
      case 1:   //jog x
        x_drive.run();
        break;
      case 2:   //jog z
        z_drive.run();
        break;
    }
  }
}

void setup() {
  //init motion system
  x_drive.setMaxSpeed(2000);
  x_drive.setAcceleration(10000);
  z_drive.setMaxSpeed(1000);
  z_drive.setAcceleration(10000);
  
  //init HMI
  nexInit();
  b_jog_x_min.attachPush(jog_x_min_Pushcallback, &b_jog_x_min);
  b_jog_x_min.attachPop(jog_Popcallback, &b_jog_x_min);
  b_jog_x_plus.attachPush(jog_x_plus_Pushcallback, &b_jog_x_plus);
  b_jog_x_plus.attachPop(jog_Popcallback, &b_jog_x_plus);
  b_jog_z_min.attachPush(jog_z_min_Pushcallback, &b_jog_z_min);
  b_jog_z_min.attachPop(jog_Popcallback, &b_jog_z_min);
  b_jog_z_plus.attachPush(jog_z_plus_Pushcallback, &b_jog_z_plus);
  b_jog_z_plus.attachPop(jog_Popcallback, &b_jog_z_plus);
  b_pipe_cw.attachPush(pipe_cw_Pushcallback, &b_pipe_cw);
  b_pipe_cw.attachPop(pipe_Popcallback, &b_pipe_cw);
  b_pipe_ccw.attachPush(pipe_ccw_Pushcallback, &b_pipe_ccw);
  b_pipe_ccw.attachPop(pipe_Popcallback, &b_pipe_ccw);
  b_print.attachPop(print_Popcallback, &b_print);
  b_onOff.attachPop(onOff_Popcallback, &b_onOff);
  b_home.attachPop(home_Popcallback, &b_home);
  b_edge.attachPop(edge_Popcallback, &b_edge);
  b_probe.attachPop(probe_Popcallback, &b_probe);
  b_printMan.attachPop(printMan_Popcallback, &b_printMan);
  
  //init in/out
  pinMode(ena1, OUTPUT);
  pinMode(ena2, OUTPUT);
  pinMode(pipe_cw, OUTPUT);
  pinMode(pipe_ccw, OUTPUT);
  pinMode(print_start, OUTPUT);
  pinMode(system_ready, OUTPUT);
  pinMode(enable_touch, OUTPUT);
  

  pinMode(probe1, INPUT_PULLUP);
  pinMode(lim_x_min, INPUT_PULLUP);
  pinMode(lim_x_max, INPUT_PULLUP);
  pinMode(lim_z_min, INPUT_PULLUP);
  pinMode(lim_z_max, INPUT_PULLUP);
  digitalWrite(enable_touch, LOW);
}

void loop() {
  if(!main_page_state){
    main_page.show();
    main_page_state=true;
  }
  // dbSerialPrint(digitalRead(lim_x_min));
  // dbSerialPrint(digitalRead(lim_x_max));
  // dbSerialPrint(digitalRead(lim_z_min));
  // dbSerialPrint(digitalRead(lim_z_max));
  dbSerialPrintln(digitalRead(probe1));
  delay(200);

  if(Serial2.available() > 0) {
    nexLoop(nex_listen_list);
  }
}

