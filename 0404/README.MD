

```
#include <stdio.h>

int main() {
    int num;
    int binary[8];
    int i, count = 0;
    
    // 0부터 255 사이의 숫자 입력 받기
    printf("0부터 255 사이의 숫자를 입력하세요: ");
    scanf("%d", &num);
    
    // 입력 범위 확인
    if (num < 0 || num > 255) {
        printf("입력 오류: 숫자는 0부터 255 사이여야 합니다.\n");
        return 1;
    }
    
    // 2진수로 변환하여 배열에 저장 (8비트)
    for (i = 7; i >= 0; i--) {
        binary[i] = num % 2;
        num /= 2;
    }
    
    // 전체 8비트 이진수 출력
    printf("8비트 이진수: ");
    for (i = 0; i < 8; i++) {
        printf("%d", binary[i]);
        if (binary[i] == 1) {
            count++;  // 1의 개수 세기
        }
    }
    printf("\n");
    
    // 1의 개수 출력
    printf("1의 개수: %d\n", count);
    
    // 상위 4비트 출력
    printf("상위 4비트: ");
    for (i = 0; i < 4; i++) {
        printf("%d", binary[i]);
    }
    printf("\n");
    
    return 0;
} 
```
