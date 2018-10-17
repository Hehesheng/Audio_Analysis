import subprocess


def main():
    subprocess.run(["echo", "rebuilding...."])
    subprocess.run(["echo", "\n\r----------Please Wait----------\n\r"])
    subprocess.run(["make", "clean"])
    subprocess.run(["make", "all"])


if __name__ == "__main__":
    main()
