#include<stdio.h>
#include<stdlib.h>

int main(){
	int digit;
	char phone_number[11];
	
	scanf("%s %d", phone_number, &digit);
	
	if(digit < -1 || digit > 9){
		printf("ERROR\n");
		return 1;
	}

	if(digit == -1){
		printf("%s\n", phone_number);
	}else{
		printf("%c\n", phone_number[digit]);
	}
	return 0;
}
