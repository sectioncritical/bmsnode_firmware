#!/bin/bash

BINFILE=../build/obj/bmsnode.elf
REPORTS=reports


sizes=$(avr-size $BINFILE | tail -n 1)
text=$(echo $sizes | cut -d ' ' -f 1)
data=$(echo $sizes | cut -d ' ' -f 2)
bss=$(echo $sizes |  cut -d ' ' -f 3)

flash=$(($text + $data))
ram=$(($data + $bss))

# '1614 has 16k flash and we reserve 512 for boot loader
# '1614 has 2k ram
flash_free=$((15872 - $flash))
ram_free=$((2048 - $ram))

flash_pct=$(($flash_free * 100 / 15872))
ram_pct=$(($ram_free * 100 / 2048))

printf "\n"
printf "|Memory | Used  |  Free  | %% Free|\n"
printf "|-------|-------|--------|-------|\n"
printf "| Flash | %4d  | %5d  |  %3d  |\n" $flash $flash_free $flash_pct
printf "| RAM   | %4d  | %5d  |  %3d  |\n" $ram $ram_free $ram_pct
printf "\n"

mkdir -p $REPORTS

venv/bin/anybadge -l RAM -v $ram_pct -s "%" -u -o -f $REPORTS/ram-badge.svg 11=brightred 30=yellow 101=green
venv/bin/anybadge -l Flash -v $flash_pct -s "%" -u -o -f $REPORTS/flash-badge.svg 11=brightred 30=yellow 101=green

if (( $ram_pct < 30 )); then
    echo "RAM Usage Warning!"
    exit 1
fi

if (($flash_pct < 20 )); then
    echo "Flash Usage Warning!"
    exit 1
fi
