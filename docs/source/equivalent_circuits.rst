Equivalent circuits
-------------------

Series RC
^^^^^^^^^

.. figure:: series_rc.png

A resistor and a capacitor are connected in series (denoted :math:`\mathrm{ESR}` 
and :math:`\mathrm{C}` in the figure above).

.. code::

    type                SeriesRC
    series_resistance     5.0e-3 ; [ohm]
    capacitance           3.0    ; [fahrad]
   
Above is the database to build a :math:`\mathrm{3\ F}` capacitor in series with a 
:math:`50\ \mathrm{m\Omega}` resistance.

.. math::

     U = U_C + R I

     I = C \frac{dU_C}{dt}


:math:`U_C` stands for the voltage across the capacitor.
Its capacitance, :math:`C`, represents its ability to store electric charge.
The equivalent series resistance, :math:`R`, add a real component to the
impedance of the circuit:

.. math::

    Z = \frac{1}{jC\omega} + R

As the frequency goes to infinity, the capacitive impedance approaches zero
and :math:`R` becomes significant.


Parallel RC
^^^^^^^^^^^

.. figure:: parallel_rc.png
   
An extra resistance is placed in parallel of the capacitor. It can be
instantiated by the following database.

.. code::

    type                 ParallelRC
    parallel_resistance      2.5e+6 ; [ohm]
    series_resistance       50.0e-3 ; [ohm]
    capacitance              3.0    ; [fahrad]

``type`` has been changed from ``SeriesRC`` to ``ParallelRC``.
A :math:`2.5\ \mathrm{M\Omega}` leakage resistance is specified.

.. math::

     U = U_C + R I

     I = C \frac{dU_C}{dt} + \frac{U_C}{R_L}

:math:`R_L` corresponds to the "leakage" resistance in parallel with the
capacitor. Low values of :math:`R_L` imply high leakage currents which means
the capacitor is not able to hold is charge.
The circuit complex impedance is given by:

.. math::

    Z = \frac{R_L}{1+jR_LC\omega} + R

