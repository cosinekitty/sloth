# Sloth Circuit Analysis

<!--
![Sloth schematic](../../photos/torpor_with_nodes.jpg)
-->

## Setting up the equations

Write current equations for nodes in the circuit.

The node `n1` is assumed to be a *virtual ground* at 0V:

$$
\frac{z}{R_1} + \frac{Q(z)}{R_2} + \frac{w}{R_3+R_9} = -C_1 \frac{\mathrm{d}x}{\mathrm{d}t}
\tag{1}
$$

Where $w$ is the voltage at `n3` and $Q(z)$ is the alternating output of the comparator `U1`, which toggles between +11.38&nbsp;V and &minus;10.64&nbsp;V.

The current equation for node `n3`:

$$
\frac{x-w}{R_6} =
    C_3 \frac{\mathrm{d}w}{\mathrm{d}t} +
    \frac{w}{R_3 + R_9} +
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
\left( 1 + \frac{R_6}{R_3 + R_9} + \frac{R_6}{R_7} \right) w
\tag{6}
$$

Solve equation (4) for $z$:

$$
z = - R_4 \left( \frac{y}{R_5} + \frac{U}{R_8} \right)
\tag{7}
$$

