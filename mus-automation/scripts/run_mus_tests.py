#!/usr/bin/env python3
"""
MUS testing runner
"""

import os
import sys
import argparse
from typing import List, Set

# Add config to path
sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'config'))
import config
from utils import (
    setup_directories, run_aaltaf_mus, get_benchmark_files,
    load_done_benchmarks, save_done_benchmarks,
    load_error_benchmarks, save_error_benchmarks,
    log_message
)

def run_mus_benchmarks(benchmark_files: List[str], flags: List[str] = None, timeout: int = None,
                       memory_limit: int = None) -> None:
    """
    Run MUS benchmarks

    Args:
        benchmark_files: List of .ltl files to process
        flags: Command flags (default: ["-emus2"])
        timeout: Timeout per benchmark
        memory_limit: Virtual memory limit in bytes (default: config.MAX_VIRTUAL_MEMORY)
    """
    if flags is None:
        flags = config.DEFAULT_FLAGS

    if timeout is None:
        timeout = config.DEFAULT_TIMEOUT

    if memory_limit is None:
        memory_limit = config.MAX_VIRTUAL_MEMORY

    log_message(f"Limits per benchmark: timeout {timeout}s, memory {memory_limit / (1024**3):.0f}GB")

    # Setup directories
    setup_directories()
    
    # Load existing done and error sets
    done = load_done_benchmarks(config.MUSBENCHMARKSD)
    err = load_error_benchmarks(config.MUSBENCHMARKSE)
    
    log_message(f"Loaded {len(done)} done benchmarks, {len(err)} error benchmarks")
    
    # Write benchmark list
    try:
        with open(config.MUSBENCHMARKS, 'w') as f:
            for benchmark in benchmark_files:
                f.write(f"{benchmark}\n")
        log_message(f"Written {len(benchmark_files)} benchmarks to {config.MUSBENCHMARKS}")
    except Exception as e:
        log_message(f"Error writing benchmarks file: {e}", "ERROR")
        return
    
    # Process benchmarks
    total_benchmarks = len(benchmark_files)
    processed = 0
    new_successes = 0
    new_failures = 0
    skipped = 0
    
    for benchmark_file in benchmark_files:
        processed += 1
        
        # Skip if already processed
        if benchmark_file in done:
            log_message(f"[{processed}/{total_benchmarks}] SKIP: {benchmark_file} (already done)")
            skipped += 1
            continue
            
        if benchmark_file in err:
            log_message(f"[{processed}/{total_benchmarks}] SKIP: {benchmark_file} (previously failed)")
            skipped += 1
            continue
        
        # Process benchmark
        log_message(f"[{processed}/{total_benchmarks}] PROCESSING: {benchmark_file}")
        
        success = run_aaltaf_mus(benchmark_file, flags, timeout, memory_limit)
        
        # Update state
        if success:
            done.add(benchmark_file)
            new_successes += 1
            log_message(f"SUCCESS: {benchmark_file}")
        else:
            err.add(benchmark_file)
            new_failures += 1
            log_message(f"FAILED: {benchmark_file}")
        
        # Save state after each benchmark
        save_done_benchmarks(config.MUSBENCHMARKSD, done)
        save_error_benchmarks(config.MUSBENCHMARKSE, err)
    
    # Final statistics
    log_message("=== BENCHMARK PROCESSING COMPLETE ===")
    log_message(f"Total benchmarks: {total_benchmarks}")
    log_message(f"Skipped (already processed): {skipped}")
    log_message(f"Newly processed: {new_successes + new_failures}")
    log_message(f"New successes: {new_successes}")
    log_message(f"New failures: {new_failures}")
    
    if new_successes + new_failures > 0:
        log_message(f"Success rate (this run): {new_successes/(new_successes+new_failures)*100:.1f}%")
    
    log_message(f"Total accumulated - Successes: {len(done)}, Failures: {len(err)}")
    log_message(f"Overall success rate: {len(done)/(len(done)+len(err))*100:.1f}%")
    
    # Write final results files
    save_done_benchmarks(config.MUSBENCHMARKSD, done)
    save_error_benchmarks(config.MUSBENCHMARKSE, err)
    
    log_message(f"Done benchmarks written to: {config.MUSBENCHMARKSD}")
    log_message(f"Error benchmarks written to: {config.MUSBENCHMARKSE}")

def main():
    """Main execution"""
    parser = argparse.ArgumentParser(
        description='MUS benchmark runner',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    
    parser.add_argument('-d', '--directory', 
                       help='Directory containing .ltl benchmark files')
    parser.add_argument('-f', '--file',
                       help='Single .ltl file to process')
    parser.add_argument('-l', '--list',
                       help='File containing list of benchmarks to process')
    parser.add_argument('-t', '--timeout', type=int, default=config.DEFAULT_TIMEOUT,
                       help=f'Timeout per benchmark in seconds (default: {config.DEFAULT_TIMEOUT})')
    parser.add_argument('-m', '--memory-limit', type=int, default=None,
                       help='Virtual memory limit per benchmark in GB (default: 4)')
    parser.add_argument('-v', '--verbose', action='store_true',
                       help='Use verbose aaltaf flags')
    
    args = parser.parse_args()
    
    # Determine flags
    flags = config.VERBOSE_FLAGS if args.verbose else config.DEFAULT_FLAGS
    
    # Gather benchmark files
    benchmark_files = []
    
    if args.file:
        # Single file
        if not os.path.exists(args.file):
            log_message(f"File not found: {args.file}", "ERROR")
            sys.exit(1)
        benchmark_files = [os.path.abspath(args.file)]
        
    elif args.directory:
        # Directory of files
        if not os.path.isdir(args.directory):
            log_message(f"Directory not found: {args.directory}", "ERROR")
            sys.exit(1)
        benchmark_files = get_benchmark_files(args.directory)
        
    elif args.list:
        # List from file
        if not os.path.exists(args.list):
            log_message(f"List file not found: {args.list}", "ERROR")
            sys.exit(1)
        try:
            with open(args.list, 'r') as f:
                for line in f:
                    benchmark = line.strip()
                    if benchmark and os.path.exists(benchmark):
                        benchmark_files.append(os.path.abspath(benchmark))
        except Exception as e:
            log_message(f"Error reading list file: {e}", "ERROR")
            sys.exit(1)
    else:
        # Default to examples directory
        if not os.path.isdir(config.EXAMPLES_DIR):
            log_message(f"Default examples directory not found: {config.EXAMPLES_DIR}", "ERROR")
            sys.exit(1)
        benchmark_files = get_benchmark_files(config.EXAMPLES_DIR)
    
    if not benchmark_files:
        log_message("No benchmark files found", "ERROR")
        sys.exit(1)
    
    log_message(f"Found {len(benchmark_files)} benchmark files")
    
    # Execute benchmarks
    memory_limit = args.memory_limit * 1024**3 if args.memory_limit else None
    try:
        run_mus_benchmarks(benchmark_files, flags, args.timeout, memory_limit)
    except KeyboardInterrupt:
        log_message("Benchmark processing interrupted by user", "WARN")
        sys.exit(1)
    except Exception as e:
        log_message(f"Unexpected error: {e}", "ERROR")
        sys.exit(1)

if __name__ == "__main__":
    main()