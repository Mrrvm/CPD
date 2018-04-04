# CPD

Parallel and Distributed Computing course

## REPORT

[CPD17/18\\ Solving Sudoku through Serial and OpenMP implementation](https://www.sharelatex.com/6778116471jnthgqydgnhm)

## GOAL

We have to find a way to divide the tree search that, at the
same time, allows each thread to have some independence in its search (tasks are
divided and it does not have to access a shared portion all the time), allows
a given task to be shared dynamically and guarantees synchronization
(each task is not repeated) without too much overhead.

I think the best solution is some compromise between these three premises
(the program can't be too much invested in one without affecting the
others).

## STEPS

1.  The problem is divided int P^N independent tasks. A thread reserver a task to
    itself (a node and all subtrees). When it finishes it, it fetches another task,
    if any is available. If none it becomes IDLE until the problem is solved.

2.  If one thread, still working, notices all other threads are IDLE, it
    redistributes its work as new tasks. When does it check? Too much overhead if
    done all the time.

3.  A thread, when if finds it has no more tasks, takes the initiative to find
    more work and snatches it from another active thread, taking care not to
    interfere in its work and to change its state correctly. (a lot of
    synchronization issues! In which situation can it snatch work???)
