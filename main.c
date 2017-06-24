#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/io.h>
#include <signal.h>

#define strengh 20
#define tsensorfile "/sys/class/thermal/thermal_zone1/temp"


#define fan_control_reg 0x93
#define fan_manual_mode 0x14
#define fan_auto_mode 0x04
#define fan_speed_reg 0x94
#define fan_rpm_reg 0x95



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

int main(int argc,char *argv[])
{
	if (ioperm(0x62, 1, 1)) {perror("ioperm"); exit(1);}
	if (ioperm(0x66, 1, 1)) {perror("ioperm"); exit(1);}
	
	int target_temp;
	sscanf(argv[1],"%d",&target_temp);
	if (argc==1)
		{
		printf("Usage:main [target temperature]");
//		exit(1);
		}
	printf("Target temperature: %d 'C\n",target_temp);

	signal (SIGINT, onintr);
	int gap_temp;
	int pwm;
	int oringinal_pwm=0;
	printf("Current  Gap  Rpm   Pre_Rpm\n");
	while (1) {
		write_ec(fan_control_reg,fan_manual_mode);
		gap_temp=gettemp()-target_temp;
			pwm=128+gap_temp*strengh;
		oringinal_pwm=(oringinal_pwm+pwm)/2;
		speedSet(pwm);
		printf("  %d     %d    %d0  %d0\n",gettemp(),gap_temp,pwm,oringinal_pwm);
		sleep(5);
	}
}


