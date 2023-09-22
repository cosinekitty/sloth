# Sloth Circuit Analysis

<!--
![Sloth schematic](../../photos/torpor_with_nodes.jpg)
-->

## Setting up the equations

Write current equations for nodes in the circuit.

The node `n1` is assumed to be a *virtual ground* at 0V.
The sum of currents into that node must be zero:

$$
\frac{z}{R_1} + \frac{Q(z)}{R_2} + \frac{w}{K} + C_1 \frac{\mathrm{d}x}{\mathrm{d}t} = 0
\tag{1}
$$

Where $w$ is the voltage at `n3`, $K=R_3+R_9$ is the variable value of a potentiometer and fixed resistance in series,
and $Q(z)$ is the alternating output of the comparator `U1`.

In my breadboard construction of the Sloth circuit, I measure $Q(z)$ toggling between
+11.38&nbsp;V and &minus;10.64&nbsp;V, based on the polarity of $z$.
These values are a consequence of using &plusmn;12&nbsp;V supply rails
combined with the behavior of the TL074 quad op-amp.

The current equation for node `n3`:

$$
\frac{x-w}{R_6} =
    C_3 \frac{\mathrm{d}w}{\mathrm{d}t} +
    \frac{w}{K} +
    \frac{w}{R_7}
\tag{2}
$$

The op-amp `U4` has the following current equation for its inverting input node:

$$
\frac{w}{R_7} = -C_2 \frac{\mathrm{d}y}{\mathrm{d}t}
\tag{3}
$$

And the final node equation describes the currents flowing through the inverting input of `U2`:

$$
\frac{y}{R_5} + \frac{U}{R_8} + \frac{z}{R_4} = 0
\tag{4}
$$

Where $U$ is the applied control voltage (CV).

## Isolating variables

Rewrite equations to isolate the node voltage variables $w$, $x$, $y$, and $z$.

We can start with equation (3) to write $w$ in terms of $y$:

$$
w = - R_7 C_2 \frac{\mathrm{d}y}{\mathrm{d}t}
\tag{5}
$$

Solve equation (2) for $x$:

$$
x = R_6 C_3 \frac{\mathrm{d}w}{\mathrm{d}t} +
\left( 1 + \frac{R_6}{K} + \frac{R_6}{R_7} \right) w
\tag{6}
$$

Solve equation (4) for $z$:

$$
z = - R_4 \left( \frac{y}{R_5} + \frac{U}{R_8} \right)
\tag{7}
$$

## Software simulation

The capacitors $C_1$, $C_2$, and $C_3$ are like analog memories.
Their initial charge states are boundary values for the differential equations.
It is reasonable and practical to assume the circuit powers up with uncharged
capacitors, so we can assume the node voltages at $t=0$ are

$$
x_0=w_0=y_0=0
$$

The control voltage $U$ and the variable resistance $K$ are sampled
once at each time step. They are time-dependent inputs into the
circuit state.

Depending on the polarity of $U$, calculate the comparator output $Q(z)$ as +11.38&nbsp;V
if $U \ge 0$, or &minus;10.64&nbsp;V if $U \lt 0$.

Rewrite equation (1) to find the infinitesimal change in $x$:

$$
\mathrm{d} x = - \frac{\mathrm{d} t}{C_1}
    \left(
        \frac{z}{R_1} +
        \frac{Q(z)}{R_2} +
        \frac{w}{K}
    \right)
\tag{8}
$$

We know the values of everything on the right hand side of equation (8).
We can update the value of $x$ over a finite time step $\Delta t$
by approximating the infinitesimal quantities as finite differences:

$$
x_{n+1} = x_n - \frac{\Delta t}{C_1}
    \left(
        \frac{z_n}{R_1} +
        \frac{Q(z_n)}{R_2} +
        \frac{w_n}{\bar{K}}
    \right)
\tag{9}
$$

where $\bar{K}$ is the mean value of the potentiometer over the time step:

$$
\bar{K} = \frac{K_n + K_{n+1}}{2}
$$

Because the control voltage $U$ is also a function of time,
later we will need to express its mean value over the time interval as

$$
\bar{U} = \frac{U_n + U_{n+1}}{2}
$$

Continuing in the same fashion, we use equation (2) to find a finite difference
estimate for the change in $w$ over the current time step:

$$
w_{n+1} = w_n + \frac{\Delta t}{C_3}
    \left[
        \frac{1}{R_6} x_n -
        \left(
            \frac{1}{R_6} +
            \frac{1}{\bar{K}} +
            \frac{1}{R_7}
        \right) w_n
    \right]
\tag{10}
$$

Use equation (3) to update the value of $y$:

$$
y_{n+1} = y_n - \frac{\Delta t}{R_7 C_2} w_n
\tag{11}
$$

Use equation (7) to update the value of $z$:

$$
z_{n+1} = - R_4
    \left (
        \frac{y_{n+1}}{R_5} +
        \frac{\bar{U}}{R_8}
    \right)
\tag{12}
$$

And finally, update the comparator output:

