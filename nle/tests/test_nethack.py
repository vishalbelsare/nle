import timeit
import random
import warnings

import numpy as np

import pytest

from nle import nethack


# MORE + compass directions + long compass directions.
ACTIONS = [
    13,
    107,
    108,
    106,
    104,
    117,
    110,
    98,
    121,
    75,
    76,
    74,
    72,
    85,
    78,
    66,
    89,
]


class TestNetHack:
    def test_run_n_episodes(self, tmpdir, episodes=3):
        pytest.skip("Not ready yet")

        tmpdir.chdir()

        game = nethack.Nethack(observation_keys=("chars", "blstats"))
        chars, blstats = game.reset()

        assert chars.shape == (21, 79)
        assert blstats.shape == (23,)

        game.step(ord("y"))
        game.step(ord("y"))
        game.step(ord("\n"))

        steps = 0
        start_time = timeit.default_timer()
        start_steps = steps

        mean_sps = 0
        sps_n = 0

        for episode in range(episodes):
            while True:
                ch = random.choice(ACTIONS)
                _, done = game.step(ch)
                if done:
                    break

                steps += 1

                if steps % 1000 == 0:
                    end_time = timeit.default_timer()
                    sps = (steps - start_steps) / (end_time - start_time)
                    sps_n += 1
                    mean_sps += (sps - mean_sps) / sps_n
                    print("%f SPS" % sps)
                    start_time = end_time
                    start_steps = steps
            print("Finished episode %i after %i steps." % (episode + 1, steps))
            game.reset()

        print("Finished after %i steps. Mean sps: %f" % (steps, mean_sps))

        nethackdir = tmpdir.chdir()
        assert nethackdir.fnmatch("*nethackdir")
        assert tmpdir.ensure("nle.ttyrec")

        assert mean_sps > 10000

        if mean_sps < 15000:
            warnings.warn("Mean sps was only %f" % mean_sps)


class TestNetHackOld:
    def test_run(self):
        # TODO: Implement ttyrecording filename in libnethack wrapper.
        # archivefile = tempfile.mktemp(suffix="nethack_test", prefix=".zip")

        game = nethack.Nethack()
        obs = game.reset()
        actions = [
            nethack.MiscAction.MORE,
            nethack.MiscAction.MORE,
            nethack.MiscAction.MORE,
            nethack.MiscAction.MORE,
            nethack.MiscAction.MORE,
            nethack.MiscAction.MORE,
        ]

        for action in actions:
            # TODO: Implement programstate observation.
            # while not response.ProgramState().InMoveloop():
            for _ in range(5):
                print(_)
                obs, done = game.step(nethack.MiscAction.MORE)

            obs, done = game.step(action)
            if done:
                # Only the good die young.
                obs = game.reset()

            glyphs, chars, _, _, blstats = obs

            x, y = blstats[:2]
            print(x, y)

            assert np.count_nonzero(chars == ord("@")) == 1
            assert chars[y, x] == ord("@")

            # # TODO: Re-add permonst, etc.
            # mon = nethack.permonst(nethack.glyph_to_mon(glyphs[y][x]))
            # self.assertEqual(mon.mname, "monk")
            # self.assertEqual(mon.mlevel, 10)

            # class_sym = nethack.class_sym.from_mlet(mon.mlet)
            # self.assertEqual(class_sym.sym, "@")
            # self.assertEqual(class_sym.explain, "human or elf")


class TestNethackFunctionsAndConstants:
    def test_permonst_and_class_sym(self):
        pytest.skip("Not ready yet")
        glyph = 155  # Lichen.

        mon = nethack.permonst(nethack.glyph_to_mon(glyph))

        self.assertEqual(mon.mname, "lichen")

        cs = nethack.class_sym.from_mlet(mon.mlet)

        self.assertEqual(cs.sym, "F")
        self.assertEqual(cs.explain, "fungus or mold")

        self.assertEqual(nethack.NHW_MESSAGE, 1)
        self.assertTrue(hasattr(nethack, "MAXWIN"))

    def test_permonst(self):
        pytest.skip("Not ready yet")
        mon = nethack.permonst(0)
        self.assertEqual(mon.mname, "giant ant")
        del mon

        mon = nethack.permonst(1)
        self.assertEqual(mon.mname, "killer bee")

    def test_some_constants(self):
        assert nethack.GLYPH_MON_OFF == 0
        assert nethack.NUMMONS > 300
