# Multi-Base Calculator (uqbasejump)

A command-line arbitrary base calculator written in C. This tool allows users to input mathematical expressions in bases ranging from 2 (Binary) to 36, evaluates them, and displays the result in multiple target bases simultaneously.
## ⚠️ Academic Integrity Disclaimer
This project was created as part of the CSSE2310 course at The University of Queensland.

For Recruiters/Employers: This represents my personal solution and coding style for the assignment.

For Current Students: Please do not copy this code. Plagiarism is taken seriously at UQ. This repository is for archival and portfolio purposes only.

## Features

* **Multi-Base Support:** Handles input and output for any base between 2 and 36 (Binary, Octal, Decimal, Hex, etc.).
* **Expression Evaluation:** detailed arithmetic parsing (Addition, Subtraction, Multiplication, Division).
* **Interactive Mode:** Real-time character-by-character input processing with immediate feedback.
* **File Mode:** Read and process batch expressions from a file.
* **History Tracking:** Keep track of previous calculations within the session.
* **Dynamic Configuration:** Change input/output bases on the fly using internal commands.

## Installation & Build

### Compilation
The project includes a `Makefile` for easy compilation.

```bash
gcc -Wall -Wextra -pedantic -std=gnu99 uqbasejump.c -o uqbasejump -lm
```
🚀 Usage
You can run the program in interactive mode or file mode.
```bash
Usage: ./uqbasejump [--obases 2..36] [--inputbase 2..36] [--file string]
```

👤 Author
* Anh Tai (Raymond) Pham

## Acknowledgement
This README was written with the support of Gemini 3.0

