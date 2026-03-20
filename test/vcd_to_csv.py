#!/usr/bin/env python

import csv
import sys
from vcd.reader import tokenize, TokenKind

def vcd_to_csv(vcd_path, csv_path):

    signals = {}
    id_to_name = {}
    current_values = {}
    rows = []
    current_time = 0

    with open(vcd_path, "rb") as f:

        for token in tokenize(f):

            if token.kind == TokenKind.VAR:
                var = token.data
                name = var.reference
                id_code = var.id_code

                signals[id_code] = name
                id_to_name[id_code] = name
                current_values[name] = None

            elif token.kind == TokenKind.CHANGE_TIME:

                if rows or current_time != 0:
                    rows.append((current_time, current_values.copy()))

                current_time = token.data

            elif token.kind == TokenKind.CHANGE_SCALAR:

                id_code = token.data.id_code
                value = token.data.value

                name = id_to_name[id_code]
                current_values[name] = value

            elif token.kind == TokenKind.CHANGE_VECTOR:

                id_code = token.data.id_code
                value = token.data.value

                name = id_to_name[id_code]
                current_values[name] = value

            elif token.kind == TokenKind.CHANGE_REAL:

                id_code = token.data.id_code
                value = token.data.value

                name = id_to_name[id_code]
                current_values[name] = value

    rows.append((current_time, current_values.copy()))

    signal_names = sorted(current_values.keys())

    with open(csv_path, "w", newline="") as csvfile:

        writer = csv.writer(csvfile)

        writer.writerow(["time"] + signal_names)

        for time, values in rows:

            row = [time]
            for sig in signal_names:
                row.append(values.get(sig))

            writer.writerow(row)


if __name__ == "__main__":

    if len(sys.argv) != 3:
        print("usage: python vcd_to_csv.py input.vcd output.csv")
        sys.exit(1)

    vcd_to_csv(sys.argv[1], sys.argv[2])
