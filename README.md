# 2-Axis-Solar-Tracker


https://github.com/adus-hash/2-Axis-Solar-Tracker/assets/66412479/bd365825-5a47-4ba2-9830-0a58dbff46f9



It is an assembled mechanism that can be divided into 3 subsystems (input, data processing, output). The input is a subsystem that consists of four photoresistors, which are placed on the shade, each photoresistor data through the voltage divider by the amount of light falling on it. This data is sampled by the Arduino, which takes care of the data processing subsystem, based on the program, the Arduino finds out which photoresistor receives less light and, accordingly, produces an adequate output in the form of turning the relevant servo motor or putting the Arduino into deep sleep (LED indicate whether the Arduino is in deep sleep).

![solar tracker schema](https://github.com/adus-hash/2-Axis-Solar-Tracker/assets/66412479/4c8fbcd3-ca6e-467b-8747-7f45ac20264b)


## Light intensity sensor
The light intensity sensor is the eyes of this project, because our project is dual-axial, which means that it can move in two axes (up/down and right/left) and it must also see in these axes, that is the reason why we used 4 photoresistors that will be placed on the shade, one in each quarter, it is enough for the light source to move in any direction and shade 2 to 3 photoresistors, as less light will fall on them, their resistance will increase and thus the voltage drop. There's a voltage divider on the circuit board and four yellow cabels going to analog input on Arduino (A0-A4).

![2023_05_18_15_58_IMG_0786](https://github.com/adus-hash/2-Axis-Solar-Tracker/assets/66412479/f38e477f-1787-47bb-829e-30a1bbff1283)

## Arduino and program
We decided write code using register because we wanted to be as much efficient as possible so we basically shut down unnecessary hardware to lower power consumption of Arduino. Our algorithm for this program is very simple, it starts with reading the analog voltages from the sensor, then we compare these values with each other and if the difference between the values is greater than 27, then the adequate servo is turned and such an endless cycle of reading and comparison runs until one servo won't move for four cycles, then the Arduino goes into deep sleep to save power, wakes up after eight seconds and goes into the read and compare cycle again. 


```C++
void my_delay(unsigned long cas){
  for(unsigned long i = 0; i <  cas; i++){
    __asm__ __volatile__("nop");  //1 clock cycle trva 62.5ns
  }
}

int my_analog_read(){
  ADCSRA |= (1<<6);  // tymto zacne cely ADC prevod, kus pockas a vysledok mas v ADCH a ADCL reg. teda 10 bit
  my_delay(90000);
  uint8_t low = ADCL;  // citas digitalnu hodnotu z prevodu najprv musim precitas low byte az potom high byte inac ti ich pri dalsom prevode nezaznamenalo
  uint8_t high = ADCH;
  
  int result = (high<<8) | low;  // tu konvertuje 2 + 8 bitov na "10" bitove cislo

  return result;
}
```

Here we just define our own delay and analog_read functions.

```C++
int main(){
  sei();
  
 
  int down_right;
  int up_right;
  int up_left;
  int down_left;

  int odchylka = 27;
  bool deep_sleep_enable = true;
  byte count_to_deep_sleep = 0;

  DDRB = 255;

  //Power Reduction Register
  PRR = B11100110; // vypinam TWI, Timer2, Timer0, SPI, USART
```
Defining variables we will need and shuting down unnecessary hardware like Timer0, Timer2 to reduce power consumption.

```C++
  //ADC prevodnik
  ADMUX = (1<<6);  // nastavujem ake referencne napatie pouzije
  ADCSRA |= (1<<7);  // ADC Enable bit, ak je 0 tak ADC je vypnuty
  ADCSRA |= 7;  // nastavujem division factor (128) pre ADC clock


  //Disable ADC - nezabudni to potom zapnut ADCSRA |= (1 << 7);
  ADCSRA &= ~(1 << 7);
 
  
  //Enable Sleep - tunak zadavam do akeho spanku pojde
  SMCR |= (1 << 2);  // power down mode
  SMCR |= 1;  //enable sleep
```

Setting ADC converter and setting the deepest sleep mode.

```C++
  //TIMER 1 
  TCCR1A = B10100010;
  TCCR1B = B00011010;
  ICR1 = 40000;         // PWM frekvencia je 50hz (perioda 20ms)
  OCR1A = 2000;         // duty cycle je od 1ms do 2ms (teda od 2000 do 4000)
  OCR1B = 3100;
```
Setting timer for 50Hz PWM signal on pin 9 and 10 and duty cycle from 1-2 ms.

```C++
while(true) {
    //opat zapinam ADC zo spanku
    ADCSRA |= (1<<7);
    deep_sleep_enable = true;

    PORTB = B00100000;
    my_delay(100000);
    
    //citam analogove hodnoty z fotorezistorov
    ADMUX &= ~((1<<0) | (1<<1));  // pin A0
    down_right = my_analog_read() * 0.82;

    ADMUX &= ~((1<<0) | (1<<1));  // pin A1
    ADMUX |= (1<<0);
    up_right = my_analog_read() * 0.87;
    
    ADMUX &= ~((1<<0) | (1<<1));  // pin A2
    ADMUX |= (1<<1);
    up_left = my_analog_read();

    ADMUX |= (1<<0) | (1<<1);     // pin A3
    down_left = my_analog_read() * 0.90;
```

Reading analog values from Light intensity sensor.

```C++
// posuv x
    if(OCR1A > 2000){
      if(down_left - down_right > odchylka){
        OCR1A -= 10;
        my_delay(100000);
        deep_sleep_enable = false;
      }
      
      if(up_left - up_right > odchylka){
        OCR1A -= 10;
        my_delay(100000);
        deep_sleep_enable = false;
      }
    }

    if(OCR1A <4000){
      if(down_right - down_left > odchylka){
        OCR1A += 10;
        my_delay(100000);
        deep_sleep_enable = false;
      }
  
      if(up_right - up_left > odchylka){
        OCR1A += 10;
        my_delay(100000);
        deep_sleep_enable = false;
      }
    }
```

For example difference between right and left photoresistor is greater than 27 that means one of them is shaded so servo will turn to correct direction. This is for X axis.

```C++
    //posuv y
    if(OCR1B > 3100){
      if(down_right - up_right > odchylka){
        OCR1B -= 10;
        my_delay(100000);
        deep_sleep_enable = false;
      }
      
      if(down_left - up_left > odchylka){
        OCR1B -= 10;
        my_delay(100000);
        deep_sleep_enable = false;
      }
    }

    if(OCR1B < 4000){
      if(up_right - down_right > odchylka){
        OCR1B += 10;
        my_delay(100000);
        deep_sleep_enable = false;
      }
  
      if(up_left - down_left > odchylka){
        OCR1B += 10;
        my_delay(100000);
        deep_sleep_enable = false;
      }
    }
```
This is for Y axis, if the servo is not at the limit it can move about 70°

```C++
if(deep_sleep_enable){
      count_to_deep_sleep++;
      if(count_to_deep_sleep == 4){
        count_to_deep_sleep = 0;

        //Watch Dog Timer
        WDTCSR = 24;  // change enable a WDE nastavujem naraz
        WDTCSR = 33;  // 8s v power down mode
        WDTCSR |= (1<<6);  // povol interupt mode
    
        //ADC prevodnik vypinam
        ADCSRA &= ~(1<<7);

        PORTB = B00000000;
        
        //Bod Disable
        MCUCR |= (3 << 5); //nastavujem BODS and BODSE naraz
        MCUCR = (MCUCR & ~(1 << 5)) | (1 << 6); //nastav BODS a clear BODSE naraz
        __asm__ __volatile__("sleep");
      }
    }
```
Here if deep_sleep_enable is true it means that not a single servo moved for 4 cycles and that means our source of light don't move either and Arduino can go to deep sleep because we don't need to waste energy for tracking because our source of light don't move, after 8 seconds Arduino wake up and whole cycle repeats.
