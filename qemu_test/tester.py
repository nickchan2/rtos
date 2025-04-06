import multiprocessing
import os
import signal
import subprocess
from typing import List

TESTS: List[str] = [
    "test_sanity",
    "test_starting_rtos_enters_first_task",
    "test_task_self",
    "test_passing_arg",
    "test_context_switch_regs",
    "test_two_tasks_yielding",
    "test_basic_sleep",
    "test_starved_task",
    "test_task_exit",
    "test_time_slicing",
    # "test_fp_context_switch", # FIXME
    "test_basic_mutex_trylock",
    "test_basic_mutex_block",
    "test_basic_suspend_resume",
    "test_cond_signal",
    "test_cond_broadcast",
]

GREEN: str = "\033[92m"
RED: str = "\033[91m"
RESET: str = "\033[0m"

def run_qemu(test: str, q: multiprocessing.Queue) -> None:
    qemu = subprocess.Popen(
        ["qemu-system-arm", "-M", "olimex-stm32-h405", "-nographic", "-kernel", f"build/{test}.elf"],
        cwd=os.path.dirname(os.path.abspath(__file__)),
        preexec_fn=os.setsid,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdin=subprocess.PIPE,
        text=True,
    )

    try:
        output: str = ""
        while True:
            read: str = qemu.stdout.readline()
            if read == "<Test finished>\n":
                break
            else:
                output += read
        q.put(output)
        exit(0)
    except:
        os.killpg(os.getpgid(qemu.pid), signal.SIGTERM)

def run_test(test: str) -> bool:
    print("\n------------------------------------------")
    print(f"Test: {test}")

    print("Building test...")
    result = subprocess.run(
        ["make", f"-j{os.cpu_count()}", f"TEST_NAME={test}"],
        cwd=os.path.dirname(os.path.abspath(__file__)),
        stdout=subprocess.PIPE, 
        stderr=subprocess.PIPE, 
        text=True
    )
    if result.returncode != 0:
        print("Error building test:")
        print(result.stdout)
        print(result.stderr)
        print(f"{RED}Test failed{RESET}")
        return False

    print("Running test...")
    
    q = multiprocessing.Queue()
    p = multiprocessing.Process(target=run_qemu, args=(test, q,))
    p.start()

    p.join(timeout=5)

    passed = False
    if p.is_alive():
        print("Test timed out")
    else:
        output: str = q.get()
        if (output == "Pass\n"):
            passed = True
        else:
            print("Test failed with output:")
            print(output)

    p.terminate()
    p.join()
    if passed:
        print(f"{GREEN}Test passed{RESET}")
    else:
        print(f"{RED}Test failed{RESET}")
    return passed

if __name__ == "__main__":
    subprocess.run(
        ["make", "clean"],
        cwd=os.path.dirname(os.path.abspath(__file__)),
    )
    pass_count: int = 0
    for test in TESTS:
        if run_test(test):
            pass_count += 1

    print(f"\n{pass_count}/{len(TESTS)} tests passed")
    if pass_count != len(TESTS):
        exit(1)
