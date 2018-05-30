#!/usr/bin/python
import os.path
import signal
import subprocess

import plotly.graph_objs as go
import plotly.offline as offline


class TimeoutException(Exception):  # Custom exception class
    pass


def timeout_handler(signum, frame):  # Custom signal handler
    raise TimeoutException


def test(exe, file, threads):
    result = subprocess.run(
        [
            'bash', '-c',
            exe + " " + file + " " + str(threads) + ' | ' + ' tail -1'
        ],
        stdout=subprocess.PIPE).stdout.decode('utf-8')
    return result


signal.signal(signal.SIGALRM, timeout_handler)

executables = ["./kuduro-adapta", "./kuduro-serial"]
files = [
    "./testfiles/4x4.txt", "./testfiles/9x9.txt", "./testfiles/9x9-nosol.txt",
    "./testfiles/25x25.txt"
]

data = []
thread_list = [1, 2, 4, 8]
thread_n = 4

for exe in executables:
    trace = go.Scatter(
        x=[],
        y=[],
        mode='markers',
        marker=dict(size=14),
        name=os.path.basename(os.path.normpath(exe)),
        hoverinfo='text+name')
    for filename in files:
        signal.alarm(1 * 60)
        try:
            result = test(exe, filename, thread_n)
            trace.x.append(
                os.path.splitext(os.path.basename(
                    os.path.normpath(filename)))[0])
            trace.y.append(result)
            print(exe, "-", filename, "-", thread_n, ":", result)
        except TimeoutException:
            print(exe, "-", filename, "-", thread_n, ":", "timed out")
            subprocess.run(["pkill", "kuduro-omp"])
            continue
        else:
            signal.alarm(0)
    if len(trace.x) > 1:
        data.append(trace)

layout = go.Layout(
    title='Kuduro - Serial vs OpenMP 4 Threads',
    xaxis=dict(
        title='File',
        titlefont=dict(family='Noto Sans, sans-serif', size=16, color='grey'),
        autotick=False,
        autorange=True,
        showgrid=True,
        zeroline=True,
        showline=True,
        ticks='',
        showticklabels=True),
    yaxis=dict(
        title='Time (s)',
        titlefont=dict(family='Noto Sans, sans-serif', size=16, color='grey'),
        showticklabels=True,
        autorange=True,
        showgrid=True,
        zeroline=True,
        showline=True,
        autotick=True,
        ticks=''))

fig = dict(data=data, layout=layout)
offline.plot(fig, image='png')