$$
Q(z_{n+1}) =
    \begin{cases}
        \mathrm{+11.38 V} & \text{when } z_{n+1} \lt 0 \\
        \mathrm{-10.64 V} & \text{when } z_{n+1} \ge 0 \\
    \end{cases}
\tag{13}
$$

We could evaluate steps (9) through (13) for each successive sample $n$
to generate the output signal. But this would be too simple for good accuracy.

## Refined stability and precision

Approximating infinitesimals like $\mathrm{d}x$ with finite differences
like $\Delta x$ is risky for accuracy and numerical stability.

Therefore, it is better to refine the algorithm above by using iteration
to converge on mean values $\bar{x}$ over the interval $x_n$ to $x_{n+1}$,
$\bar{w}$ over the interval $w_n$ to $w_{n+1}$. We will do this wherever a capacitor
is being charged/discharged over the time interval.

The idea is to start with steps (9) through (12) as outlined above to
compute initial estimates of $x_{n+1}$, $w_{n+1}$, $y_{n+1}$, and $z_{n+1}$.

Then define mean values over the time interval as

$$
\bar{x} = \frac{x_n + x_{n+1}}{2}
$$

$$
\bar{w} = \frac{w_n + w_{n+1}}{2}
$$

$$
\bar{y} = \frac{y_n + y_{n+1}}{2}
$$

$$
\bar{z} = \frac{z_n + z_{n+1}}{2}
$$

Usually $\bar{Q}$ will be a constant over the time interval,
because usually the values of $z_n$ and $z_{n+1}$ have the same polarity.

But the mean value $\bar{Q}$ requires care when $z_n$ and $z_{n+1}$ have
opposite polarities, or more precisely, when $z_n z_{n+1} \lt 0$.
In this case, use a weighted mean value of $\bar{Q}$ over the time
interval. Assuming that $z(t)$ is approximately linear over the time
step, let $0 \le \alpha \le 1$ represent the fraction of the time step
at which $z(t) = 0$. Then

$$
\alpha = \frac{z_n}{z_n - z_{n+1}}
$$

Then compute the weighted mean

$$
\bar{Q} = \alpha Q({z_n}) + (1 - \alpha) Q(z_{n+1})
$$

Now we have updated estimates for the mean values of
all the voltage variables over the time interval.

Update our estimates of the voltages in the next
time step by using modified versions of steps (9) through (11):

$$
x_{n+1} = x_n - \frac{\Delta t}{C_1}
    \left(
        \frac{\bar{z}}{R_1} +
        \frac{\bar{Q}}{R_2} +
        \frac{\bar{w}}{\bar{K}}
    \right)
\tag{14}
$$

$$
w_{n+1} = w_n + \frac{\Delta t}{C_3}
    \left[
        \frac{1}{R_6} \bar{x} -
        \left(
            \frac{1}{R_6} +
            \frac{1}{\bar{K}} +
            \frac{1}{R_7}
        \right) \bar{w}
    \right]
\tag{15}
$$

$$
y_{n+1} = y_n - \frac{\Delta t}{R_7 C_2} \bar{w}
\tag{16}
$$

Unlike the other voltage variables, we can assume the op-amp `U2`
responds instanteously to its input, since there is no capacitor
to be charged or discharged. Therefore equation (12) remains valid.
We re-evaluate (12) on each iteration because the estimate
of $y_{n+1}$ changes each time.

$$
z_{n+1} = - R_4
    \left (
        \frac{y_{n+1}}{R_5} +
        \frac{\bar{U}}{R_8}
    \right)
$$

Iterate these steps until the values $x_{n+1}$, $w_{n+1}$, $y_{n+1}$, and $z_{n+1}$
converge within tolerance. More precisely, we keep track of the values of
the voltage deltas

$$
\begin{aligned}
(\Delta x)_{n+1} & = x_{n+1} - x_n \\
(\Delta w)_{n+1} & = w_{n+1} - w_n \\
(\Delta y)_{n+1} & = y_{n+1} - y_n
\end{aligned}
$$

and define the changes in the deltas as

$$
\begin{aligned}
X_{n+1} & = (\Delta x)_{n+1} - (\Delta x)_{n} \\
W_{n+1} & = (\Delta w)_{n+1} - (\Delta w)_{n} \\
Y_{n+1} & = (\Delta y)_{n+1} - (\Delta y)_{n}
\end{aligned}
$$

Keep iterating until

$$
{X_{n+1}}^2 + {W_{n+1}}^2 + {Y_{n+1}}^2 \lt \epsilon^2
$$

where $\epsilon_{n+1}$ is a voltage error tolerance.
In my implementation I use a tolerance of one picovolt ($\epsilon = 10^{-12} V$).
At a sample rate of 44100&nbsp;Hz, convergence almost always happens in 3 iterations, but it never takes more than 4 iterations.
It is important in general to test at different sample rates and
input conditions to make sure convergence happens, and to provide
a failsafe iteration limit. In my implementation, I never allow
more than 5 iterations.
