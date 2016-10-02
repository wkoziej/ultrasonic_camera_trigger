#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <time.h>
#define TRIG 18
#define ECHO 24 
#define LIGHT 17
#define NO_OF_FOLLOWING_READS_TO_ACTIVE_CAM 2

int noOfCmToActivateCam = 30;
void setup() {
  wiringPiSetupGpio();
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(LIGHT, OUTPUT);
  // TRIG pin must start LOW
  digitalWrite(TRIG, LOW);
  delay(30);
}

int getCM() {
  // Send trig pulse
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(20);
  digitalWrite(TRIG, LOW);

  // Wait for echo start
  while(digitalRead(ECHO) == LOW);

  // Wait for echo end
  long startTime = micros();
  while(digitalRead(ECHO) == HIGH);
  long travelTime = micros() - startTime;
  //Get distance in cm
  int distance = travelTime / 58;
  return distance;
}

int objectAvailable () {
  int distance = getCM();
  printf("Distance %dcm\n", distance);
  static int numberOfReads = 0;
  if (distance < noOfCmToActivateCam) {
    numberOfReads ++;
  } else {
    numberOfReads = 0;
  }
  return numberOfReads > NO_OF_FOLLOWING_READS_TO_ACTIVE_CAM;
}

void lightOn () {
  digitalWrite(LIGHT, HIGH);
}

void lightOff () {
  digitalWrite(LIGHT, LOW);
}

void record(int numberOfImages) {
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  char imageName [256];
  char makeImageCommand [256];
  int i = 0;
  sprintf(imageName, "%d-%d-%d_%d:%d:%d_%d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, i);
  sprintf(makeImageCommand, "fswebcam -d/dev/video0 -i0 -r800x600 \"images/%s.jpg\" & ", imageName);
  system(makeImageCommand);
  sprintf(makeImageCommand, "avconv -f video4linux2 -r 7 -s 640x480 -t %d -i /dev/video0 -y \"images/%s.avi\" & ",
	  numberOfImages, imageName);
  system(makeImageCommand);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf ("Usage: ultra <number_of_centimeters_to_activete_camera_and_light> [s]\n");
    printf ("\ts - do not make images");
  }

  noOfCmToActivateCam = atoi(argv[1]);
  int simulate = argc > 2 && argv[2][0] == 's';
  setup();
  while (1) {		  
    if ( objectAvailable() ) {
      int movieSecs = 8;
      lightOn();
      if (!simulate)  {
	record(movieSecs);
      }       
      delay(1000 * movieSecs);
    } else {
      lightOff();
    }
    // If we teke to low delay ultrosonic sensors sometimes hangs
    delay (500);
  }
  return 0;
}
