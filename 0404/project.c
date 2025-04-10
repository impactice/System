#include <stdio.h>

int main() {
    int num;
    
    // 사용자로부터 0~255 사이의 숫자 입력 받기
    printf("0에서 255 사이의 정수를 입력하세요: ");
    scanf("%d", &num);
    
    // 입력값이 범위를 벗어나는지 확인
    if (num < 0 || num > 255) {
        printf("오류: 입력값은 0에서 255 사이여야 합니다.\n");
        return 1;
    }
    
    // 입력된 수의 2진수 표현 출력
    printf("%d의 2진수 표현: ", num);
    
    // 8비트로 표현 (왼쪽에서 오른쪽으로 출력)
    for (int i = 7; i >= 0; i--) {
        // 비트 연산을 사용하여 각 비트 확인
        int bit = (num >> i) & 1;
        printf("%d", bit);
    }
    printf("\n");
    
    // 상위 4비트 출력
    printf("상위 4비트: ");
    for (int i = 7; i >= 4; i--) {
        int bit = (num >> i) & 1;
        printf("%d", bit);
    }
    printf("\n");
    
    return 0;
}
