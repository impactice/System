# include <stdio.h> 

int main(){
    char in; 
    printf("문자 입력:");
    scanf("%c",&in);
    if(in >= 'A' && in <= 'Z') {
        printf("%c의 소문자는 %c입니다.",in,in+32);
    } else if (in >= 'a' && in <= 'z') {
        printf("%c의 대문자는 %c입니다.",in, in-32);
    } else if(in==0){
        return 0;
    } else {
        
    }
    
}