import itertools
import unittest

from examples.and_or_f import build, critical_path_run, truth_table
from logicsim import Circuit, CircuitError, ls08


class AndOrFTest(unittest.TestCase):
    def test_truth_table_matches_expression(self):
        for a, b, c, f in truth_table():
            self.assertEqual(f, int((a or b) and a and c), (a, b, c))

    def test_equals_simplified_ac(self):
        for a, b, c, f in truth_table():
            self.assertEqual(f, a & c, (a, b, c))

    def test_critical_path_delay(self):
        _, delay = critical_path_run()
        self.assertEqual(delay, 14 + 8 + 8)

    def test_no_floating_inputs(self):
        self.assertEqual(build().check(), [])


class CircuitRulesTest(unittest.TestCase):
    def test_missing_power_is_error(self):
        c = Circuit()
        u = c.add(ls08("U1"))
        c.connect(u, 7, "GND")
        c.connect(u, 3, "Y")
        with self.assertRaises(CircuitError):
            c.run()

    def test_floating_ttl_input_reads_high(self):
        c = Circuit()
        u = c.add(ls08("U1"))
        c.connect(u, 14, "VCC")
        c.connect(u, 7, "GND")
        c.connect(u, 1, "A")
        c.connect(u, 3, "Y")
        self.assertEqual(len(c.check()), 1)
        c.set_input("A", 1, 0)
        c.run()
        self.assertEqual(c.nets["Y"].value, 1)

    def test_two_outputs_on_one_net_is_error(self):
        c = Circuit()
        u = c.add(ls08("U1"))
        c.connect(u, 3, "Y")
        with self.assertRaises(CircuitError):
            c.connect(u, 6, "Y")


if __name__ == "__main__":
    unittest.main()
