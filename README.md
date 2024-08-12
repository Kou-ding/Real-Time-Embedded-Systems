# Real-Time-Embedded-Systems
The Real-time embedded systems assignment 2024.

### Report Markdown (report.md)
This is the assignment report in Markdown format.

### Cross-compilation Tutorial(cross-compile.md)
This is a quick tutorial on cross-compilation in Markdown format.

### Python folder (python_websockets)
This is a folder containing:
- a bash script that creates a virtual environment, installs necessary dependencies and plots the delay CSV files.
- the Python plotting script itself and
- a Python example of how the Finnhub WebSocket behaves.

### Pthreads test (ptest.c)
It is a program used for experimentation with Pthreads based on an example file that was given to us.

### WebSocket test (wstest.c)
It is a program used to test the Libwebsockets library's functionality.

### Main Program (main.c)
It is the program we were asked to produce combining Pthreads and the Libwebsockets library. The result is a parallel implementation of listening to a finance-oriented WebSocket and using this info to log the trades, calculate the candlestick and moving average of these trades as well as perform various benchmarks. The various results are logged inside TXT and CSV files.