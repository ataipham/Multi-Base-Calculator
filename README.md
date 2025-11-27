# Multi-Base Calculator (uqbasejump)

A command-line arbitrary base calculator written in C. This tool allows users to input mathematical expressions in bases ranging from 2 (Binary) to 36, evaluates them, and displays the result in multiple target bases simultaneously.
This project is based on a coding assignment from CSSE2310 course at UQ in Semester 2 2025

## 📋 Features

* **Multi-Base Support:** Handles input and output for any base between 2 and 36 (Binary, Octal, Decimal, Hex, etc.).
* **Expression Evaluation:** detailed arithmetic parsing (Addition, Subtraction, Multiplication, Division).
* **Interactive Mode:** Real-time character-by-character input processing with immediate feedback.
* **File Mode:** Read and process batch expressions from a file.
* **History Tracking:** Keep track of previous calculations within the session.
* **Dynamic Configuration:** Change input/output bases on the fly using internal commands.

## 🛠 Installation & Build

### Prerequisites
* GCC Compiler
* Make

### Compilation
The project includes a `Makefile` for easy compilation.

```bash
make
🚀 Usage
You can run the program in interactive mode or file mode.

1. Interactive Mode
Run without arguments to start the interactive shell:
