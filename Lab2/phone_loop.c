#include<stdio.h>
#include<stdlib.h>

int main(){
	int digit;
	char phone_number[11];
	int err = 0;
	
	scanf("%s", phone_number);
	
	while(scanf("%d", &digit)!= EOF){
		if(digit < -1 || digit > 9){
			printf("ERROR\n");
			err++;
		}
		else {
			if(digit == -1){
				printf("%s\n", phone_number);
			}else{
				printf("%c\n", phone_number[digit]);
			}
		}
	}
	if(err == 0){
		return 0;
	}else{
		return 1;
	}
}
