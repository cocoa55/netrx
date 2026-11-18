#!/usr/bin/env bash
# Prints the machine state that affects benchmark noise, for saving next to a result JSON.
echo "date: $(date -Iseconds)"
echo "cpu: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2- | sed 's/^ //')"
echo "kernel: $(uname -r)"
# Disk UUIDs (rd.luks.uuid=, root=/dev/mapper/luks-..., root=UUID=...) are redacted.
echo "cmdline: $(sed -E 's/[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}/<uuid-redacted>/g' /proc/cmdline)"
echo "isolcpus: $(cat /sys/devices/system/cpu/isolated 2>/dev/null)"
echo "nohz_full: $(cat /sys/devices/system/cpu/nohz_full 2>/dev/null)"
echo "governor: $(cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor 2>/dev/null | sort -u | paste -sd' ')"
echo "intel_pstate no_turbo: $(cat /sys/devices/system/cpu/intel_pstate/no_turbo 2>/dev/null)"
echo "loadavg: $(cat /proc/loadavg)"
