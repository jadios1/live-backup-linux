# sop-backup

An interactive, live backup tool for Linux, written in C. It mirrors a source directory into one or more target directories and keeps them in sync as files change. Built for the Operating Systems course at Warsaw University of Technology.

## Features

- **One worker process per target** (`fork`): backups run independently and can be started and stopped separately.
- **Live synchronization** with recursive `inotify` watches: creating, modifying, moving and deleting files and directories is mirrored as it happens.
- **Symbolic links** are copied, and absolute links are rewritten so they point inside the backup.
- **Clean shutdown**: `SIGINT` / `SIGTERM` handlers (`sigaction`) stop and reap every worker.
- Built with AddressSanitizer and UBSan enabled by default.

## Build

```sh
make
```

## Usage

```
./sop-backup
> add <source> <target> [<target> ...]   # start mirroring source into each target
> end <source> <target> [<target> ...]   # stop the backup for the given targets
> list                                   # show running backups (pid, source, target)
> exit                                   # stop all workers and quit
```

## Known limitations

- `restore` is not implemented yet.

## Project layout

| File | Purpose |
| --- | --- |
| `main.c` | command loop and worker process management |
| `inotify.c` | recursive directory watching and event handling |
| `file_ops.c` | copying files, directories and symlinks |
| `utils.c` | path validation helpers |
