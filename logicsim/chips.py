"""74LS 시리즈 논리 게이트 IC 모델.

IC 내부 트랜지스터를 계산하지 않고, 핀 배치·논리 기능·전파 지연만 모델링한다
(PSpice의 디지털 소자 모델과 같은 방식).
"""

from dataclasses import dataclass


@dataclass(frozen=True)
class Gate:
    inputs: tuple[int, ...]
    output: int


# 7408 / 7432 공통 핀 배치 (DIP-14, 위에서 본 기준)
#   1A=1 1B=2 1Y=3 | 2A=4 2B=5 2Y=6 | GND=7
#   3Y=8 3A=9 3B=10 | 4Y=11 4A=12 4B=13 | VCC=14
QUAD_2INPUT = (
    Gate((1, 2), 3),
    Gate((4, 5), 6),
    Gate((9, 10), 8),
    Gate((12, 13), 11),
)


# 7404 핀 배치 (DIP-14)
#   1A=1 1Y=2 | 2A=3 2Y=4 | 3A=5 3Y=6 | GND=7
#   4Y=8 4A=9 | 5Y=10 5A=11 | 6Y=12 6A=13 | VCC=14
HEX_INVERTER = (
    Gate((1,), 2),
    Gate((3,), 4),
    Gate((5,), 6),
    Gate((9,), 8),
    Gate((11,), 10),
    Gate((13,), 12),
)


def logic_not(values):
    """3값 논리 NOT. None은 알 수 없음(X)."""
    (v,) = values
    return None if v is None else 1 - v


def logic_and(values):
    """3값 논리 AND. None은 알 수 없음(X)."""
    if 0 in values:
        return 0
    if all(v == 1 for v in values):
        return 1
    return None


def logic_or(values):
    """3값 논리 OR. None은 알 수 없음(X)."""
    if 1 in values:
        return 1
    if all(v == 0 for v in values):
        return 0
    return None


class Chip:
    vcc_pin = 14
    gnd_pin = 7

    def __init__(self, ref, part, func, gates, tplh_ns, tphl_ns):
        self.ref = ref
        self.part = part
        self.func = func
        self.gates = gates
        self.tplh_ns = tplh_ns
        self.tphl_ns = tphl_ns
        self.pins = {}  # 핀 번호 -> Net

    def delay(self, old, new):
        if old == 0 and new == 1:
            return self.tplh_ns
        if old == 1 and new == 0:
            return self.tphl_ns
        return max(self.tplh_ns, self.tphl_ns)

    def __repr__(self):
        return f"{self.ref}({self.part})"


# 지연 시간은 TI 데이터시트의 대표값(typ, VCC=5V, CL=15pF) 기준 근사치
def ls04(ref):
    """74LS04: NOT 게이트 6개."""
    return Chip(ref, "74LS04", logic_not, HEX_INVERTER, tplh_ns=9, tphl_ns=10)


def ls08(ref):
    """74LS08: 2입력 AND 게이트 4개."""
    return Chip(ref, "74LS08", logic_and, QUAD_2INPUT, tplh_ns=8, tphl_ns=10)


def ls32(ref):
    """74LS32: 2입력 OR 게이트 4개."""
    return Chip(ref, "74LS32", logic_or, QUAD_2INPUT, tplh_ns=14, tphl_ns=14)
