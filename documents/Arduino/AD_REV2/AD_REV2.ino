
#include "settings.h"
#include "AD_CHIP.h"
#include "Goertzel.h"
#include "Communications.h"
#include <ccspi.h>
#include <SPI.h>


// These two lines can be commented out to disable WIFI
#include "arduino_pi.h"
#define WIFI

////////////////////////// Global resourses /////////////////////////////////
typedef void (*func_ptr)();

AD_Chip AD;  //instance of the class that handles the AD chip
long data[MAX_SAMPLES];
int data_index;
int segment_count;
int recalibrate_counter;
int change_detected;
func_ptr run;
Goerzel_result v60, v180, v300, v420, i60, i180, i300, i420, start_phase;
Goerzel tracer_goerzel;
process_data tracer_process[4] { {60, 348},{60,349},{60, 350},{60,351} };
divider_process p60 (60);
divider_process p180(180);
divider_process p300(300);
divider_process p420(420);
long  base_active_power = 0;
float base_power_factor = 0;
float power_factor, active_power;



//////////////////////////////////////////////////////////////////////////////
////////////////////////////// working area  /////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void nothing(){}

// find zero - returns -1 if it fails
int find_zero(long* data)
{
    int max_loops = 100;
    bool up = false;
    process_data pd(60, CURRENT_SAMPLE_RATE);
    int count = 0;
    while(1)
    {
        count += 58;
        int step = 4;
        while(step)
        {
            float phase = pd.process_all(data+count).phase();
//            Serial.print(phase);
//            Serial.print(' ');
            --max_loops;
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
            if(count < 0 || count >= TRACER_MIN || max_loops <= 0)
            {
                if(Serial)
                    Serial.print("Failed to find zero, restarting identification\n");
                return -1;  
            }
        }
    }
}

// calibrate the sample rate - calls find_zero and returns it's value - returns -1 on error
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
        if(count < 0)
            return -1;
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
        
//    long current_active_power = AD.read_long(LAENERGY)*power_ratio;
//    long apparent_power = AD.read_long(LVAENERGY);
//    float current_power_factor;
//    if(apparent_power == 0)
//    {
//        reset_process();
//        return;
//          current_power_factor = 0;
//    }
//    else
//        current_power_factor = float(current_active_power)/AD.read_long(LVAENERGY);
  
//    if( changed(current_active_power, current_power_factor) )
//    {
//        reset_process(current_active_power, current_power_factor);
//        return;
//    }
        
    ++segment_count;
    int index = tracer_goerzel.count - TRACER_MIN;
    start_phase += tracer_process[index].get_results(tracer_goerzel);
    //Serial.println(index);
    if(segment_count == SEGMENTS_TO_READ)
    {
        //dump_data();
//        if(start_phase.phase() > 180)
//            start_phase.set_offset(START_PHASE_CORRECTION);
//        else
//            start_phase.set_offset(START_PHASE_CORRECTION+180);
        float phase_offset = start_phase.phase();
//        if(phase_offset > 90)
//            phase_offset -= 180;
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
            long apparent_power = AD.read_long(RVAENERGY);
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
            if(count < 0)
            {
                reset_process();
                return; 
            }
            //Serial.println(count);
            i60  = p60.process_all(data+count);
            i180 = p180.process_all(data+count);
            i300 = p300.process_all(data+count);
            i420 = p420.process_all(data+count);
            i60.adjust_phase(phase_offset);
            i180.adjust_phase(phase_offset);
            i300.adjust_phase(phase_offset);
            i420.adjust_phase(phase_offset);
        }
        send_all();
        
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
    run = process_mode_2;
}

void process_mode(){
    data_index = 0;
    data_index = 0;
    segment_count = 0;
    start_phase.clear();
    tracer_goerzel.clear();
    AD.read_irq();
    AD.read_long(RAENERGY);    //clear the accumulated powers
    AD.read_long(RVAENERGY);
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
    if(count < 0)
    {
       run = startup;
       return;
    }
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
#ifdef WIFI
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
    Serial.print("Connected!\n"); 
#endif    
    PAST_DATA = data + MAX_SAMPLES;

    AD.setup();
    //delay(5000);
    AD.write_int(LINECYC, lineCyc); // make line-cycle accumulation produce power readings
    AD.write_byte(CH1OS, ch1os);    // integrator setting
    AD.write_byte(GAIN, gain);      // gain adjust
    AD.enable_irq(WSMP);
    AD.enable_irq(CYCEND);
    if(Serial)
        Serial.print("Ready to begin Sampling\n");
    run = startup;
}

// the loop function runs over and over again forever
void loop(){
    run();
}

