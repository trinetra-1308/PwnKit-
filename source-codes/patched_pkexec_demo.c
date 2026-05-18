/*
    =========================================================================
        CVE-2021-4034 (PwnKit) - Patched Logic Demonstration
    =========================================================================

    FILE: patched_pkexec_demo.c

    PURPOSE:
    --------
    This is a simplified educational recreation of how the PwnKit
    vulnerability was fixed.

    This is NOT the real pkexec source code.

    The goal is to explain:
    - why the original code was vulnerable
    - what validation was missing
    - how proper argc checking prevents exploitation
    - why tiny fixes can stop catastrophic bugs

    =========================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
    =========================================================================
        SAFE / PATCHED IMPLEMENTATION
    =========================================================================
*/

int main(int argc, char **argv, char **envp)
{
    int n;

    printf("[+] argc = %d\n", argc);

    /*
        ---------------------------------------------------------------------
        PATCH #1 - Validate argc BEFORE touching argv[]
        ---------------------------------------------------------------------

        Original vulnerable logic assumed:

            argc >= 1

        But Linux allows malformed process execution via execve():

            execve("/path/program", NULL, envp);

        Which may create:
            argc == 0

        In the vulnerable code:
            argv[1] became an out-of-bounds access.

        The patched code immediately validates argc
        before performing ANY argument operations.
    */

    if (argc < 1)
    {
        fprintf(stderr,
            "[!] Invalid argc detected.\n"
            "[!] Refusing execution to prevent memory corruption.\n");

        return 1;
    }

    /*
        ---------------------------------------------------------------------
        PATCH #2 - Safe argument processing
        ---------------------------------------------------------------------

        The vulnerable code later accessed argv[n]
        without guaranteeing n remained valid.

        The fix ensures:
            n NEVER exceeds valid argv bounds.
    */

    for (n = 1; n < argc; n++)
    {
        if (argv[n] != NULL)
        {
            printf("[+] Processing argument: %s\n", argv[n]);
        }
    }

    /*
        ---------------------------------------------------------------------
        PATCH #3 - Bounds-safe access
        ---------------------------------------------------------------------

        The vulnerable version did:

            path = argv[n];

        Even when:
            n == 1
            argc == 0

        Which caused:
            argv[1] → envp[0]

        The patched version verifies:
            n < argc

        BEFORE dereferencing argv[n].
    */

    if (n >= argc)
    {
        printf("\n[+] No valid argument available.\n");
        printf("[+] Preventing out-of-bounds access.\n");

        return 0;
    }

    /*
        Safe access.
    */

    char *path = argv[n];

    /*
        Additional NULL safety check.
    */

    if (path == NULL)
    {
        printf("[+] Argument pointer is NULL.\n");
        printf("[+] Safe termination.\n");

        return 0;
    }

    /*
        ---------------------------------------------------------------------
        SAFE EXECUTION FLOW
        ---------------------------------------------------------------------
    */

    printf("\n[+] Safe argument retrieved:\n");
    printf("[+] argv[%d] = %s\n", n, path);

    /*
        ---------------------------------------------------------------------
        WHY THE PATCH WORKS
        ---------------------------------------------------------------------

        The original vulnerability depended on:

            1. argc edge case
            2. invalid argv indexing
            3. argv/envp contiguous memory
            4. environment corruption

        The fix kills the chain at STEP 1.

        No invalid argc state:
            ↓

        No OOB access:
            ↓

        No environment corruption:
            ↓

        No GCONV_PATH injection:
            ↓

        No root shell.

        Tiny fix.
        Massive impact.
    */

    printf("\n[+] Program executed safely.\n");

    return 0;
}

/*
    =========================================================================
    MEMORY LAYOUT COMPARISON
    =========================================================================

    -----------------------------
    VULNERABLE EXECUTION
    -----------------------------

    +------------+------------+------------+
    | argv[0]    | NULL       | envp[0]    |
    +------------+------------+------------+
                         ^
                         |
                    argv[1] access
                    lands here

    Result:
        Out-of-Bounds Read/Write

    -----------------------------
    PATCHED EXECUTION
    -----------------------------

    if (argc < 1)
        return;

    Result:
        Invalid memory access NEVER occurs.

    =========================================================================
    HOW TO COMPILE
    =========================================================================

    gcc patched_pkexec_demo.c -o patched_demo

    =========================================================================
    KEY SECURITY LESSONS
    =========================================================================

    1. Never trust argc.
    2. Never assume argv exists.
    3. Always validate array bounds.
    4. SUID binaries require paranoid validation.
    5. Tiny edge cases become catastrophic under privilege.

    =========================================================================
    WHY THIS BUG MATTERED SO MUCH
    =========================================================================

    The terrifying part of PwnKit was not advanced exploitation.

    It was:
        one missing validation check
            +
        one privileged binary
            +
        thirteen years unnoticed

    And suddenly:
        almost every Linux distro shipped a root escalation primitive.

    =========================================================================
*/