#!/usr/bin/env python3
import sys

def extract_first_row_freq_mhz(text: str) -> str:
    lines = text.splitlines()

    # Find the first table header that contains both "Endpoint" and "Freq(MHz)"
    header_idx = None
    for i, line in enumerate(lines):
        s = line.strip()
        if s.startswith("|") and "Endpoint" in s and "Freq(MHz)" in s:
            header_idx = i
            break

    if header_idx is None:
        raise ValueError("Could not find the Endpoint/Freq(MHz) table header.")

    # After the header there is usually a separator line starting with '+',
    # then the first data row starting with '|'
    for j in range(header_idx + 1, len(lines)):
        s = lines[j].strip()
        if not s:
            continue
        if s.startswith("+"):  # table border/separator
            continue
        if s.startswith("|"):
            # This should be the first data row
            cols = [c.strip() for c in s.split("|")[1:-1]]  # fields between pipes
            if not cols:
                continue
            freq = cols[-1]  # last column is Freq(MHz)
            return freq

    raise ValueError("Could not find the first data row after the table header.")

def main():
    if len(sys.argv) != 2:
        print("Usage: python script.py path/to/file.txt", file=sys.stderr)
        sys.exit(2)

    path = sys.argv[1]
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        text = f.read()

    print(extract_first_row_freq_mhz(text))

if __name__ == "__main__":
    main()
