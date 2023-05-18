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
  
  //ADC prevodnik
  ADMUX = (1<<6);  // nastavujem ake referencne napatie pouzije
  ADCSRA |= (1<<7);  // ADC Enable bit, ak je 0 tak ADC je vypnuty
  ADCSRA |= 7;  // nastavujem division factor (128) pre ADC clock


  //Disable ADC - nezabudni to potom zapnut ADCSRA |= (1 << 7);
  ADCSRA &= ~(1 << 7);
 
  
  //Enable Sleep - tunak zadavam do akeho spanku pojde
  SMCR |= (1 << 2);  // power down mode
  SMCR |= 1;  //enable sleep
  
  //TIMER 1 
  TCCR1A = B10100010;
  TCCR1B = B00011010;
  ICR1 = 40000;         // PWM frekvencia je 50hz (perioda 20ms)
  OCR1A = 2000;         // duty cycle je od 1ms do 2ms (teda od 2000 do 4000)
  OCR1B = 3100;


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

    if(deep_sleep_enable){
      count_to_deep_sleep++;
      if(count_to_deep_sleep == 4){
        count_to_deep_sleep = 0;
    
        //ADC prevodnik vypinam
        ADCSRA &= ~(1<<7);

        PORTB = B00000000;
        
        //Bod Disable
        MCUCR |= (3 << 5); //nastavujem BODS and BODSE naraz
        MCUCR = (MCUCR & ~(1 << 5)) | (1 << 6); //nastav BODS a clear BODSE naraz
        __asm__ __volatile__("sleep");
      }
    }

  }

}

ISR(WDT_vect){
  //INTERUPT RUTINA PRE WATCH DOG TIMER  
}
