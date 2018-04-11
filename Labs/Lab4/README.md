# 1st Exercise Class

## Ex 1

![Image1](https://github.com/Mrrvm/CPD/blob/master/Labs/Lab4/ex1.png)

## Ex 2

```
ft_parallel/10 + fT_serial = 1/6

1 = fT_serial + ft_parallel

ft_parallel ~ 0.926
```

## Ex 3

This models the probability of q processors being active at a certain time.  θ is the degree of parallelism.

```
F(q,θ) = β[(1−θ)·(p+ 1) + (2θ−1)·q]

```

### a)

```
∑ (p, q=1) F(q, θ) = 1
∑ (p, q=1) β[(1−θ)·(p+ 1) + (2θ−1)·q] = 1
β[(1−θ) · ∑ (p, q=1)(p+ 1) + (2θ−1) · ∑ (p, q=1) q] = 1
β[(1−θ) · p(p+ 1) + (2θ−1) · p(p+1)/2] = 1
β = 2/(p(p+1))

```

### b)

```
S = Ts/Tp

S = 1/(Fs + Fp/p)

Tp = Ts · ∑ (p, q=1) F(q, θ)/q

S = Ts / (Ts · ∑ (p, q=1) F(q, θ)/q) = 1 / (∑ (p, q=1) F(q, θ)/q)

```

### c)

```
∑ (p, q=1) F(q, θ)/q =
∑ (p, q=1) (1/q) 2/(p(p+1))[(1−θ)·(p+ 1) + (2θ−1)·q] = 
2(1-θ)/p · ∑ (p, q=1) (1/q) + 2(2θ-1)/p(p+1) · ∑ (p, q=1) 1 = 

S = p(p+1)/(2(1-θ)(ln(p)+1)(p+1) + 2(2θ-1)p)

θ = 0   :  S = p/2ln(p)
θ = 0.5 :  S = p/(ln(p)+1)
θ = 1   :  S = (p+1)/2
```




