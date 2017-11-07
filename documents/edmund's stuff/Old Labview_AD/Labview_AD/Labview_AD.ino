
//Sorry Jeff, moved my whole header into your file for testing, I have the code in commented blocks. Was getting include errors.

//-------------------------------------------------------------------
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <string.h>
#include <stdint.h>
#include "utility/debug.h"

//The stuff down there needs to be global

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed but DI
Adafruit_CC3000_Client pi_connection;

const uint32_t DHCP_TIMEOUT = 300000;
const bool STATUS_MESSAGES = true;
const uint32_t DATA_LEN = 40;

#include <SPI.h>
#include "settings.h"
#include "AD_CHIP.h"
#include "Goertzel.h"
//#include "arduino_pi.h"
//276 and 306 are where the wifi functions will be called

////////////////////////// Global resourses /////////////////////////////////
typedef void (*func_ptr)();

AD_Chip AD;  //instance of the class that handles the AD chip
long data[MAX_SAMPLES];
int data_index;
int segment_count;
int recalibrate_counter;
int change_detected;
func_ptr run;
Goerzel_result v60, v180, v300, v420, i60, i180, i300, i420;
Goerzel tracer_goerzel;
process_data tracer_process[4] { {60, 348},{60,349},{60, 350},{60,351} };
divider_process p60 (60);
divider_process p180(180);
divider_process p300(300);
divider_process p420(420);
long  base_active_power = 0;
float base_power_factor = 0;
float start_phase, power_factor, active_power;



/////////////////////// Serial input processing ////////////////////////////

//enum only used by serial event
enum {get_reg, get_1_2, get_2_2, get_1}serial_state = get_reg;

void serialEvent() {
    static byte reg, data;
    while(Serial.available()){
        byte new_data = Serial.read();
        Serial.write(new_data);
        switch(serial_state){
            case get_reg:
            {
                if(new_data == 0x00){
                    startup();
                    return;
                }
                if(new_data == 0x01){
                    long l_data = AD.read_waveform();
                    byte* access = (byte*)&l_data;
                    Serial.write(access[2]);  //reverse endian order
                    Serial.write(access[1]);
                    Serial.write(access[0]);
                    return;
                }
                if(new_data == 0xAA){
                    AD.write_byte(CH1OS,0x00);
                    AD.write_byte(CH1OS,ch1os);
                    run = nothing;
                    return;
                }
                reg = new_data & 0x7F; //save it without the write bit set
                if(reg > last_REG){
                    Serial.write(0xA1);  //send an error code back if this value is out of range
                    //Serial.write(reg);
                    break;            //and do nothing with this byte
                }
                byte reg_size = REG_bytes[reg];
                if(new_data & 0x80){     //was write indicated?
                    switch(reg_size){    //if so set the next state by the data_size
                        case 1:
                            serial_state = get_1;
                            break;
                        case 2:
                            serial_state = get_1_2;
                            break;
                        default:       //if the value is not 1 or 2 then this is not a register we write to
                            Serial.write(0xA2);  //so send an error code back (and do nothing with this byte)
                    }
                }
                else {  //this is a read so no other data to recieve so lets read the data and send it back
                    if(reg_size == 3){
                        long long_data = AD.read_long(reg);
                        byte* access = (byte*)&long_data;
                        Serial.write(access[2]);  //reverse endian order
                        Serial.write(access[1]);
                        Serial.write(access[0]);
                    }
                    else if(reg_size == 2){
                        int int_data = AD.read_int(reg);
                        byte* access = (byte*)&int_data;
                        Serial.write(access[1]);  //reverse endian order
                        Serial.write(access[0]);
                    }
                    else{
                        data = AD.read_byte(reg);
                        Serial.write(data);
                    }
                }
            }
                break;
            case get_1_2:
                data = new_data;
                serial_state = get_2_2;
                break;
            case get_2_2:
                union{
                    int i;
                    byte b[2];
                }d2;
                d2.b[1]=data;
                d2.b[0]=new_data;
                AD.write_int(reg,d2.i);
                serial_state = get_reg;
                break;
            default:
                AD.write_byte(reg,new_data);
                serial_state = get_reg;
                break;
        }
    }
    
}

//////////////////////////////////////////////////////////////////////////////
////////////////////////////// working area  /////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void nothing(){}

