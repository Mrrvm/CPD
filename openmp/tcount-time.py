#!/usr/bin/python
import os.path
import signal
import subprocess

import plotly.graph_objs as go
import plotly.offline as offline

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
    return result


signal.signal(signal.SIGALRM, timeout_handler)
files = [
    "./testfiles/4x4.txt", "./testfiles/9x9.txt", "./testfiles/9x9-nosol.txt",
    "./testfiles/25x25.txt"
]

data = []
thread_list = [1, 2, 4, 8]

for filename in files:
    trace = go.Scatter(
        x=[],
        y=[],
        dx=1,
        mode='lines+markers',
        name=os.path.splitext(os.path.basename(os.path.normpath(filename)))[0],
        hoverinfo='text+name',
        line=dict(shape='spline'))
    for thread_n in thread_list:
        signal.alarm(1 * 60)
        try:
            result = test(filename, thread_n)
            trace.x.append(thread_n)
            trace.y.append(result)
        except TimeoutException:
            print(filename, "-", thread_n, ":", "timed out")
            subprocess.run(["pkill", "kuduro-omp"])
            continue
        else:
            signal.alarm(0)
    if len(trace.x) > 1:
        data.append(trace)

layout = go.Layout(
    title='Kuduro - Threads/Execution Time',
    xaxis=dict(
        title='Thread Count',
        titlefont=dict(family='Noto Sans, sans-serif', size=16, color='grey'),
        tick0=1,
        dtick=1,
        autotick=False,
        autorange=True,
        showgrid=True,
        zeroline=True,
        showline=True,
        ticks='',
        showticklabels=True),
    yaxis=dict(
        title='Execution Time (s)',
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
