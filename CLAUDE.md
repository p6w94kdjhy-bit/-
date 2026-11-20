# 한국사 강의 정리 작업 규칙

사용자는 한국사 수업의 **교재 PDF(장별 슬라이드)**와 **강의 녹음(m4a)**을 올린다. 결과물은 모두 `한국사_강의노트/`에 둔다.

## 1. 교재 + 녹음을 받으면: 그 장만 "정리"
사용자 요청(2026-11): "교재랑 녹음파일 주면 그거 정리만 해줘."
- 그 장의 정리본만 만든다. 전체 통합 PDF는 만들지 않는다.
- 정리 방식은 16장처럼 한다. 녹음을 교재와 비교해서 아래 세 가지만 뽑는다.
  1. **시험 포인트 표**: 교수님이 "중요", "핵심", "결정적" 등으로 강조한 곳은 ⭐와 타임스탬프 `[장 hh:mm:ss]`
  2. **[강의 추가]**: 교재에 없고 강의에서만 나온 내용
  3. **⚠️ 강의와 교재가 다른 곳**: 교재 기준으로 안내, 불확실하면 [?]
- 저장할 것
  - `_work/chapters/notesNN.md` (장 정리)
  - `_work/NN.txt` (받아쓰기 원본)
  - `chNN_정리.pdf` (그 장만의 PDF): 사용자에게 보내고 커밋·푸시
- 그 장의 핵심 요약 10줄 이내를 노트 끝에 붙인다.

## 2. "기말자료 정리해줘"라고 하면: 총통합 PDF
- `_work/chapters/` 의 모든 장을 교재 목차(장 번호) 순서로 합친다.
- 맨 앞에 범위·표기 안내, 맨 끝에 **전체 시험 대비 핵심 요약**(장별 핵심을 통합)을 넣는다.
- 결과
  - `기말자료_통합.pdf` (+ `.md`)
  - 원하면 교정 대본과 [?] 목록 통합본도 만든다.
- 기존 `lecture_notes.md/pdf`(12·13·15·16장)도 통합 대상에 포함한다. 12·13·15장은 `_work/chapters/notes12a.md`, `notes12b.md`, `notes13.md`, `notes15.md` 에 있다.

## 받아쓰기 절차 (`_scripts/`)
- faster-whisper large-v3, `language="ko"`. GPU가 없으면 CPU int8을 쓴다. 55분 녹음이 약 40분 걸린다. 백그라운드로 돌리고, 2시간 넘는 작업은 나눠서 돌린다.
- `initial_prompt`: 교재 목차와 핵심 용어로 223토큰 이하. 예시는 `_work/prompts/NN.txt`.
- `python3 _scripts/transcribe.py <audio> <out.txt> <prompt.txt>`: ffmpeg로 디코딩한다. PyAV 19와 호환되지 않아서다. VAD는 켠다.
- 녹음 앞뒤의 음량을 `ffmpeg volumedetect`로 먼저 확인한다. -34dB보다 작은 구간은 VAD가 무시하므로 `transcribe2.py <audio> <out> <prompt> <start_sec>`(음량 정규화, VAD 끔)로 다시 받아쓴다.
- 끝나면 타임스탬프 사이에 30초 넘게 빈 곳이 있는지 확인한다.
- 모델 다운로드에는 huggingface.co와 us.aws.cdn.hf.co 접근이 필요하다. 환경 허용 목록에 추가해 둠.

## PDF 만들기
- `python3 _scripts/topdf.py <src.md|.txt> <out.pdf> "<제목>"`
- 필요한 것: markdown 모듈, Node Playwright, 나눔 글꼴(`apt-get install fonts-nanum`)과 이모지 글꼴(`fonts-noto-color-emoji`).
