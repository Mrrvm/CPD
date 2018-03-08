# 1st Exercise Class

## Ex 1

```
Fa, Fb, Fc - fraction of time spent in mode A/B/C
Fa + Fb + Fc = 1
Fa/100 + Fb/50 + Fc = 1/80
Fc = 0.0002
Fb = 0.2302
```

## Ex2 

Ti(r) - time per instruction when r% of them are memory acesses

Ti(0) - time per instruction with no memory accesses (r = 0%)

```
Ti(0) = 0.5ns

Ti(100) = 400ns

Ti(0.2) = 0.998Ti(0) + 0.002Ti(100) = 1.299ns

Ti(0.2)/Ti(0) = 2.598
```

## Ex3

### a)

Tsl (time spent loading)
Tsr (time spent on remote)
Trecv (time receiving data)

```
P0: Tsl | Tsl | ...
P1:     | Tsr | Trecv	| ...
P2:           | Tsr 	| Trecv ...
```

Speedup is

```
S = T_serial/T_parallel 
```

To calculate T_serial, `partial_sum()` takes 100ns per iteration, the number of iterations is SIZE*SIZE.

```
T_serial =  100ns*SIZE*SIZE
```

To caulculate T_parallel

```
N*Tsl + Trecv +  2(matrices)*Trecv*4(bytes)*SIZE*SIZE/N + 100ns*SIZE*SIZE/N + 4*Trecv*SIZE*SIZE/N + Tsl + Tsr
```


### b)

```
S(100x100) = 15.95
S(1000*1000) = 89.24
```

In the first case, there is communication overhead.
In the second case, one process will calculate more than on line, so the communication time doesnt' matter as much.
