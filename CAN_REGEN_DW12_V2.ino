#include <SPI.h>      //Library for using SPI Communication
#include <mcp2515.h>  //Library for using CAN Communication

//pin connections
#define LED_GP 2   //Glow Plug
#define LED_FP 3   //Metering Pump
#define LED_0 4    //0% Bargraph
#define LED_20 5   //20% Bargraph
#define LED_40 6   //40% Bargraph
#define LED_60 7   //60% Bargraph
#define LED_80 8   //80% Bargraph
#define LED_100 9  //100% Bargraph

//LED ON States
#define LED_0_ON 0x01    //0% Bargraph
#define LED_20_ON 0x02   //20% Bargraph
#define LED_40_ON 0x04   //40% Bargraph
#define LED_60_ON 0x08   //60% Bargraph
#define LED_80_ON 0x10   //80% Bargraph
#define LED_100_ON 0x20  //100% Bargraph
#define LED_GP_ON 0x40   //Glow Plug
#define LED_FP_ON 0x80   //Metering Pump

//LED OFF States
#define LED_0_OFF 0xFE    //0% Bargraph
#define LED_20_OFF 0xFD   //20% Bargraph
#define LED_40_OFF 0xFB   //40% Bargraph
#define LED_60_OFF 0xF7   //60% Bargraph
#define LED_80_OFF 0xEF   //80% Bargraph
#define LED_100_OFF 0xDF  //100% Bargraph
#define LED_GP_OFF 0xBF   //Glow Plug
#define LED_FP_OFF 0x7F   //Metering Pump

//Variables etc..
struct can_frame canMsg;
struct can_frame frame;

MCP2515 mcp2515(10);  //CS Pin of CAN chip.
byte CANData[8];
boolean toggle1 = 0;
boolean LED_ON = 0;
char mybuffer[40];
int soot_load_OL = 0;
int soot_load_CL = 0;
int soot_load_total = 0;
int loop_counter = 0;
int fp_flash_const = 0;
byte LEDState;

//--------------------------------------------------------------------------------------
void setup() {
  //setup SPI comms
  SPI.begin();

  //setup CAN chip
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setFilterMask(0, 0, 0x00000700);
  mcp2515.setFilterMask(1, 0, 0x00000700);
  mcp2515.setFilter(0, 0, 0x000007FF);
  mcp2515.setNormalMode();

  //setup IO connections
  pinMode(LED_FP, OUTPUT);
  pinMode(LED_GP, OUTPUT);
  pinMode(LED_0, OUTPUT);
  pinMode(LED_20, OUTPUT);
  pinMode(LED_40, OUTPUT);
  pinMode(LED_60, OUTPUT);
  pinMode(LED_80, OUTPUT);
  pinMode(LED_100, OUTPUT);

  //Power ON LED test
  LED_Test();

  //set timer1
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 50;
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS11);  // | (1 << CS10); div by 8
  TIMSK1 |= (1 << OCIE1A);
  sei();

  //reset variables
  loop_counter = 0;
  fp_flash_const = 0;
  LED_ON = false;
}

//--------------------------------------------------------------------------------------
//Main program loop
void loop() {
  //read open loop soot calculation
  read_DID(0x057B);
  if ((frame.data[0] == 0x05) & (frame.data[1] == 0x62) & (frame.data[2] == 0x05)) {
    soot_load_OL = (frame.data[4] * 256) + frame.data[5];
    soot_load_OL = soot_load_OL / 20;
    soot_load_total = (soot_load_OL + soot_load_CL) / 2;
  } else {
    soot_load_total = 0;
    reset_can_driver();
  }

  //read closed loop soot calculation
  read_DID(0x0579);
  if ((frame.data[0] == 0x05) & (frame.data[1] == 0x62) & (frame.data[2] == 0x05)) {
    soot_load_CL = (frame.data[4] * 256) + frame.data[5];
    soot_load_CL = soot_load_CL / 20;
    soot_load_total = (soot_load_OL + soot_load_CL) / 2;
  } else {
    soot_load_total = 0;
    reset_can_driver();
  }
  //display calculated soot load on LEDs
  show_led(soot_load_total);

  //read vapouriser glow plug status
  read_DID(0x03B8);
  if ((frame.data[0] == 0x07) & (frame.data[1] == 0x62) & (frame.data[2] == 0x03) & (frame.data[3] == 0xB8)) {
    if ((frame.data[4] & 0x04) == 0x04) {
      LEDState = LEDState | LED_GP_ON;
    } else {
      LEDState = LEDState & LED_GP_OFF;
    }
  } else {
    LEDState = LEDState & LED_GP_OFF;
    reset_can_driver();
  }

  //read vapouriser metering pump status
  read_DID(0x041E);
  if ((frame.data[0] == 0x05) & (frame.data[1] == 0x62) & (frame.data[2] == 0x04) & (frame.data[3] == 0x1E)) {
    set_LED_FP_speed(frame.data[5]);
  } else {
    LEDState = LEDState & LED_FP_OFF;
    set_LED_FP_speed(0);
    reset_can_driver();
  }
  delay(500);
}


//--------------------------------------------------------------------------------------
//reset CAN driver chip - necessary if bus errors occur due to ignition off etc.
void reset_can_driver(void) {
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setFilterMask(0, 0, 0x00000700);
  mcp2515.setFilterMask(1, 0, 0x00000700);
  mcp2515.setFilter(0, 0, 0x000007FF);
  mcp2515.setNormalMode();
}

