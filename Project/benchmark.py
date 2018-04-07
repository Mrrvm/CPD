#!/usr/bin/python
import signal
import subprocess
import plotly.offline as offline
import plotly.graph_objs as go
import os.path

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
    "./testfiles/16x16.txt", "./testfiles/25x25.txt"
]

data = []

for filename in files:
    trace = go.Scatter(
        x=[],
        y=[],
        dx=1,
        mode='lines+markers',
        name=os.path.basename(os.path.normpath(filename)),
        hoverinfo='text+name',
        line=dict(shape='spline'))
    for thread_n in range(4):
        signal.alarm(3 * 60)
        try:
            result = test(filename, thread_n + 1)
            trace.x.append(thread_n + 1)
            trace.y.append(result)
        except TimeoutException:
            print(filename, "-", thread_n + 1, ":", "timed out")
            subprocess.run(["pkill", "kuduro-omp"])
            continue
        else:
            signal.alarm(0)
    if len(trace.x) > 1:
        data.append(trace)

layout = go.Layout(
    title='Kuduro, the Sudoku solver',
    xaxis=dict(
        title='Thread Count',
        titlefont=dict(
            family='Noto Sans, sans-serif', size=16, color='lightgrey'),
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
        titlefont=dict(
            family='Noto Sans, sans-serif', size=16, color='lightgrey'),
        showticklabels=True,
        autorange=True,
        showgrid=True,
        zeroline=True,
        showline=True,
        autotick=True,
        ticks=''))

fig = dict(data=data, layout=layout)

offline.plot(fig, image='png')
