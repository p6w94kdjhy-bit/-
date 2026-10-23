"""이벤트 기반 디지털 회로 시뮬레이터."""

import heapq
import itertools


class Net:
    def __init__(self, name):
        self.name = name
        self.value = None  # None = 알 수 없음(X)
        self.history = []  # (시각 ns, 값)
        self.readers = []  # 이 넷을 입력으로 쓰는 (Chip, Gate)
        self.driver = None  # 이 넷을 출력으로 구동하는 (Chip, Gate)

    def __repr__(self):
        return f"Net({self.name}={self.value})"


class CircuitError(Exception):
    pass


class Circuit:
    def __init__(self):
        self.nets = {}
        self.chips = []
        self.vcc = self.net("VCC")
        self.gnd = self.net("GND")
        self.time = 0
        self._queue = []
        self._seq = itertools.count()
        self._projected = {}  # 게이트별로 마지막에 예약된 출력값

    def net(self, name):
        if name not in self.nets:
            self.nets[name] = Net(name)
        return self.nets[name]

    def add(self, chip):
        self.chips.append(chip)
        return chip

    def connect(self, chip, pin, net_name):
        net = self.net(net_name)
        chip.pins[pin] = net
        for gate in chip.gates:
            if pin in gate.inputs:
                net.readers.append((chip, gate))
            if pin == gate.output:
                if net.driver is not None:
                    raise CircuitError(
                        f"{net_name}: 출력 {net.driver[0]}와 {chip}가 한 넷에 같이 물려 있음"
                    )
                net.driver = (chip, gate)

    def check(self):
        """전원 배선을 검사하고, 떠 있는 입력 핀 경고 목록을 돌려준다."""
        for chip in self.chips:
            if chip.pins.get(chip.vcc_pin) is not self.vcc:
                raise CircuitError(f"{chip} {chip.vcc_pin}번 핀(VCC)이 VCC에 연결되지 않음")
            if chip.pins.get(chip.gnd_pin) is not self.gnd:
                raise CircuitError(f"{chip} {chip.gnd_pin}번 핀(GND)이 GND에 연결되지 않음")

        warnings = []
        for chip in self.chips:
            for gate in chip.gates:
                if gate.output not in chip.pins:
                    continue  # 안 쓰는 게이트
                for pin in gate.inputs:
                    if pin not in chip.pins:
                        warnings.append(
                            f"{chip} {pin}번 입력이 떠 있음 (TTL은 HIGH로 읽음)"
                        )
        return warnings

    def set_input(self, net_name, value, at_ns):
        self._schedule(at_ns, self.net(net_name), value)

    def _schedule(self, t, net, value):
        heapq.heappush(self._queue, (t, next(self._seq), net, value))

    def _read(self, chip, pin):
        net = chip.pins.get(pin)
        if net is None:
            return 1  # TTL 입력이 떠 있으면 HIGH
        return net.value

    def _evaluate(self, chip, gate):
        out_net = chip.pins.get(gate.output)
        if out_net is None:
            return
        new = chip.func([self._read(chip, p) for p in gate.inputs])
        key = (id(chip), gate)
        old = self._projected.get(key, out_net.value)
        if new == old:
            return
        self._projected[key] = new
        self._schedule(self.time + chip.delay(old, new), out_net, new)

    def run(self, until_ns=None):
        """이벤트가 없어질 때까지(또는 until_ns까지) 시뮬레이션한다."""
        self.check()
        while self._queue:
            t = self._queue[0][0]
            if until_ns is not None and t > until_ns:
                break
            _, _, net, value = heapq.heappop(self._queue)
            self.time = t
            if net.value == value:
                continue
            net.value = value
            net.history.append((t, value))
            for chip, gate in net.readers:
                self._evaluate(chip, gate)
        if until_ns is not None:
            self.time = max(self.time, until_ns)
        return self.time
