#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/io.h>
#include <signal.h>

#define target_temp 60
#define tsensorfile "/sys/class/thermal/thermal_zone0/temp"


#define fan_control_reg 0x93
#define fan_manual_mode 0x14
#define fan_auto_mode 0x04
#define fan_speed_reg 0x94
#define fan_rpm_reg 0x95
#define fan_speed_val_0 0xff
#define fan_speed_val_10 0xe6 
#define fan_speed_val_20 0xc8
#define fan_speed_val_40 0x96
#define fan_speed_val_50 0x7e
#define fan_speed_val_60 0x64
#define fan_speed_val_80 0x32



void wait_write_ec(void) {
	while (inb(0x66) & 0x02) {
		usleep(10000);
	}
}

void wait_read_ec(void) {
	while (!(inb(0x66) & 0x01)) {
		usleep(10000);
	}
}

void write_ec (int reg,int value) {
	wait_write_ec();
	outb(0x81,0x66);
	wait_write_ec();
	outb(reg,0x62);
	wait_write_ec();
	outb(value,0x62);
}

int read_ec (int reg) {
	wait_write_ec();
	outb(0x80, 0x66);
	wait_write_ec();
	outb(reg, 0x62);
	wait_read_ec();
	return inb(0x62);
}


int gettemp(void) {
	int temp;
	FILE *tsensorfd;
	if ((tsensorfd = fopen(tsensorfile,"r")) == 0) {
		perror("sensfile");
		exit(1);
	}
	fscanf(tsensorfd, "%d", &temp);
	fclose(tsensorfd);
	return temp/1000; 
}

void speedSet(int speed) {
	speed = (speed > 255 ? 255 : (speed < 0 ? 0 : speed)); 
	write_ec(fan_speed_reg,255-speed);
}

int speedGet(void) {
	return 255-read_ec(fan_rpm_reg);
}
void onintr(sig){
	write_ec(fan_control_reg,fan_auto_mode);
	exit(0);
}

int main()
{
	if (ioperm(0x62, 1, 1)) {perror("ioperm"); exit(1);}
	if (ioperm(0x66, 1, 1)) {perror("ioperm"); exit(1);}
	
	signal (SIGINT, onintr);
	int dt;
	int pwm;
	int opwm=0;
	while (1) {
		write_ec(fan_control_reg,fan_manual_mode);
		dt=gettemp()-target_temp;
		if (dt < -6 ) {
			pwm=0;
		} else if (dt < -5) {
			pwm=10;
		} else if (dt < -3) {
			pwm=25;
		} else if (dt < -1) {
			pwm=62;
		} else if (dt < 2) {
			pwm=100;
		} else if (dt < 4) {
			pwm=150;
		} else {
			pwm=200;
		}
		opwm=(opwm+pwm)/2;
		speedSet(opwm);
		sleep(5);
	}
}


