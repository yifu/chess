sudo bash -c "echo 0 > /proc/sys/kernel/yama/ptrace_scope"
ulimit -c unlimited
