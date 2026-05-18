# CVE-2021-4034 | PwnKit Local Privilege Escalation

<p align="center">
  <img src="https://img.shields.io/badge/Linux-Vulnerable-success">
  <img src="https://img.shields.io/badge/Exploit-Local%20Privilege%20Escalation-black">
  <img src="https://img.shields.io/badge/Research-Educational-blue">
</p>

> A clean proof-of-concept exploit for the infamous **PwnKit** vulnerability affecting Linux systems from 2009 to 2022.

---

## [+] Disclaimer

This repository exists strictly for:

- Security research
- Educational purposes
- Authorized penetration testing

If you use this against systems you do not own or explicitly have permission to test, that's on you.  
Do not speedrun federal charges because a README looked cool at 3 AM.

---

# [>_] What Is PwnKit?

`PwnKit` is a local privilege escalation vulnerability in Polkit's `pkexec` utility.

- **CVE:** CVE-2021-4034
- **Component:** `pkexec`
- **Impact:** Local user → root
- **Severity:** Critical
- **Discovered publicly:** January 2022
- **Bug age:** ~12 years

The vulnerable code existed quietly inside Linux systems for over a decade.  
A regular low-privileged user could become `root` without knowing any password.

Absolute cinema.

---

# [>_] Vulnerability Flow

```text
┌─────────────────────────────────────────────────────────────────┐
│ You (regular user)                                             │
│ $ ./pwnkit.sh                                                  │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ 1. Fake gconv module gets created                              │
│ 2. pkexec gets tricked into loading it                         │
│ 3. Malicious initialization code executes                      │
│ 4. Root shell gets spawned                                     │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ # id                                                           │
│ uid=0(root) gid=0(root) groups=0(root)                         │
│ Privilege escalation successful                                │
└─────────────────────────────────────────────────────────────────┘
```

---

# [>_] Why This Vulnerability Was So Dangerous

`pkexec` was designed to securely execute commands as another user.

Instead:

- no authentication bypass needed
- no memory corruption needed
- no kernel exploit needed
- no race condition needed

Just execute the payload locally and collect root like a side quest reward.

---

# [>_] Requirements

Minimal setup.

### Needed

- Linux target
- Vulnerable `pkexec`
- `gcc`

---

# [>_] Installing GCC

## Debian / Ubuntu

```bash
sudo apt update
sudo apt install gcc -y
```

## Fedora / RHEL / CentOS

```bash
sudo dnf install gcc -y
```

## Arch Linux

```bash
sudo pacman -S gcc
```

---

# [>_] Checking Vulnerability

```bash
pkexec --version
```

Most vulnerable systems contain older unpatched versions of Polkit.

---

# [>_] Installation

## One-liner

```bash
curl -sSL https://raw.githubusercontent.com/yourusername/CVE-2021-4034-PwnKit-Exploit/main/pwnkit.sh | bash
```

---

## Manual Installation

```bash
# Clone repository
git clone https://github.com/trinetra-1308/PwnKit-.git

# Enter directory
cd PwnKit-/

# Make executable
chmod +x pwnkit.sh

# Run exploit
./pwnkit.sh
```

---

# [>_] Example Output

```bash
$ ./pwnkit.sh

[!] CVE-2021-4034 - PwnKit
[!] For educational and authorized testing only

[+] Found pkexec binary
[+] Preparing payload
[+] Compiling exploit
[+] Triggering vulnerability
[*] Attempting privilege escalation...

# whoami
root

# id
uid=0(root) gid=0(root) groups=0(root)

# hostname
target-machine

# exit
$
```

---

# [>_] Detection & Mitigation

## Check SUID Bit

```bash
ls -l /usr/bin/pkexec
```

Vulnerable systems typically show:

```bash
-rwsr-xr-x
```

The `s` means the binary runs with root privileges.

---

## Temporary Mitigation

```bash
chmod 0755 /usr/bin/pkexec
```

This removes the SUID bit and blocks exploitation.

---

# [>_] References

- [Qualys Research](https://blog.qualys.com/vulnerabilities-threat-research/2022/01/25/pwnkit-local-privilege-escalation-vulnerability-discovered-in-polkits-pkexec-cve-2021-4034)
- [NIST NVD](https://nvd.nist.gov/vuln/detail/cve-2021-4034)

---

# [>_] Final Notes

PwnKit became legendary because it proved something terrifying:

Sometimes the deadliest vulnerabilities are not hidden inside complex kernel exploitation chains.

Sometimes they're sitting quietly in a default Linux component for twelve years waiting for someone to notice one cursed `argc` edge case.

Linux admins collectively aged five years that week.

---
