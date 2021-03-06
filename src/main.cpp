#include "mbed.h"
#include "math.h"

// setup the SPI
// PC1 - CS (chip select for gyro)
// PF7 - SCLK (Serial clock)
// PF8 - MISO (Master in Slave out)
// PF9 - MOSI (Master out Slave in)

SPI spi(PF_9, PF_8, PF_7); // mosi, miso, sclk
DigitalOut gyro_cs(PC_1);  // chip select
int window_size =10;
int pit_stop =0;
int i = 0;
bool flag = false;
const int sample = 10;
int sampling_rate = 0.5;
float dps_x =0;
float dps_y =0;
float dps_z = 0;
float dx = 0;
float dy = 0;
float dz = 0;
float noisy_x[100];
float noisy_y[100];
float noisy_z[100];
int k=0;


void display()
{
  printf("\n\r\n\r");
  printf("datax = [");
  for (int i=0;i<100;i++){
    printf("%.2f,", noisy_x[i]);
  }
  printf("]");
  printf("\n\r\n\r");
  printf("datay = [");
  for (int i=0;i<100;i++){
      printf("%.2f,", noisy_y[i]);
  }
  printf("]");
  printf("\n\r\n\r");
  printf("dataz= [");
  for (int i=0;i<100;i++){
      printf("%.2f,", noisy_z[i]);
  }
  printf("]\n");
}
int window_sum(int start){
    int sum=0;
    for(int i=start; i<start+window_size; i++)
    {
      sum+=abs(noisy_x[i]);
    }
    return sum;
  }
void find_pitstop(){
  bool f= false;
  for (int i = 0;i<100;i+=10){
    int sum1 = window_sum(i);
    float avg= sum1/100;
    if(avg<5)
    {
      f=true;
      pit_stop=i;
    }
  }
  if(f==false)
  {
    pit_stop=100;
  }
}
void get_sample(){
    for (int j=0;j<sample;j++){
      gyro_cs = 0;
      spi.write(232);
      short x_axis_low = spi.write(0x00);
      short x_axis_high = spi.write(0x00);
      short x_axis = x_axis_high << 8 | x_axis_low;

      gyro_cs = 1;

      // Read 2 bytes of angular velocity data around the Y axis (address code: 0xEA)
      gyro_cs = 0;

      spi.write(234);

      short y_axis_low = spi.write(0x00);
      short y_axis_high = spi.write(0x00);
      short y_axis = y_axis_high << 8 | y_axis_low;

      gyro_cs = 1;

      // Read 2 bytes of angular velocity data around the Z axis (address code: 0xEC)
      gyro_cs = 0;

      spi.write(236);
      short z_axis_low = spi.write(0x00);
      short z_axis_high = spi.write(0x00);
      short z_axis = z_axis_high << 8 | z_axis_low;
      noisy_x[k]=x_axis*0.00875; // degrees per second
      noisy_y[k]=y_axis*0.00875;
      noisy_z[k]=z_axis*0.00875;
      k++;
      gyro_cs = 1;
      // degrees per second
      dps_x += abs((x_axis *0.00875)); 
      dps_y += abs((y_axis *0.00875));
      dps_z += abs((z_axis *0.00875));
      wait_us(50000);
    }

};

void config_gyro(){
  // Chip must be deselected
  gyro_cs = 1;

  // Setup the SPI for 8 bit data and SPI mode is 3
  spi.format(8, 3);
  // second edge capture, with a 1MHz clock rate
  spi.frequency(8000000);

  // initializing the gyroscope
  // Control register 1 (0x20) set to 00001111 -> 0x0F
  // enables all axes
  // turns on the gyroscope
  gyro_cs = 0;
  wait_us(1000);
  spi.write(32);
  spi.write(15);
  wait_us(1000);
  gyro_cs = 1;
  wait_us(2000000);
  // check if there is no bit in one of the registers where it shows my data is not ready
}
int main()
{
  config_gyro();
  float avg_dps_x[sample];
  float avg_dps_y[sample];
  float avg_dps_z[sample];
  while (1){
    printf("New batch\n");
    wait_us(1000000);
    
    for (int i=0;i<sample;i++){ // 5 seconds to run
      get_sample(); // 0.5 seconds to run
      avg_dps_x[i] = dps_x/sample;
      avg_dps_y[i] = dps_y/sample;
      avg_dps_z[i] = dps_z/sample;
      dps_x = 0;
      dps_y = 0;
      dps_z = 0;
    }
    k=0;
    display();
    find_pitstop();
  
    for (int j=0;j<sample;j++){
      dx += avg_dps_x[j] * 0.5 * 0.0174533;
      dy += avg_dps_y[j] * 0.5 * 0.0174533;
      dz += avg_dps_z[j] * 0.5 * 0.0174533;
    }

    float dis = sqrt(pow(dx, 2) + pow(dy, 2) + pow(dz, 2));
    float s = dis * 1.5;
    if(s<0.5)
    {
      s=0.00;  //This is when there is no movement, so it becomes 0/0 
    }
    //float time=((pit_stop*0.5)/10);
    float linear_velocity = s/((pit_stop*0.5)/10);
    printf("distance  = %.2f\n", s);
    //printf("Walked for = %.2f\n", time);
    printf("velocity = %.2f\n", linear_velocity);
    avg_dps_x[sample] = {0};
    avg_dps_y[sample] = {0};
    avg_dps_z[sample] = {0};
    dx=0;
    dy=0;
    dz=0;
  }
}