//--------------------------------------------------------------------------------------
//Read desired parameter over CAN
void read_DID(short DID_Number) {
  char mybuffer[40];

  canMsg.can_id = 0x7E0;
  canMsg.can_dlc = 8;
  canMsg.data[0] = 0x03;
  canMsg.data[1] = 0x22;
  canMsg.data[2] = (DID_Number & 0xFF00) / 255;
  canMsg.data[3] = DID_Number & 0x00FF;
  canMsg.data[4] = 0;
  canMsg.data[5] = 0;
  canMsg.data[6] = 0;
  canMsg.data[7] = 0;
  mcp2515.sendMessage(&canMsg);  //Sends the CAN message

  delay(100);

  if (mcp2515.readMessage(&frame) == MCP2515::ERROR_OK) {
    // frame contains received message - If serial setup then could monitor in terminal
    //sprintf (mybuffer, "%02X %02X %02X %02X %02X %02X %02X %02X", frame.data[0], frame.data[1],  frame.data[2],  frame.data[3],  frame.data[4],  frame.data[5], frame.data[6], frame.data[7]);
    //Serial.println(mybuffer);
  }
}

//--------------------------------------------------------------------------------------
//make FP LED flash at same rate as meting pump is being pulsed
void set_LED_FP_speed(int pump_speed) {
  if (pump_speed > 0) {
    LED_ON = true;
    fp_flash_const = 12500 / pump_speed;
  } else {
    LED_ON = false;
  }
  return;
}

//--------------------------------------------------------------------------------------
//set value to display based upon ranges below..
void show_led(int bar_value) {
  int base_offset = 0;  // 0 to 100%
  //int base_offset=20; // 20 to 120%

  //make 100% LED come on at 95%
  if (bar_value >= base_offset + 95) {
    LEDState = LEDState | LED_100_ON;
  } else {
    LEDState = LEDState & LED_100_OFF;
  }

  if (bar_value >= base_offset + 80) {
    LEDState = LEDState | LED_80_ON;
  } else {
    LEDState = LEDState & LED_80_OFF;
  }

  if (bar_value >= base_offset + 60) {
    LEDState = LEDState | LED_60_ON;
  } else {
    LEDState = LEDState & LED_60_OFF;
  }

  if (bar_value >= base_offset + 40) {
    LEDState = LEDState | LED_40_ON;
  } else {
    LEDState = LEDState & LED_40_OFF;
  }

  if (bar_value >= base_offset + 25) {
    LEDState = LEDState | LED_20_ON;
  } else {
    LEDState = LEDState & LED_20_OFF;
  }

  if (bar_value >= base_offset + 5) {
    LEDState = LEDState | LED_0_ON;
  } else {
    LEDState = LEDState & LED_0_OFF;
  }

  return;
}
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
ISR(TIMER1_COMPA_vect) {  //timer1 interrupt to flash pump LED and control other LEDs

  //general toggle for FP LED
  if (loop_counter < 1) {
    loop_counter = fp_flash_const;
    if (toggle1) {
      if (LED_ON == true) {
        LEDState = LEDState | LED_FP_ON;
      } else {
        LEDState = LEDState & LED_FP_OFF;
      }
      toggle1 = 0;
    } else {
      LEDState = LEDState & LED_FP_OFF;
      toggle1 = 1;
    }
  } else {
    loop_counter--;
  }

  if ((LEDState & LED_0_ON) > 0) {
    digitalWrite(LED_0, HIGH);
  } else {
    digitalWrite(LED_0, LOW);
  }
  if ((LEDState & LED_20_ON) > 0) {
    digitalWrite(LED_20, HIGH);
  } else {
    digitalWrite(LED_20, LOW);
  }
  if ((LEDState & LED_40_ON) > 0) {
    digitalWrite(LED_40, HIGH);
  } else {
    digitalWrite(LED_40, LOW);
  }
  if ((LEDState & LED_60_ON) > 0) {
    digitalWrite(LED_60, HIGH);
  } else {
    digitalWrite(LED_60, LOW);
  }
  if ((LEDState & LED_80_ON) > 0) {
    digitalWrite(LED_80, HIGH);
  } else {
    digitalWrite(LED_80, LOW);
  }
  if ((LEDState & LED_100_ON) > 0) {
    digitalWrite(LED_100, HIGH);
  } else {
    digitalWrite(LED_100, LOW);
  }
  if ((LEDState & LED_GP_ON) > 0) {
    digitalWrite(LED_GP, HIGH);
  } else {
    digitalWrite(LED_GP, LOW);
  }
  if ((LEDState & LED_FP_ON) > 0) {
    digitalWrite(LED_FP, HIGH);
  } else {
    digitalWrite(LED_FP, LOW);
  }
}
//--------------------------------------------------------------------------------------
//Power ON LED test - had to do it like this as interrupts/delay do not run during setup and
//did not want this in main loop
void LED_Test(void) {

  digitalWrite(LED_FP, HIGH);
  digitalWrite(LED_GP, HIGH);
  digitalWrite(LED_0, HIGH);
  digitalWrite(LED_20, HIGH);
  digitalWrite(LED_40, HIGH);
  digitalWrite(LED_60, HIGH);
  digitalWrite(LED_80, HIGH);
  digitalWrite(LED_100, HIGH);

  delay(300);
  digitalWrite(LED_FP, LOW);
  delay(100);
  digitalWrite(LED_GP, LOW);
  delay(100);
  digitalWrite(LED_100, LOW);
  delay(100);
  digitalWrite(LED_80, LOW);
  delay(100);
  digitalWrite(LED_60, LOW);
  delay(100);
  digitalWrite(LED_40, LOW);
  delay(100);
  digitalWrite(LED_20, LOW);
  delay(100);
  digitalWrite(LED_0, LOW);
  delay(200);
}

//--------------------------------------------------------------------------------------