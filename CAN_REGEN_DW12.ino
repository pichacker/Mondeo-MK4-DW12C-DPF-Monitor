#include <SPI.h>          //Library for using SPI Communication 
#include <mcp2515.h>      //Library for using CAN Communication

#define GP_LED 2    //Glow Plug
#define PUMP_LED 3  //Metering Pump
#define LED_0 4     //0% Bargraph
#define LED_20 5    //20% Bargraph
#define LED_40 6    //40% Bargraph
#define LED_60 7    //60% Bargraph
#define LED_80 8    //80% Bargraph
#define LED_100 9   //100% Bargraph

struct can_frame canMsg;
struct can_frame frame;

MCP2515 mcp2515(10); //CS Pin of CAN chip.
byte CANData[8];
boolean toggle1 = 0;
boolean LED_ON = 0;
char mybuffer[40];
int soot_load_OL = 0;
int soot_load_CL = 0;
int soot_load_total = 0;
int loop_counter = 0;
int fp_flash_const = 0;


//--------------------------------------------------------------------------------------
void setup() {
  //while (!Serial);
  //Serial.begin(9600);
  SPI.begin();

  pinMode(PUMP_LED, OUTPUT);
  pinMode(GP_LED, OUTPUT);
  pinMode(LED_0, OUTPUT);
  pinMode(LED_20, OUTPUT);
  pinMode(LED_40, OUTPUT);
  pinMode(LED_60, OUTPUT);
  pinMode(LED_80, OUTPUT);
  pinMode(LED_100, OUTPUT);

  digitalWrite(PUMP_LED, HIGH);
  digitalWrite(GP_LED, HIGH);
  digitalWrite(LED_0, HIGH);
  digitalWrite(LED_20, HIGH);
  digitalWrite(LED_40, HIGH);
  digitalWrite(LED_60, HIGH);
  digitalWrite(LED_80, HIGH);
  digitalWrite(LED_100, HIGH);

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setFilterMask(0, 0, 0x00000700);
  mcp2515.setFilterMask(1, 0, 0x00000700);
  mcp2515.setFilter(0, 0, 0x000007FF);
  mcp2515.setNormalMode();

  delay(500);
  digitalWrite(PUMP_LED, LOW);  delay(100);
  digitalWrite(GP_LED, LOW);  delay(100);
  digitalWrite(LED_100, LOW);  delay(100);
  digitalWrite(LED_80, LOW);  delay(100);
  digitalWrite(LED_60, LOW);  delay(100);
  digitalWrite(LED_40, LOW);  delay(100);
  digitalWrite(LED_20, LOW);  delay(100);
  digitalWrite(LED_0, LOW);  delay(300);

  //set timer1
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A = 50;
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS11); // | (1 << CS10); div by 8
  TIMSK1 |= (1 << OCIE1A);
  sei();

  loop_counter = 0;
  fp_flash_const = 0;
  LED_ON = false;
  digitalWrite(PUMP_LED, LOW);

}

void loop()
{
  read_DID(0x057B);
  if ((frame.data[0] == 0x05) & (frame.data[1] == 0x62) & (frame.data[2] == 0x05))
  {
    soot_load_OL = (frame.data[4] * 256) + frame.data[5];
    soot_load_OL = soot_load_OL / 20;
    //sprintf (mybuffer, "OL Soot loading is %d%%", soot_load_OL);
    //Serial.println(mybuffer);
    soot_load_total = (soot_load_OL + soot_load_CL ) / 2;
  }
  else
  {
    soot_load_total = 0;
    //Serial.println("Unable to read OL soot loading DID");
    reset_can_driver();
  }

  read_DID(0x0579);
  if ((frame.data[0] == 0x05) & (frame.data[1] == 0x62) & (frame.data[2] == 0x05))
  {
    soot_load_CL = (frame.data[4] * 256) + frame.data[5];
    soot_load_CL = soot_load_CL / 20;
    //sprintf (mybuffer, "CL Soot loading is %d%%", soot_load_CL);
    //Serial.println(mybuffer);
    soot_load_total = (soot_load_OL + soot_load_CL ) / 2;
  }
  else
  {
    soot_load_total = 0;
    //Serial.println("Unable to read CL soot loading DID");
    reset_can_driver();
  }


  show_led(soot_load_total);

  read_DID(0x03B8);
  if ((frame.data[0] == 0x07) & (frame.data[1] == 0x62) & (frame.data[2] == 0x03) & (frame.data[3] == 0xB8))
  {
    if ((frame.data[4] & 0x04) == 0x04)
    {
      digitalWrite(GP_LED, HIGH);
      //Serial.println("Vapouriser Glow Plug On");
    }
    else
    {
      digitalWrite(GP_LED, LOW);
      //Serial.println("Vapouriser Glow Plug Off");
    }
  }
  else
  { digitalWrite(GP_LED, LOW);
    //Serial.println("Unable to read glow plug status DID");
    reset_can_driver();
  }

  read_DID(0x041E);
  if ((frame.data[0] == 0x05) & (frame.data[1] == 0x62) & (frame.data[2] == 0x04) & (frame.data[3] == 0x1E))
  {
    set_pump_LED_speed(frame.data[5] );
    //sprintf (mybuffer, "Fuel pump frequency is %dhz", frame.data[5]);
    //Serial.println(mybuffer);
  }
  else
  { digitalWrite(PUMP_LED, LOW);
    set_pump_LED_speed(0);
    //Serial.println("Unable to read fuel pump DID");
    reset_can_driver();
  }

  delay(500);
}


