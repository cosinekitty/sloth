# Sloth Circuit Analysis

<!--
![Sloth schematic](../../photos/torpor_with_nodes.jpg)
-->

Write current equations for nodes in the circuit.

The node `n1` is assumed to be a *virtual ground* at 0V:

$$
\begin{equation}
\frac{z}{R_1} + \frac{Q}{R_2} + \frac{w}{R_3+R_9} = -C_1 \frac{\mathrm{d}x}{\mathrm{d}t}
\end{equation}
$$

Where $w$ is the voltage at `n3`, $Q$ is the alternating output of the comparator `U1`, which toggles between +11.38&nbsp;V and &minus;10.64&nbsp;V, depending on the polarity of the output of `U1`, called $z$.

The current equation for node `n3`:

$$
\begin{equation}
\frac{x-w}{R_6} =
    C_3 \frac{\mathrm{d}w}{\mathrm{d}t} +
    \frac{w}{R_3 + R_9} +
    \frac{w}{R_7}
\end{equation}
$$

The op-amp `U4` has the following current equation for its inverting input node:

$$
\begin{equation}
\frac{w}{R_7} = -C_2 \frac{\mathrm{d}y}{\mathrm{d}t}
\end{equation}
$$

And the final node equation describes the currents flowing through the inverting input of `U2`:

$$
\begin{equation}
\frac{y}{R_5} + \frac{U}{R_8} + \frac{z}{R_4} = 0
\end{equation}
$$

Where $U$ is the applied control voltage (CV).