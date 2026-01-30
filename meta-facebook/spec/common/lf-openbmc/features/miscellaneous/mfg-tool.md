`mfg-tool` is a command-line tool specifically designed for manufacturing and testing purposes on OpenBMC platforms. It provides various subcommands to perform different tasks, such as displaying sensor values, setting fan speeds, and manipulating fan modes, among others. This tool leverages the sdbusplus library and system APIs and libraries to interact with the D-Bus interface and modify behavior as needed.

If users want to have retries, they can create a dotfile under `/home/root/.mfgtool.json`. This file is a JSON file with key-value pairs. Currently, it supports `retries` and `timeout`.

If the command fails to execute, it will try again for the number of times defined in `retries`. It will exit once the return code (rc) of the child process exits cleanly, or the number of retries has been exhausted.

There's also a timeout feature. This simply creates an alarm signal, and the child process is killed. The retries setting is honored.

Example of `/home/root/.mfgtool.json`:

```json
{
  "timeout": 120,
  "retries": 5
}
```
