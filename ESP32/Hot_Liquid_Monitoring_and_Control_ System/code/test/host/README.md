Run `python test/host/run_tests.py` from the project directory with Python 3 and
`g++` installed. Set `CXX` to the compiler executable path if it is not on PATH.
The build and executable use a temporary directory that is removed afterward.

These tests compile the actual device manager, LED, buzzer, relay, and pin modules.
They replace the clock, sensor readings, GPIO, and PWM hardware with host stubs.
Coverage includes all four threshold boundaries, selected latch modes, persistent
faults, settings changes, guarded atomic clearing, invalid sensors, fresh readings
on clear, manual relay requests, LED/buzzer timing, and the startup beep.

Physical pin wiring, sensor drivers, dashboard HTTP behavior, and firmware boot
must be verified separately on the board.
