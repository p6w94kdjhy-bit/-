// =====================================================================
//  IR 현장 측정 (시리얼 모니터 115200, '새 줄' 무관)
//   계속 출력: L(A6) R(A7) F(A0) 원시값
//   명령
//     w : 흰 바닥 위에 두 센서를 두고 → 흰색 기록
//     b : 로봇을 선과 '직각'으로 놓아 두 센서가 모두 선 정중앙 위 → 검은색 기록
//     c : 로봇을 선 방향으로, 선 한가운데 놓고 → 주행 중 상태 확인 (교차점 오검출 경고, LT_CENTER_OFFSET)
//     f : 장애물을 '멈추고 싶은 거리'에 두고 → 전방 기준값 기록
//     p : config.h 에 붙여 넣을 줄 출력
//     m : 연속 출력 켜기/끄기
//  측정은 대회장 조명에서, 맵의 여러 위치에서 반복 확인. 파란 통로를 따라가면 파란 선 위에서 측정.
// =====================================================================
#include <AGV2.h>

int wL = -1, wR = -1, bL = -1, bR = -1, cL = -1, cR = -1, fTh = -1;
bool monitor = true;

void avg3(int& l, int& r, int& f)
{
    long sl = 0, sr = 0, sf = 0;
    const int N = 64;
    for (int i = 0; i < N; i++) {
        sl += analogRead(PIN_IR_L);
        sr += analogRead(PIN_IR_R);
        sf += analogRead(PIN_IR_FRONT);
        delay(3);
    }
    l = sl / N; r = sr / N; f = sf / N;
}

void printLine(const __FlashStringHelper* name, int v, const __FlashStringHelper* note)
{
    Serial.print(F("#define "));
    Serial.print(name);
    Serial.print(' ');
    Serial.print(v);
    Serial.print(F("   // "));
    Serial.println(note);
}

void printConfig()
{
    Serial.println(F("----- config.h [1][2] -----"));
    printLine(F("IR_L_WHITE  "), wL, F("현장값"));
    printLine(F("IR_L_BLACK  "), bL, F("현장값"));
    printLine(F("IR_R_WHITE  "), wR, F("현장값"));
    printLine(F("IR_R_BLACK  "), bR, F("현장값"));
    if (fTh >= 0) printLine(F("FRONT_OBSTACLE_TH"), fTh, F("현장값"));
    if (wL >= 0 && bL >= 0 && wR >= 0 && bR >= 0) {
        int thL = (wL + bL) / 2, thR = (wR + bR) / 2;
        Serial.print(F("// 기준값 L=")); Serial.print(thL);
        Serial.print(F(" R=")); Serial.println(thR);
        if (bL - wL < 150) Serial.println(F("// 경고: 왼쪽(L) 흑백 차이가 작음 — b 때 왼쪽 센서가 선 위였는지 확인"));
        if (bR - wR < 150) Serial.println(F("// 경고: 오른쪽(R) 흑백 차이가 작음 — b 때 오른쪽 센서가 선 위였는지 확인"));
        if (cL > bL || cR > bR) {
            Serial.println(F("// 경고: c 값이 b(검정)보다 큼 → b 측정 때 센서가 선 위에 없었음. b 다시 측정"));
            return;
        }
        if (cL >= 0) {
            // 선 중앙 주행 상태에서 두 센서의 위치(%)
            int pL = (long)(cL - wL) * 100 / max(1, bL - wL);
            int pR = (long)(cR - wR) * 100 / max(1, bR - wR);
            int need = max(pL, pR) + 15;
            Serial.print(F("// 선 중앙 주행 시 L=")); Serial.print(pL);
            Serial.print(F("% R=")); Serial.print(pR); Serial.println('%');
            if (need > 50) {
                Serial.print(F("// 경고: 주행 중에도 교차점 오검출 가능 → XING_LEVEL_PCT "));
                Serial.println(min(need, 85));
            }
            int nL = (long)(cL - wL) * 1000 / max(1, bL - wL);
            int nR = (long)(cR - wR) * 1000 / max(1, bR - wR);
            printLine(F("LT_CENTER_OFFSET"), nL - nR, F("선 중앙에서의 err"));
        }
    }
}

void setup()
{
    liftInit();                      // 맨 먼저 리프터 내리기 (교본 26쪽: 전원을 켤 때 리프터가 올라가는 현상 처리)
    Serial.begin(115200);
    driveInit();
    uiInit();
    Stop();
    Serial.println(F("calib_ir: w b c f p m"));
}   

void loop()
{
    if (Serial.available()) {
        char c = Serial.read();
        int l, r, f;
        switch (c) {
            case 'w': avg3(l, r, f); wL = l; wR = r; Serial.print(F("WHITE L=")); Serial.print(l); Serial.print(F(" R=")); Serial.println(r); beep(1047, 60); break;
            case 'b': avg3(l, r, f); bL = l; bR = r; Serial.print(F("BLACK L=")); Serial.print(l); Serial.print(F(" R=")); Serial.println(r); beep(1047, 60); break;
            case 'c': avg3(l, r, f); cL = l; cR = r; Serial.print(F("CENTER L=")); Serial.print(l); Serial.print(F(" R=")); Serial.println(r); beep(1047, 60); break;
            case 'f': avg3(l, r, f); fTh = f; Serial.print(F("FRONT=")); Serial.println(f); beep(1047, 60); break;
            case 'p': printConfig(); break;
            case 'm': monitor = !monitor; break;
        }
    }
    if (monitor) {
        static unsigned long last = 0;
        if (millis() - last >= 100) {
            last = millis();
            IrSample s = readIr();
            Serial.print(F("L=")); Serial.print(s.l);
            Serial.print(F(" R=")); Serial.print(s.r);
            Serial.print(F(" F=")); Serial.print(irFront());
            if (irCalibrated()) {         // config.h 값이 들어가 있으면 판정도 표시
                Serial.print(F("  ["));
                Serial.print(isBlackL(s.l) ? 'B' : 'w');
                Serial.print(isBlackR(s.r) ? 'B' : 'w');
                Serial.print(F("] err=")); Serial.print(lineError(s));
            }
            Serial.println();
        }
    }
}
