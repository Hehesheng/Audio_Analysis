import subprocess


def main():
    subprocess.run(["echo", "building...."])
    subprocess.run(["echo", "\n\r----------Please Wait----------\n\r"])
    subprocess.run(["make", "all"])
    # subprocess.run(["make", "delete"])


if __name__ == "__main__":
    main()