// find zero assuming the phase prediction won't work (pre calibration)
int find_zero(long* data)
{
    bool up = false;
    process_data pd(60, CURRENT_SAMPLE_RATE);
    int count = 0;
    while(1)
    {
        count += 58;
        int step = 4;
        while(step)
        {
            float phase = pd.process_all(data+count).phase;
//            Serial.print(phase);
//            Serial.print(' ');
            if(phase_from_zero(phase) < .5)
            {
                return count;
            }
            if(up)
            {
                if(phase < 180)
                {
                    up = false;
                    step>>=1;
                    count -= step;
                }
                else
                    count += step;
            }
            else
            {
                if (phase > 180)
                {
                    up = true;
                    step>>=1;
                    count += step;
                }
                else
                    count -=step;
            }
        }
    }
}

// calibrate the sample rate - calls find_zero and returns it's value
int calibrate(long* data){
    const int step_start = 10;
    power_only p59(59);
    power_only p61(61);
    float r59, r61;
    float my_rate = CURRENT_SAMPLE_RATE;
    float step = step_start;
    int count = 0;
    int cycles = 2;
    bool up = true;
    while(cycles)
    {
        count = find_zero(data);

        while(step > .001)
        {
            p59.calculate_constants(my_rate);
            p61.calculate_constants(my_rate);
            r59 = p59.process_all(data+count, my_rate);
            r61 = p61.process_all(data+count, my_rate);
            if(r59 > r61)
                if(up)
                {
                    up = false;
                    my_rate -= step;
                    step /= 2;
                }
                else
                    my_rate += step;
            else
                if(up)
                    my_rate -= step;
                else
                {
                    up = false;
                    my_rate -= step;
                    step /= 2;
                }
        }
        calc::calibrate_all(my_rate);
        step = step_start;
        --cycles;
    }
    return count;
}

//sample the waveform and save it in the array along with the time
bool sample_waveform(){
    if (data+data_index >= PAST_DATA)  //we are going off the end of the data!
    {
        if(Serial)
            Serial.write("Critical ERROR: attempting to go past end of data in sample_waveform.\n");
        run = startup;
        return true;
    }
    data[data_index] = AD.read_waveform();
    ++data_index;
    if (data_index == MAX_SAMPLES)
        return false;
    return true;
}

bool within_m(float a,float b,float threshold){
    if(a*(1+THRESHOLD) < b || a*(1-THRESHOLD) > b)
        return false;
    return true;
}

bool power_changed(long ac_power, float pf){
    if(within_m(ac_power, active_power, THRESHOLD) || within_m(pf, power_factor, THRESHOLD))
        return true;
    return false;
}

void dump_data()
{
    if(Serial)
    { 
        Serial.print("\nDisplaying all data gathered:\n");
        for(int i = 0; i< data_index; ++i)
        {
           Serial.print(data[i]); 
           Serial.print('\n');
        }
    }
}

void send_update_v(){
    if(Serial)
    {
        Serial.print("\nVoltage Results:");
        Serial.print("\n60hz: ");
        Serial.print(v60.amp);
        Serial.print(" ");
        Serial.print(v60.phase);
        Serial.print("\n180hz: ");
        Serial.print(v180.amp);
        Serial.print(" ");
        Serial.print(v180.phase);
        Serial.print("\n300hz: ");
        Serial.print(v300.amp);
        Serial.print(" ");
        Serial.print(v300.phase);
        Serial.print("\n420hz: ");
        Serial.print(v420.amp);
        Serial.print(" ");
        Serial.print(v420.phase);
        Serial.print('\n');
    }
    float wifi_package[10] = { 0,0,v60.amp,v60.phase,v180.amp,v180.phase,v300.amp,v300.phase,v420.amp,v420.phase};
    write_data(wifi_package);
}

void send_update_i(){
    if(Serial)
    {
        Serial.print("\nCurrent Results:");
        Serial.print("\nPower: ");
        Serial.print(active_power);
        Serial.print(" Power_Factor: ");
        Serial.print(power_factor);
        Serial.print("\n60hz: ");
        Serial.print(i60.amp);
        Serial.print(" ");
        Serial.print(i60.phase);
        Serial.print("\n180hz: ");
        Serial.print(i180.amp);
        Serial.print(" ");
        Serial.print(i180.phase);
        Serial.print("\n300hz: ");
        Serial.print(i300.amp);
        Serial.print(" ");
        Serial.print(i300.phase);
        Serial.print("\n420hz: ");
        Serial.print(i420.amp);
        Serial.print(" ");
        Serial.print(i420.phase);
        Serial.print('\n');
    }
    float wifi_package[10] = {active_power,power_factor,i60.amp,i60.phase,i180.amp,i180.phase,i300.amp,i300.phase,i420.amp,i420.phase};
    write_data(wifi_package);
}

