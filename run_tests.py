import os
import subprocess

tests_dir = "external/bin-files"
emulator = "./build/riscv-emulator"
passed = 0
failed = 0

def run_test(file_path):
    global passed
    global failed

    # Emulate test
    with open(os.devnull, 'w') as devnull:
        result = subprocess.run([emulator, file_path], stdout=devnull)

    # Check the return status
    if result.returncode == 1:
        passed += 1
        return True
    else:
        failed += 1
        return False

def pad(name):
    amount = 20
    assert(len(name) <= amount)
    return name + (amount-len(name)) * " "

def main():
    tests = []

    for file_name in os.listdir(tests_dir):
        file_path = os.path.join(tests_dir, file_name)
        if os.path.isfile(file_path):
            tests.append({
                "name": file_path,
                "display_name": pad(file_path.replace(tests_dir + "/", ""))
            })

    for test in tests:
        if run_test(test["name"]):
            test["passed"] = True
        else:
            test["passed"] = False

    # Display
    print("Test Name\t      Passed")
    print("-" * 30)

    for test in tests:
        status = "✅" if test["passed"] else "❌"
        print(f"{test['display_name']}\t{status}")

    print(f"\n\tPassed {passed}/{passed + failed}")

if __name__ == "__main__":
    main()
