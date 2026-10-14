# adam-menu

A program launcher for the Coleco Adam with FujiNet. It lists the `.ddp`
files in the directory it was mounted from and boots the one you pick.

## Using it

1. Put `adam-menu.ddp` in the same directory as the programs, and mount it
   in D1.
2. Boot. Pick a program with Up/Down and press RETURN.

The program is mounted read-only in D4 and booted from there, so D1 still
holds the menu. Rebooting the Adam brings the menu back.

## Returning to the menu

A program returns to the menu when it exits through EOS_START (`$FC30`),
which reboots D1. For a z88dk program, link with:

    -pragma-define:CRT_ON_EXIT=0xFC30

## Building

    make adam    # r2r/adam/adam-menu.ddp
    make dist    # dist/adam-menu-<version>.zip

`FUJINET_LIB` in the Makefile clones and builds fujinet-lib.
Each [release](https://github.com/RichStephens/adam-menu/releases) carries
`adam-menu.ddp`.

## Releasing

1. Rename `[Unreleased]` in `Changelog.md` to the new version and date, and
   start an empty `[Unreleased]` above it.
2. Commit and push to `main`.
3. Tag and push the tag:

       git tag v1.2.3
       git push origin v1.2.3

The release workflow builds the zip and publishes it with that changelog
section as the notes. It fails if the newest section does not match the tag.
