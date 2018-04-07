#!/usr/bin/python
import signal
import subprocess

executable = "./kuduro-omp"


class TimeoutException(Exception):  # Custom exception class
    pass


def timeout_handler(signum, frame):  # Custom signal handler
    raise TimeoutException


def test(file, threads):
    result = subprocess.run(
        [
            'bash', '-c',
            executable + " " + file + " " + str(threads) + ' | ' + ' tail -1'
        ],
        stdout=subprocess.PIPE).stdout.decode('utf-8')
    print(file, "-", threads, ":", result)


signal.signal(signal.SIGALRM, timeout_handler)
li = [
    "./testfiles/4x4.txt", "./testfiles/9x9.txt", "./testfiles/9x9-nosol.txt",
    "./testfiles/16x16.txt", "./testfiles/25x25.txt"
]

for thread_n in range(4):
    for filename in li:
        signal.alarm(3 * 1)
        try:
            test(filename, thread_n + 1)
        except TimeoutException:
            print(filename, "-", thread_n + 1, ":", "timed out")
            subprocess.run(["pkill", "kuduro-omp"])
            continue
        else:
            signal.alarm(0)
