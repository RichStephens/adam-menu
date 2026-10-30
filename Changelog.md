# Changelog

## [Unreleased]

## [1.0.1] 2026-10-30

- Programs boot on real hardware: a launch waits for D4 to report its disk, as EOS does before booting, and retries the boot block reads
- A failed launch shows the EOS status code instead of restarting the menu
- D4 is no longer unmounted before each launch

## [1.0.0] 2026-10-14

- Lists the `.ddp` files in the directory the menu was mounted from and boots the one picked, mounted read-only in D4
- A program returns to the menu when it exits through EOS_START (`$FC30`), which reboots D1