void send_all()
{
    if(Serial)
        {
              Serial.print(CURRENT_SAMPLE_RATE);
              Serial.print(' ');
              Serial.print(active_power);
              Serial.print(' ');
              Serial.print(power_factor);
              Serial.print(' ');
              Serial.print(start_phase);
              Serial.print(' ');
              Serial.print(i60.amp);
              Serial.print(' ');
              Serial.print(i60.phase);
              Serial.print(' ');
              Serial.print(i180.amp);
              Serial.print(' ');
              Serial.print(i180.phase);
              Serial.print(' ');
              Serial.print(i300.amp);
              Serial.print(' ');
              Serial.print(i300.phase);
              Serial.print(' ');
              Serial.print(i420.amp);
              Serial.print(' ');
              Serial.print(i420.phase);
              Serial.print('\n');
        }
}

bool changed(long, float)
{
    //TODO: fill this in
    return false;
}

void reset_process(long current_active_power = 0, float current_power_factor = 0)
{
    base_active_power = current_active_power;
    base_power_factor = current_power_factor;
    run = process_mode; 
}
void process_mode_2(){

    if(!AD.read_irq())
        return;
        
    if(AD.irq_set(WSMP))
    {
        sample_waveform();
        p60.process(tracer_goerzel, data[data_index]);
    }
    if(!AD.irq_set(CYCEND))
        return;
    long current_active_power = AD.read_long(LAENERGY)*power_ratio;
    long apparent_power = AD.read_long(LVAENERGY);
//    if(apparent_power == 0)
//    {
//        reset_process();
//        return;
//    }
    
  
    float current_power_factor = float(current_active_power)/AD.read_long(LVAENERGY);
    if( changed(current_active_power, current_power_factor) )
    {
        reset_process(current_active_power, current_power_factor);
        return;
    }
        
    ++segment_count;
    int index = tracer_goerzel.count - TRACER_MIN;
    start_phase += tracer_process[index].get_results(tracer_goerzel).phase;
    //Serial.println(index);
    if(segment_count == SEGMENTS_TO_READ)
    {
        //dump_data();
        start_phase /= SEGMENTS_TO_READ;
        start_phase -= START_PHASE_CORRECTION;
        active_power = AD.read_long(RAENERGY)*power_ratio;
//        if(active_power < min_power)
//        {
//            active_power = 0;
//            power_factor = 0;
//            i60.clear();
//            i180.clear();
//            i300.clear();
//            i420.clear();
//        }
//        else
        {
            apparent_power = AD.read_long(RVAENERGY);
            if(apparent_power == 0)
            {
//                reset_process();
//                return;
                  power_factor = 0;
            }
            else
                power_factor = active_power/apparent_power;
            active_power *= power_scaler;
            int count = find_zero(data);
            //Serial.println(count);
            i60  = p60.process_all(data+count);
            i180 = p180.process_all(data+count);
            i300 = p300.process_all(data+count);
            i420 = p420.process_all(data+count);
            i60.adjust_phase(start_phase);
            i180.adjust_phase(start_phase);
            i300.adjust_phase(start_phase);
            i420.adjust_phase(start_phase);
        }
        send_update_i();
        
        ++recalibrate_counter;
        if (recalibrate_counter == RECALIBRATE_CYCLES)
            run = startup;
        else
            run = process_mode;
    }
}

void process_mode_1(){
    
    if(!AD.read_irq())
        return;
    
    if(!AD.irq_set(CYCEND))
        return;
    AD.read_long(RAENERGY);    //clear the accumulated powers
    AD.read_long(RVAENERGY);
    sample_waveform();
    run = process_mode_2;
}

void process_mode(){
    data_index = 0;
    AD.read_irq();
    data_index = 0;
    segment_count = 0;
    start_phase = 0;
    run = process_mode_1;
}


void startup_1(){
    
    if(!AD.read_irq())
        return;
    if(!AD.irq_set(WSMP))
        return;
    if(sample_waveform())   //indication returned our sampling is not done?
        return;
        
//    if(Serial)
//        Serial.print("Samples Gathered\n");
    int count = calibrate(data);
//    if(Serial)
//    {
//        Serial.print("Calibration complete:\nZero found at sample: ");
//        Serial.print(count);
//        Serial.print(" and sample_rate set to: ");
//        Serial.print(CURRENT_SAMPLE_RATE);
//    }
    v60 = p60.process_all(data+count);
    v180 = p180.process_all(data+count);
    v300 = p300.process_all(data+count);
    v420 = p420.process_all(data+count);
    AD.write_int(MODE, iMode);  // enable normal mode
//    send_update_v();
//    if(Serial)
//        dump_data();
    send_update_v();
    run = process_mode;
}

