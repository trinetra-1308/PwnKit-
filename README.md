# 🛡️ CVE-2021-4034 | PwnKit - Local Privilege Escalation Exploit

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Security Research](https://img.shields.io/badge/Purpose-Security%20Research-red)](https://github.com/yourusername/CVE-2021-4034-PwnKit-Exploit)
[![Compatibility](https://img.shields.io/badge/Linux-✓-green)](https://www.kernel.org/)

> **A proof-of-concept exploit for the PwnKit vulnerability that affects nearly every Linux distribution from 2009 to 2022**

## ⚠️ Important Disclaimer

This tool is created **exclusively for educational and authorized security testing purposes**. 
Using it against systems you don't own or lack explicit permission to test is illegal and unethical. 
You assume full responsibility for how you use this code.

## 🎯 What Is This?

This repository contains a clean, well-documented proof-of-concept exploit for **CVE-2021-4034**, 
commonly known as "PwnKit." It's a local privilege escalation vulnerability in Polkit's `pkexec` 
utility that has been hiding in plain sight for over 13 years.

In plain English? A regular user (no special permissions needed) can become **root** just by running 
a simple script. Yes, it's that bad.

## 🔍 A Quick Story

Imagine you're working on a shared Linux machine. You need root access for a legitimate task, 
but you don't have the password. Normally, you'd be stuck. But on vulnerable systems, there's 
a nasty surprise: any user can become root without a password. That's exactly what this 
vulnerability allows.

The irony? `pkexec` was designed to *securely* grant privileges. Instead, it became the backdoor.

## 📋 How the Exploit Works (The Short Version)

Here's what happens under the hood when you run this exploit:
