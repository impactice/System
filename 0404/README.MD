# 생성형 AI 

## 세상을 바꾸는 기술 
- 챗GPT로 대변되는 생성형 AI는 '인터넷', 또는 모바일 혁명을 일이킨 '아이폰'에 비견되는 혁신으로 평가 받고 있음 

### 업계 리더가 말하는 생성형 AI 
- "사용자의 질문에 놀라울 정도로 인간과 비슷하게 답하는 챗봇인 챗GPT가 인터넷만큼 중대한 발명이라고 믿는다." -빌 게이츠(Microsoft 창업자) 

- "인공지능(AI)의 아이폰 시대가 시작됐습니다. 생성형 AI가 모든 산업을 재창조할 것입니다." -젠슨 황(Nvidia 창업자)

### 생성형 AI가 주목받는 이유 
기술 측면 
1. 인간의 고유 영역에 도전하는 기술의 혁신 - 단순 데이터 인식을 넘어 '창조'의 영역에 진입한 생성형 AI 기술 
2. 가속화된 생성형 AI 기술의 진보 속도 - GPT3에서 3.5까지 2년, 3.5에서 4까지 4개월로 가속화된 업그레이드 GPT AI 모델 출시 주기# 생성형 AI 

## 세상을 바꾸는 기술 
- 챗GPT로 대변되는 생성형 AI는 '인터넷', 또는 모바일 혁명을 일이킨 '아이폰'에 비견되는 혁신으로 평가 받고 있음 

### 업계 리더가 말하는 생성형 AI 
- "사용자의 질문에 놀라울 정도로 인간과 비슷하게 답하는 챗봇인 챗GPT가 인터넷만큼 중대한 발명이라고 믿는다." -빌 게이츠(Microsoft 창업자) 

- "인공지능(AI)의 아이폰 시대가 시작됐습니다. 생성형 AI가 모든 산업을 재창조할 것입니다." -젠슨 황(Nvidia 창업자)

### 생성형 AI가 주목받는 이유 
기술 측면 
- 인간의 고유 영역에 도전하는 기술의 혁신 - 단순 데이터 인식을 넘어 '창조'의 영역에 진입한 생성형 AI 기술 
- 가속화된 생성형 AI 기술의 진보 속도 - GPT3에서 3.5까지 2년, 3.5에서 4까지 4개월로 가속화된 업그레이드 GPT AI 모델 출시 주기

산업 측면 
- 'AI의 산업화 시대'를 이글어 갈 서비스 생태계 활성화 - 대형 언어 모델을 기반으로 한 수 많은 AI애플리케이션과 서비스의 출시 가속 

## 개인에서 기업, 이제는 산업으로 
- 개인에게 높은 침투율을 보인 챗GPT, 산업 내에서도 빠르게 확산되고 있음 
- MS등 주요 솔루션 업체는 자체 기업 서비스에 챗GPT를 적극적으로 연동하며 기업 성장의 신규 동력으로 활용하고 있음 

![Image](https://github.com/user-attachments/assets/74d77786-99ac-4e25-9dda-30e9be964eca) 

## 인공지능의 역사 
- 초기 인공지능 연구는 체스, 수학 문제 풀이 등 특정 작업을 수행하기 위한 간단한 알고리즘과 모델 개방 중심 
- 점차 AI가 정교해지면서 딥 러닝 기술과 대량의 데이터 통합을 통해 더 복잡한 문제 해결 가능해짐 
- 자연어 처리 및 컴퓨터 비전 등의 영역에서 괄목한 만한 발전 달성 

![Image](https://github.com/user-attachments/assets/cbfc03d0-0c4e-41cb-bdd6-cb3993b17c79) 

## 제너레이티브 AI의 경제적 잠재력  
![Image](https://github.com/user-attachments/assets/76c21d66-e074-4368-b3fc-95d9907fd936) 

## AI 상용화 원년 밝았다...DX를 넘어 AX로 
**인공지능을 기업의 비즈니스에 도입하기 위한 구체적인 가이드라인과 방법론들이 나오고 있음** 
![Image](https://github.com/user-attachments/assets/d5684f67-ae6d-4c0d-a772-3d6aa68b85f6) 

![Image](https://github.com/user-attachments/assets/de28d4b5-252e-4ba9-8870-05f378ed40f4)

## 복잡한 입찰안내서, AI가 분석해 준다고? 
![Image](https://github.com/user-attachments/assets/5618266f-427b-483a-9506-08bafa83d8f3) 

![Image](https://github.com/user-attachments/assets/7641a858-4215-4f9c-8652-ba1a5ab38aee) 



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