// get ready to gather samples, calibrate the sample_rate and process the voltage wave
void startup(){
    AD.write_int(MODE, vMode);  // sample voltage on ADC1
    data_index = 0;
    recalibrate_counter = 0;
    run = startup_1;
}


////////////////////////////// Main Functions /////////////////////////////



//the setup function runs once when reset button pressed, board powered on, or serial connection
void setup() {
    Serial.begin(115200);  
    while(!setup_cc3000());  //loop until it connects to wifi
    if(Serial)
    {
        Serial.print("Connecting to IP:");
        Serial.print(IP);
        Serial.print(" Port number:");
        Serial.println(PORTNO);
    }
    while(!tcp_connect(IP, PORTNO))  //loop 
        if(Serial)
            Serial.print("Could not connect. Retrying...");
            
    PAST_DATA = data + MAX_SAMPLES;
    // connect to wifi
    AD.setup();
    //delay(5000);
    AD.write_int(LINECYC, lineCyc); // make line-cycle accumulation produce power readings
    AD.write_byte(CH1OS, ch1os);    // integrator setting
    AD.write_byte(GAIN, gain);      // gain adjust
    AD.enable_irq(WSMP);
    AD.enable_irq(CYCEND);
    if(Serial)
        Serial.print("Sample_Rate Active_Power Power_Factor tracer_phase 60_amp 60_phase 180_amp 180_phase 300_amp 300_phase 420_amp 420_phase\n");
    run = startup;
}

// the loop function runs over and over again forever
void loop(){
    run();
}


//------------------------------------------------------------------
void delay1k(void)
{
  delay(1000);
}

bool setup_cc3000()
{
  if(Serial)
  {
    displayDriverMode();
    Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
    Serial.println(F("Initialising the CC3000 ..."));
  }
  if(Serial)
  {
    Serial.println(F("Right before cc3000.begin"));
  }
  if (!cc3000.begin())
  {
    if(Serial)
      Serial.println(F("Unable to initialise the CC3000! Check your wiring?"));
    //while(1);
    return false;
  }
  if(Serial)
  {
    Serial.println(F("Right before firmware check."));
  }
  uint16_t firmware = checkFirmwareVersion();
  if (firmware < 0x113) {
    if(Serial)
      Serial.println(F("Wrong firmware version!"));
    //for(;;);
    return false;
  }
  if(Serial)
  {
    Serial.println(F("Right before display MAC."));
  }
  if(Serial)
  {
    displayMACAddress();
    Serial.println(F("Deleting old connection profiles"));
  }
  if (!cc3000.deleteProfiles()) {
    if(Serial)
      Serial.println(F("Failed!"));
    //while(1);
    return false;
  }

  /* Attempt to connect to an access point */
  char *ssid = WLAN_SSID;             /* Max 32 chars */
  if(Serial)
    Serial.print(F("Attempting to connect to ")); Serial.println(ssid);

  /* NOTE: Secure connections are not available in 'Tiny' mode!
     By default connectToAP will retry indefinitely, however you can pass an
     optional maximum number of retries (greater than zero) as the fourth parameter.
  */
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    if(Serial)
      Serial.println(F("Failed!"));
    //while(1);
    return false;
  }

  if(Serial)
  {
    Serial.println(F("Connected!"));
    Serial.println(F("Request DHCP"));
  }

  uint32_t DHCP_time = 0;
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
    DHCP_time += 100;
    if (DHCP_time >= DHCP_TIMEOUT)
      return false;
  }

  if (Serial)
  {
    while (! displayConnectionDetails()) {
      delay(1000);
    }
    Serial.println(F("IP to connect to : "));
    cc3000.printIPdotsRev(IP);
    Serial.println();
  }

  if (cc3000.checkConnected())
  {
    if(Serial)
      Serial.println(F("Succesfully connected to internet."));
    return true;
  }
}

bool tcp_connect (uint32_t tcp_ip, uint16_t tcp_port)
{
  pi_connection = cc3000.connectTCP(tcp_ip, tcp_port);
  if (pi_connection.connected())
    return true;
  else
    return false;
}

