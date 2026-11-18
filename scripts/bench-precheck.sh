#!/usr/bin/env bash
# Warns (never fails) when the machine is not quiet enough for stable benchmark numbers.
gov=$(cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor 2>/dev/null | sort -u | paste -sd' ')
if [ "$gov" != "performance" ]; then
  echo "WARNING: CPU governor is '${gov:-unknown}', not 'performance'; benchmark numbers will be noisy" >&2
fi
load=$(cut -d' ' -f1 /proc/loadavg)
if awk -v l="$load" 'BEGIN { exit !(l > 1) }'; then
  echo "WARNING: 1-min load average is $load (> 1); benchmark numbers will be noisy" >&2
fi
exit 0
