from enum import StrEnum
import multiprocessing as mp
import os
import signal
import subprocess
import sys
from typing import List, Optional

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
    "test_basic_task_join",
    "test_fp_context_switch",
    "test_basic_mutex_trylock",
    "test_basic_mutex_block",
    "test_basic_suspend_resume",
    "test_cond_signal",
    "test_cond_broadcast",
    "test_priority_change",
    "test_no_priority_inversion_deadlock",
    "test_privilege_disable_irq",
    "test_mqueue_basic",
    "test_mqueue_waiting",
    "test_mqueue_wait_enqueue",
    "test_mqueue_try_enqueue_isr",
]

class Ansi(StrEnum):
    BOLD = "\033[1m"
    GREEN = "\033[92m"
    RED = "\033[91m"
    RESET = "\033[0m"

def run_qemu(test: str, output_q: mp.Queue, qemu_pid_q: mp.Queue) -> None:
    qemu = subprocess.Popen(
        ["qemu-system-arm", "-M", "olimex-stm32-h405", "-nographic", "-kernel",
         f"build/{test}.elf"],
        cwd=os.path.dirname(os.path.abspath(__file__)),
        preexec_fn=os.setsid,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdin=subprocess.PIPE,
        text=True,
    )

    qemu_pid_q.put(qemu.pid)

    output: str = ""
    while True:
        read: str = qemu.stdout.readline()
        if read == "<Test finished>\n":
            break
        else:
            output += read
    output_q.put(output)
    exit(0)

def build_test(test: str, optimize: bool) -> Optional[str]:
    oflags: str = "-O2 -flto" if optimize else "-O0"
    result = subprocess.run(
        ["make", f"-j{os.cpu_count()}", f"TEST_NAME={test}",
         "TARGET_BOARD=F405", f"OPTIMIZE_FLAGS={oflags}"],
        cwd=os.path.dirname(os.path.abspath(__file__)),
        stdout=subprocess.PIPE, 
        stderr=subprocess.PIPE, 
        text=True
    )
    return None if result.returncode == 0 else result.stdout + result.stderr

def run_test(test: str, optimize: bool) -> bool:
    print("\n------------------------------------------")
    print(f"{Ansi.BOLD}Test: {test}{Ansi.RESET}")
    print("Building test...")
    build_error = build_test(test, optimize)
    if build_error:
        print("\nBuild failed with output:")
        print(build_error)
        print(f"{Ansi.RED}Test failed{Ansi.RESET}")
        return False

    print("Running test...")
    
    # Use queues to return values from the subprocess
    output_q = mp.Queue()
    qemu_pid_q = mp.Queue()
    p = mp.Process(target=run_qemu, args=(test, output_q, qemu_pid_q))
    p.start()

    p.join(timeout=4)

    passed = False
    if p.is_alive():
        p.terminate()
        p.join()
        print("Test timed out")
    else:
        output: str = output_q.get()
        if (output == "Pass\n"):
            passed = True
        else:
            print("Test failed with output:")
            print(output)

    os.kill(qemu_pid_q.get(), signal.SIGKILL)

    if passed:
        print(f"{Ansi.GREEN}Test passed{Ansi.RESET}")
    else:
        print(f"{Ansi.RED}Test failed{Ansi.RESET}")
    return passed

def tester() -> None:
    subprocess.run(
        ["make", "clean"],
        cwd=os.path.dirname(os.path.abspath(__file__)),
    )

    pass_count: int = 0
    for test in TESTS:
        if run_test(test, "optimize" in sys.argv[1:]):
            pass_count += 1

    print(f"\n{pass_count}/{len(TESTS)} tests passed")
    if pass_count != len(TESTS):
        exit(1)

if __name__ == "__main__":
    tester()