void write_random_data(uint16_t iterations)
{
  randomSeed(analogRead(0));
  for (uint16_t i = 0; i < iterations; i++)
  {
    float random_string1[] = {random(1, 1000000), random(1, 1000000), random(1, 1000000), random(1, 1000000), random(1, 1000000), random(1, 1000000), random(1, 1000000), random(1, 1000000), random(1, 1000000), random(1, 1000000),};
    float random_string2[] = {0, 0, random(1, 1000000), random(1, 1000000),random(1, 1000000), random(1, 1000000), random(1, 1000000), random(1, 1000000), random(1, 1000000), random(1, 1000000),};
    Serial.println(F("Writing data."));
    if(!write_data(random_string1))
    {
      Serial.println(F("Error writing random data."));
      break;
    }
    Serial.println(F("Data written."));
    delay1k();
    Serial.println(F("Writing data."));
    if(!write_data(random_string2))
    {
      Serial.println(F("Error writing random data."));
      break;
    }
    Serial.println(F("Data written."));
    delay1k();
  }
}

bool write_data(float data[])
{
  while (!cc3000.checkConnected())
  {
    cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY);
  }
  if (cc3000.checkConnected())
  {
    pi_connection.write(data, DATA_LEN);
    delay1k();
    return true;
  }
  return false;
}

void close_connection()
{
  if (cc3000.checkConnected())
  {
    float termination_string[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    pi_connection.write(termination_string, DATA_LEN);
  }
  pi_connection.close();
  cc3000.disconnect();
}


/**************************************************************************/
/*!
    @brief  Displays the driver mode (tiny of normal), and the buffer
            size if tiny mode is not being used

    @note   The buffer size and driver mode are defined in cc3000_common.h
*/
/**************************************************************************/
void displayDriverMode(void)
{
  #ifdef CC3000_TINY_DRIVER
    Serial.println(F("CC3000 is configure in 'Tiny' mode"));
  #else
    Serial.print(F("RX Buffer : "));
    Serial.print(CC3000_RX_BUFFER_SIZE);
    Serial.println(F(" bytes"));
    Serial.print(F("TX Buffer : "));
    Serial.print(CC3000_TX_BUFFER_SIZE);
    Serial.println(F(" bytes"));
  #endif
}

/**************************************************************************/
/*!
    @brief  Tries to read the CC3000's internal firmware patch ID
*/
/**************************************************************************/
uint16_t checkFirmwareVersion(void)
{
  uint8_t major, minor;
  uint16_t version;

#ifndef CC3000_TINY_DRIVER
  if(!cc3000.getFirmwareVersion(&major, &minor))
  {
    Serial.println(F("Unable to retrieve the firmware version!\r\n"));
    version = 0;
  }
  else
  {
    Serial.print(F("Firmware V. : "));
    Serial.print(major); Serial.print(F(".")); Serial.println(minor);
    version = major; version <<= 8; version |= minor;
  }
#endif
  return version;
}

/**************************************************************************/
/*!
    @brief  Tries to read the 6-byte MAC address of the CC3000 module
*/
/**************************************************************************/
void displayMACAddress(void)
{
  uint8_t macAddress[6];

  if(!cc3000.getMacAddress(macAddress))
  {
    Serial.println(F("Unable to retrieve MAC Address!\r\n"));
  }
  else
  {
    Serial.print(F("MAC Address : "));
    cc3000.printHex((byte*)&macAddress, 6);
  }
}


/**************************************************************************/
/*!
    @brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;

  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}

/**************************************************************************/
/*!
    @brief  Begins an SSID scan and prints out all the visible networks
*/
/**************************************************************************/

void listSSIDResults(void)
{
  uint32_t index;
  uint8_t valid, rssi, sec;
  char ssidname[33];

  if (!cc3000.startSSIDscan(&index)) {
    Serial.println(F("SSID scan failed!"));
    return;
  }

  Serial.print(F("Networks found: ")); Serial.println(index);
  Serial.println(F("================================================"));

  while (index) {
    index--;

    valid = cc3000.getNextSSID(&rssi, &sec, ssidname);

    Serial.print(F("SSID Name    : ")); Serial.print(ssidname);
    Serial.println();
    Serial.print(F("RSSI         : "));
    Serial.println(rssi);
    Serial.print(F("Security Mode: "));
    Serial.println(sec);
    Serial.println();
  }
  Serial.println(F("================================================"));

  cc3000.stopSSIDscan();
}

//-----------------------------------------------------------------------------------------

