import os
import subprocess
import sys

tests_dir = "external/riscv-tests/isa"
emulator = "./build/riscv-emulator"
passed = 0
failed = 0

def run_test(file_path, file_name):
    global passed
    global failed

    print(" " * 30, end='\r')
    print(f"Running {file_name}", end='\r')

    # Emulate test
    with open(os.devnull, 'w') as devnull:
        try:
            result = subprocess.run([emulator, "--testing", file_path], stdout=devnull, stderr=devnull)
        except KeyboardInterrupt:
            print("Running " + file_name)
            exit()

    print(" " * 30, end='\r')

    # Check the return status
    if result.returncode == 1:
        passed += 1
        return True
    else:
        failed += 1
        return False

def pad(name):
    amount = 30
    return name + (amount-len(name)) * " "

def main():
    tests = []

    # Buid list of tests
    for file_name in os.listdir(tests_dir):
        # Ignore "non-tests"
        if file_name[-3:] != "bin":
            continue

        file_path = os.path.join(tests_dir, file_name)
        if os.path.isfile(file_path):
            tests.append({
                "name": file_path,
                "display_name": pad(file_path.replace(tests_dir + "/", ""))
            })

    for test in tests:
        if run_test(test["name"], test["display_name"]):
            test["passed"] = True
        else:
            test["passed"] = False

    # Display
    print()
    print("Test Name\t            Passed")
    print("-" * 34)

    for test in tests:
        status = "✅" if test["passed"] else "❌"
        print(f"{test['display_name']}\t{status}")

    print(f"\n\tPassed {passed}/{passed + failed}")

if __name__ == "__main__":
    main()
