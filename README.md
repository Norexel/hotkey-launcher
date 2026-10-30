# Hotkey Launcher

A Windows desktop app written in C++ that starts and stops programs from global keyboard shortcuts.

You add a program once (its path, an optional command line and a hotkey). With monitoring switched on, pressing the hotkey anywhere in Windows launches that program, and pressing it again closes it.

## What it does

- Keeps a list of programs, each with a path, a command line and a hotkey
- Watches the keyboard globally through a low-level hook while monitoring is on
- Tracks the processes it started, so the same hotkey can close them again
- Lives in the system tray
- Draws its own interface with GDI+

## Built with

C++, WinAPI, GDI+, Visual Studio

## Build

1. Open `MegaTest1.sln` in Visual Studio 2022
2. Choose `x64` and `Release`
3. Build and run

## Status

Working prototype.
