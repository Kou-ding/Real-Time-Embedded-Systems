# Real-Time-Embedded-Systems
The Real-time embedded systems assignment 2024.

### Report Markdown (report.md)
This is the assignment report in Markdown format.

### Final report Markdown (shortened-report.md)
This is the report that has been shortened to meet the assignment's length criteria.

### Cross-compilation Tutorial(cross-compile.md)
This is a quick tutorial on cross-compilation in Markdown format.

### Python folder (python-websockets)
This is a folder containing:
- a bash script that creates a virtual environment, installs necessary dependencies and plots the delay CSV files.
- the Python plotting script itself and
- a Python example of how the Finnhub WebSocket behaves.

### Media folder (md-media)
This is a folder that contains:
- PNG images used in the report Markdown and
- the pdf of the final report Markdown.

### Cross-compilation test (hello.c)
It is a program that prints hello world on our desired host machine architecture, to make sure the cross-compiler works.

### Pthreads test (ptest.c)
It is a program used for experimentation with Pthreads based on an example file that was given to us.

### WebSocket test (wstest.c)
It is a program used to test the Libwebsockets library's functionality.

### Main Program (websockets.c)
It is the program we were asked to produce combining Pthreads and the Libwebsockets library. The result is a parallel implementation of listening to a finance-oriented WebSocket and using this info to log the trades, calculate the candlestick and moving average of these trades as well as perform various benchmarks. The various results are logged inside TXT and CSV files.
