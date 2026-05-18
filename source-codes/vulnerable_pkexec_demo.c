/*
    =========================================================================
        CVE-2021-4034 (PwnKit) - Simplified Vulnerable Logic Demonstration
    =========================================================================

    FILE: vulnerable_pkexec_demo.c

    PURPOSE:
    --------
    This is a simplified educational recreation of the vulnerable logic
    behind PwnKit (CVE-2021-4034).

    This is NOT the real pkexec source code.

    The goal is to help researchers understand:
    - how argc/argv assumptions failed
    - how argv and envp overlap in memory
    - why out-of-bounds access became exploitable
    - how environment variable corruption occurred

    =========================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
    Simplified vulnerable function.
*/
int main(int argc, char **argv, char **envp)
{
    int n;

    /*
        ---------------------------------------------------------------------
        STEP 1: pkexec assumes argc >= 1
        ---------------------------------------------------------------------

        Under normal execution:

            ./program arg1

        argc would be:
            argc = 2

        But Linux allows execve() with malformed argument vectors.

        Example:
            execve("/path/program", NULL, envp);

        In that case:
            argc may become 0

        The original pkexec logic did NOT validate this safely.
    */

    printf("[+] argc = %d\n", argc);

    /*
        ---------------------------------------------------------------------
        STEP 2: Vulnerable argument-processing loop
        ---------------------------------------------------------------------

        If argc == 0:

            n starts at 1
            condition (1 < 0) fails immediately

        So:
            n remains 1

        That becomes important later.
    */

    for (n = 1; n < argc; n++)
    {
        printf("[+] Processing argument: %s\n", argv[n]);
    }

    /*
        ---------------------------------------------------------------------
        STEP 3: OUT-OF-BOUNDS ACCESS
        ---------------------------------------------------------------------

        Here's the vulnerability.

        If argc == 0:
            argv[1] DOES NOT EXIST

        But the code still accesses it.

        Linux stores:
            argv[]
            envp[]

        contiguously in memory.

        So:

            argv[1]
                ↓
            envp[0]

        The program accidentally reads environment variable memory.
    */

    char *path = argv[n];

    /*
        ---------------------------------------------------------------------
        DEBUG OUTPUT
        ---------------------------------------------------------------------
    */

    printf("\n[+] n = %d\n", n);

    if (path)
    {
        printf("[+] argv[%d] points to: %s\n", n, path);
    }
    else
    {
        printf("[!] argv[%d] is NULL\n", n);
    }

    /*
        ---------------------------------------------------------------------
        STEP 4: Why this became exploitable
        ---------------------------------------------------------------------

        The real pkexec later wrote data BACK into argv[n].

        Since:
            argv[1] overlaps envp[0]

        That means attackers could overwrite environment variables.

        This was catastrophic because:
            SUID binaries sanitize dangerous env vars.

        But the vulnerability allowed attackers to REINTRODUCE them.

        Especially:
            GCONV_PATH

        Which later enabled malicious shared-object loading.
    */

    if (path != NULL)
    {
        /*
            Simulated overwrite.
        */

        printf("\n[+] Simulating overwrite of argv[%d]\n", n);

        argv[n] = "/tmp/malicious_path";

        /*
            Since argv[1] overlaps envp[0],
            this effectively corrupts environment memory.
        */

        printf("[+] argv[%d] overwritten\n", n);
    }

    /*
        ---------------------------------------------------------------------
        STEP 5: Memory Layout Visualization
        ---------------------------------------------------------------------

        NORMAL LAYOUT:

        +------------+------------+------------+------------+
        | argv[0]    | argv[1]    | NULL       | envp[0]    |
        +------------+------------+------------+------------+

        MALFORMED EXECUTION:

        +------------+------------+------------+
        | argv[0]    | NULL       | envp[0]    |
        +------------+------------+------------+
                             ^
                             |
                        argv[1] access
                        lands here

        That's the entire bug.

        One invalid assumption about argc.
    */

    printf("\n[+] Demonstration complete.\n");

    return 0;
}

/*
    =========================================================================
    HOW TO COMPILE
    =========================================================================

    gcc vulnerable_pkexec_demo.c -o vulnerable_demo

    =========================================================================
    WHY THIS BUG WAS DEVASTATING
    =========================================================================

    The real pkexec binary:
    - ran as SUID-root
    - loaded dynamic libraries
    - interacted with environment variables

    Once attackers regained control over:
        GCONV_PATH

    they could:
    - load malicious shared objects
    - execute arbitrary code
    - spawn root shells

    WITHOUT:
    - kernel exploits
    - race conditions
    - memory leaks
    - ASLR bypasses

    Just:
        unsafe argument validation
        +
        contiguous process memory

    =========================================================================
*/