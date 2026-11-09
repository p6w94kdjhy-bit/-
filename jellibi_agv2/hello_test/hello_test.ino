// 연결 테스트: GitHub → 노트북 → 로봇 업로드 확인용 (모터는 움직이지 않음)
//  - 켜지면 부저(D3) 도-미-솔
//  - 시리얼 모니터(115200)에 1초마다 메시지 + 센서값
//  - 보드 LED(D13) 1초 깜빡임
//  - 버튼(A3)을 누르면 '삑'
#define VERSION "hello_test v1"

void setup()
{
    Serial.begin(115200);
    pinMode(13, OUTPUT);
    pinMode(A3, INPUT);
    tone(3, 523, 150); delay(180);
    tone(3, 659, 150); delay(180);
    tone(3, 784, 250); delay(300);
    Serial.println(F("=== " VERSION " : GitHub에서 받은 코드가 업로드됨 ==="));
}

void loop()
{
    static unsigned long last = 0;
    if (millis() - last >= 1000) {
        last = millis();
        digitalWrite(13, !digitalRead(13));
        Serial.print(F(VERSION "  L(A6)="));
        Serial.print(analogRead(A6));
        Serial.print(F("  R(A7)="));
        Serial.print(analogRead(A7));
        Serial.print(F("  F(A0)="));
        Serial.println(analogRead(A0));
    }
    if (digitalRead(A3) == LOW) {
        tone(3, 1047, 80);
        Serial.println(F("button!"));
        delay(250);
    }
}
