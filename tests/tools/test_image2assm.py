#!/usr/bin/env python3
"""Round-trip test for tools/image2assm.py: image -> ASSM text -> words.

  test_image2assm.py                run the built-in cases
  test_image2assm.py image.bin ...  also round-trip real images (CTest passes
                                    the CROOK images the build made)
"""

import os
import random
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                "..", "..", "tools"))
import image2assm as i2a  # noqa: E402

REAL_IMAGES = []


def round_trip(words, **kw):
    text = i2a.image_to_assm(words, "TEST", **kw)
    header, back = i2a.parse_assm(text)
    return text, header, back


class RoundTrip(unittest.TestCase):
    def test_words(self):
        rnd = random.Random(400)
        words = [rnd.randrange(65536) for _ in range(1000)]
        words[10:30] = [0] * 20            # a long run of zeros -> RES*
        words[50:53] = [0] * 3             # a short run stays as words
        words += [0] * 5000                # trailing padding, like a real image
        text, header, back = round_trip(words, base=0o4000)
        self.assertEqual(words, back)
        self.assertIn("RES* 20.", text)
        self.assertEqual(
            dict(title="TEST", type=0, base=0o4000, attr=0o50, mem=0x1FFF),
            header)

    def test_extremes_and_no_res(self):
        words = [0, 1, 7, 8, 0o77777, 0o100000, 0o177777, 0o377, 0o177400]
        _, _, back = round_trip(words, min_run=10**9, words_per_line=3)
        self.assertEqual(words, back)

    def test_documented_forms(self):
        text, _, _ = round_trip([0o177400, 0o377, 0, 0o100000])
        self.assertIn("FILE*TEST,0,0,050.", text)
        self.assertIn("S* 0.", text)
        self.assertIn("0177400. 0377. 0. 0100000.", text)

    def test_bytes_and_options(self):
        data = bytes([0x12, 0x34, 0xFF, 0x00])
        words = i2a.read_words(data)
        self.assertEqual([0x1234, 0xFF00], words)
        text, header, back = round_trip(words, type_=2, attr=0o10, mem=0x3FFF,
                                        prog_block=True, eol="\r")
        self.assertEqual(data, i2a.words_to_bytes(back))
        self.assertEqual((2, 0o10, 0x3FFF),
                         (header["type"], header["attr"], header["mem"]))
        self.assertTrue(text.startswith("PROG*\r"))
        self.assertTrue(text.endswith("FINPROG*\r"))

    def test_errors(self):
        with self.assertRaises(ValueError):
            i2a.read_words(b"\x01")
        with self.assertRaises(ValueError):
            i2a.image_to_assm([0], "bad title")
        with self.assertRaises(ValueError):
            i2a.image_to_assm([0] * 10, "T", base=65530)
        with self.assertRaises(ValueError):
            i2a.parse_assm("FILE*T,0,0,050.\nS* 0.\n0400\n")

    def test_real_images(self):
        for path in REAL_IMAGES:
            with open(path, "rb") as f:
                data = f.read()
            words = i2a.read_words(data)
            _, _, back = round_trip(words)
            self.assertEqual(data, i2a.words_to_bytes(back), path)
            self.assertGreater(len(words), 0, path)


if __name__ == "__main__":
    REAL_IMAGES.extend(a for a in sys.argv[1:] if not a.startswith("-"))
    unittest.main(argv=[sys.argv[0]])
