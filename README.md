# letterSimu
DNA/RNA Sequence simulator

## CLI

`lettersimu` now uses a Sharg-based command-line parser.

Examples:

```bash
./build/lettersimu --config config.toml
./build/lettersimu --list-targets
./build/lettersimu --config config.toml --target dna_reads
./build/lettersimu --config config.toml --log-level debug
```

Supported options:

- `-c, --config`: path to the TOML configuration file, defaults to `config.toml`
- `-l, --log-level`: override `LETTERSIMU_LOG_LEVEL` for the current run
- `-t, --target`: print the derived parameter set for one target parser
- `--list-targets`: list the available target parser names and exit
