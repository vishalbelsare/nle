import contextlib
import termios
import timeit
import random
import sys

from nle import nethack


NO_SELF_PLAY = 2


@contextlib.contextmanager
def no_echo(fd=0):
    old = termios.tcgetattr(fd)
    try:
        new = termios.tcgetattr(fd)
        new[3] &= ~termios.ICANON & ~termios.ECHO  # lflags
        termios.tcsetattr(fd, termios.TCSAFLUSH, new)
        yield
    finally:
        termios.tcsetattr(fd, termios.TCSAFLUSH, old)


def main():
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

    nle = nethack.Nethack(observation_keys=("chars", "blstats"))
    nle.reset()

    nle.step(ord("y"))
    nle.step(ord("y"))
    nle.step(ord("\n"))

    steps = 0
    start_time = timeit.default_timer()
    start_steps = steps

    mean_sps = 0
    sps_n = 0

    for episode in range(0):
        while True:
            ch = random.choice(ACTIONS)
            _, done = nle.step(ch)
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
        nle.reset()

    print("Finished after %i steps. Mean sps: %f" % (steps, mean_sps))

    for _ in range(NO_SELF_PLAY):
        nle.reset()
        done = False
        while not done:
            try:
                with no_echo():
                    (chars, blstats), done = nle.step(ord(sys.stdin.read(1)))
            except KeyboardInterrupt:
                break
            for line in chars:
                print(line.tobytes().decode("utf-8"))
            print(blstats)


main()
