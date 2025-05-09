파일 디스크립터 복제
![image](https://github.com/user-attachments/assets/4150d590-3515-41d7-b044-d5be6d0c8177)

## 파일 시스템 구조 
![image](https://github.com/user-attachments/assets/f85d6912-5146-49ed-a3c1-5c7293a31502)

- 부트 블록(Boot block)
  - 파일 시스템 시작부에 위치하고 보통 첫 번째 섹터를 차지
  - 부트스트랩 코드가 저장되는 블록

- 슈퍼 블록(Super block)
  - 전체 파일 시스템에 대한 정보를 저장
    - 총 블록 수, 사용 가능한 i-노드 개수, 사용 가능한 블록 비트 맵, 블록의 크기, 사용 중인 블록 수, 사용 가능한 블록 수 등

- i-리스트(i-list)
  - 각 파일을 나타내는 모든 i-노드들의 리스트
  - 한 블록은 약 40개 정도의 i-노드를 포함 

- 데이터 블록(Data block)
  - 파일의 내용(데이터)을 저장하기 위한 블록들