//--------------------------------------------------------------------------------------
void reset_can_driver(void)
{
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setFilterMask(0, 0, 0x00000700);
  mcp2515.setFilterMask(1, 0, 0x00000700);
  mcp2515.setFilter(0, 0, 0x000007FF);
  mcp2515.setNormalMode();
}
//--------------------------------------------------------------------------------------
void read_DID(short DID_Number)
{
  char mybuffer[40];

  canMsg.can_id  = 0x7E0;
  canMsg.can_dlc = 8;
  canMsg.data[0] = 0x03;
  canMsg.data[1] = 0x22;
  canMsg.data[2] = (DID_Number & 0xFF00) / 255;
  canMsg.data[3] = DID_Number & 0x00FF;
  canMsg.data[4] = 0;
  canMsg.data[5] = 0;
  canMsg.data[6] = 0;
  canMsg.data[7] = 0;
  mcp2515.sendMessage(&canMsg);     //Sends the CAN message

  delay(100);

  if (mcp2515.readMessage(&frame) == MCP2515::ERROR_OK) {
    // frame contains received message
    //sprintf (mybuffer, "%02X %02X %02X %02X %02X %02X %02X %02X", frame.data[0], frame.data[1],  frame.data[2],  frame.data[3],  frame.data[4],  frame.data[5], frame.data[6], frame.data[7]);
    //Serial.println(mybuffer);
  }

}
//--------------------------------------------------------------------------------------
ISR(TIMER1_COMPA_vect)
{ //timer1 interrupt to flash pump LED

  if (loop_counter < 1)
  {
    loop_counter = fp_flash_const;
    if (toggle1) {
      if (LED_ON == true)
      { digitalWrite(PUMP_LED, HIGH);
      }
      else
      {
        digitalWrite(PUMP_LED, LOW);
      }
      toggle1 = 0;
    }
    else {
      digitalWrite(PUMP_LED, LOW);
      toggle1 = 1;
    }
  }
  else
  {
    loop_counter--;
  }
}

//--------------------------------------------------------------------------------------
void set_pump_LED_speed(int pump_speed)
{
  if (pump_speed > 0)
  {
    LED_ON = true;
    fp_flash_const = 20000 / pump_speed;
  }
  else
  {
    LED_ON = false;
  }
  return;
}
//--------------------------------------------------------------------------------------
void show_led(int soot_load)
{
  int base_offset = 0; // 0 to 100%
  //int base_offset=20; // 20 to 120%

  if (soot_load > base_offset + 99)
  { digitalWrite(LED_100, HIGH);
  }
  else
  { digitalWrite(LED_100, LOW);
  }

  if (soot_load > base_offset + 79)
  { digitalWrite(LED_80, HIGH);
  }
  else
  { digitalWrite(LED_80, LOW);
  }

  if (soot_load > base_offset + 59)
  { digitalWrite(LED_60, HIGH);
  }
  else
  { digitalWrite(LED_60, LOW);
  }

  if (soot_load > base_offset + 39)
  { digitalWrite(LED_40, HIGH);
  }
  else
  { digitalWrite(LED_40, LOW);
  }

  if (soot_load > base_offset + 19)
  { digitalWrite(LED_20, HIGH);
  }
  else
  { digitalWrite(LED_20, LOW);
  }

  if (soot_load > base_offset)
  { digitalWrite(LED_0, HIGH);
  }
  else
  { digitalWrite(LED_0, LOW);
  }

  return;
}
//--------------------------------------------------------------------------------------
