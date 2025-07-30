# MUS Automation

System to automate MUS (Minimal Unsatisfiable Subsets) analysis with aaltaf.

## Setup

```bash
cd mus-automation
python3 mus_automation.py setup
```

## Usage

### Clean benchmark state
```bash
# Reset tracking (benchmarks will be processed again)
python3 mus_automation.py clean

# Also remove output files
python3 mus_automation.py clean --output
```

### Single file test
```bash
python3 mus_automation.py test -f ../examples/p11.ltl
```

### Batch test on directory
```bash
python3 mus_automation.py test -d ../examples
```

### Analyze results

Basic analysis with summary:
```bash
python3 mus_automation.py analyze --report
```

Advanced analysis with visualizations:
```bash
python3 mus_automation.py analyze --plots --csv --detailed
```

## Analysis Features

The analysis system provides multiple output formats:

- **Basic report** (`--report`) - Console summary with key statistics
- **Detailed JSON** (`--detailed`) - Comprehensive statistics in JSON format
- **CSV export** (`--csv`) - Results in CSV format for spreadsheet analysis
- **Visualizations** (`--plots`) - Statistical plots and charts:
  - MUS count distribution per file
  - Execution time vs MUS count correlation
  - Individual MUS size distribution
  - Performance metrics overview

Output files are saved to `results/` directory by default.

## Output

- `results/` - Analysis results, plots, CSV files
- `benchmarks/` - Processed file tracking
- Each .ltl file generates a `*_out` file with MUS output

## Available commands

- `setup` - Configure environment
- `clean` - Reset benchmark tracking (force re-run)
- `test` - Run MUS tests
- `analyze` - Analyze results with various output formats

## Test options

- `-d DIR` - Directory with .ltl files
- `-f FILE` - Single .ltl file
- `-t SEC` - Timeout in seconds
- `-v` - Verbose output

## Analysis options  

- `--report` - Generate console summary
- `--plots` - Generate visualization plots
- `--csv` - Export to CSV format
- `--detailed` - Generate detailed JSON statistics
- `-o DIR` - Specify output directory

The system automatically tracks processed files and avoids repeating them.