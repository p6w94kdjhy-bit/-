import unittest

from examples.lab_a_plus_not_b import build, glitch_widths, lab_run, truth_table


class LabAPlusNotBTest(unittest.TestCase):
    def test_both_circuits_equal_a_or_not_b(self):
        for a, b, y1, y2 in truth_table():
            self.assertEqual(y1, a | (1 - b), (a, b))
            self.assertEqual(y2, y1, (a, b))

    def test_ideal_waveforms_identical(self):
        c = lab_run(ideal=True)
        settled = lambda h: [(t, v) for (t, v), nxt in zip(h, h[1:] + [(None, None)]) if nxt[0] != t]
        self.assertEqual(settled(c.nets["Y1"].history), settled(c.nets["Y2"].history))

    def test_real_delay_shows_glitches(self):
        c = lab_run(ideal=False)
        self.assertIn(21, glitch_widths(c.nets["Y1"].history))
        self.assertIn(9, glitch_widths(c.nets["Y2"].history))

    def test_no_floating_inputs(self):
        self.assertEqual(build().check(), [])


if __name__ == "__main__":
    unittest.main()
