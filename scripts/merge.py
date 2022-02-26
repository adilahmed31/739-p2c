
with open("data.txt", "r") as totFile:
    for unit in totFile:
        f = open("output.txt", "a")
        value = float(unit) / (1024.0)
        f.write(str(value) + '\n')


