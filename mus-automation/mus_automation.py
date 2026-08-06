#!/usr/bin/env python3
"""
MUS Automation System
"""

import os
import sys
import argparse
import subprocess
from typing import List, Dict

# Add scripts to path
SCRIPTS_DIR = os.path.join(os.path.dirname(__file__), 'scripts')
sys.path.append(SCRIPTS_DIR)

# Add config to path
CONFIG_DIR = os.path.join(os.path.dirname(__file__), 'config')
sys.path.append(CONFIG_DIR)

import config
from utils import log_message, setup_directories

def run_tests(args) -> int:
    """Run MUS tests"""
    cmd = [sys.executable, os.path.join(SCRIPTS_DIR, 'run_mus_tests.py')]
    
    if args.directory:
        cmd.extend(['-d', args.directory])
    if args.file:
        cmd.extend(['-f', args.file])
    if args.list:
        cmd.extend(['-l', args.list])
    if args.timeout:
        cmd.extend(['-t', str(args.timeout)])
    if args.memory_limit:
        cmd.extend(['-m', str(args.memory_limit)])
    if args.retry_failed:
        cmd.append('--retry-failed')
    if args.verbose:
        cmd.append('-v')
    
    log_message("Starting MUS benchmark execution")
    result = subprocess.run(cmd)
    
    if result.returncode == 0:
        log_message("Benchmark execution completed successfully")
    else:
        log_message("Benchmark execution failed", "ERROR")
    
    return result.returncode

def analyze_results(args) -> int:
    """Analyze MUS results"""
    
    # If using advanced analysis, delegate to analyze_results.py
    if args.plots or args.csv or args.detailed:
        return run_advanced_analysis(args)
    
    # Check if we have output files to analyze
    if not os.path.exists(config.MUSBENCHMARKSD):
        log_message("No completed benchmarks found for analysis", "ERROR")
        return 1
    
    # Load completed benchmarks
    try:
        from utils import load_done_benchmarks, parse_mus_output_from_file
        
        done_benchmarks = load_done_benchmarks(config.MUSBENCHMARKSD)
        log_message(f"Analyzing {len(done_benchmarks)} completed benchmarks")
        
        # Collect analysis data
        analysis_data = {
            'meta': {
                'total_benchmarks': len(done_benchmarks),
                'analysis_timestamp': None
            },
            'benchmarks': {},
            'aggregate_stats': {}
        }
        
        successful_parses = 0
        
        for benchmark_file in done_benchmarks:
            output_file = benchmark_file + config.OUTPUT_EXTENSION
            parsed_data = parse_mus_output_from_file(output_file)
            
            if parsed_data:
                analysis_data['benchmarks'][benchmark_file] = parsed_data
                successful_parses += 1
            else:
                log_message(f"Failed to parse: {benchmark_file}", "WARN")
        
        # Generate aggregate statistics
        if successful_parses > 0:
            analysis_data['aggregate_stats'] = generate_aggregate_stats(analysis_data['benchmarks'])
        
        analysis_data['meta']['successful_parses'] = successful_parses
        analysis_data['meta']['analysis_timestamp'] = subprocess.check_output(['date']).decode().strip()
        
        # Save analysis results
        import json
        with open(config.ANALYSIS_RESULTS, 'w') as f:
            json.dump(analysis_data, f, indent=2)
        
        log_message(f"Analysis results written to: {config.ANALYSIS_RESULTS}")
        log_message(f"Successfully analyzed: {successful_parses}/{len(done_benchmarks)} benchmarks")
        
        # Generate summary report
        if args.report:
            generate_summary_report(analysis_data)
        
        return 0
        
    except Exception as e:
        log_message(f"Analysis failed: {e}", "ERROR")
        return 1

def run_advanced_analysis(args) -> int:
    """Run advanced analysis using analyze_results.py"""
    if not os.path.exists(config.ANALYSIS_RESULTS):
        log_message("No analysis results found. Run basic analysis first.", "ERROR")
        return 1
    
    cmd = [sys.executable, os.path.join(SCRIPTS_DIR, 'analyze_results.py'), config.ANALYSIS_RESULTS]
    
    if args.output_dir:
        cmd.extend(['-o', args.output_dir])
    else:
        cmd.extend(['-o', config.RESULTS_DIR])
    
    if args.plots:
        cmd.append('--plots')
    if args.csv:
        cmd.append('--csv')
    if args.detailed:
        cmd.append('--report')
    
    log_message("Running advanced analysis")
    result = subprocess.run(cmd)
    
    if result.returncode == 0:
        log_message("Advanced analysis completed successfully")
    else:
        log_message("Advanced analysis failed", "ERROR")
    
    return result.returncode

def generate_aggregate_stats(benchmark_data: Dict) -> Dict:
    """Generate aggregate statistics"""
    
    total_muses = 0
    total_boolean_calls = 0
    total_ltlf_creations = 0
    total_enumeration_time = 0.0
    
    mus_counts = []
    mus_sizes = []
    enumeration_times = []
    
    for benchmark_file, data in benchmark_data.items():
        if 'muses' in data:
            mus_count = len(data['muses'])
            mus_counts.append(mus_count)
            total_muses += mus_count
            
            # Individual MUS sizes
            for mus in data['muses']:
                content_parts = mus.get('content', '').strip().split()
                if content_parts:
                    mus_sizes.append(len(content_parts))
        
        if 'stats' in data:
            total_boolean_calls += data['stats'].get('total_boolean_calls', 0)
            total_ltlf_creations += data['stats'].get('total_ltlf_creations', 0)
        
        if 'timing' in data and 'total_enumeration_time' in data['timing']:
            enum_time = data['timing']['total_enumeration_time']
            enumeration_times.append(enum_time)
            total_enumeration_time += enum_time
    
    # Aggregate calculations
    aggregate = {
        'total_benchmarks': len(benchmark_data),
        'total_muses': total_muses,
        'total_boolean_calls': total_boolean_calls,
        'total_ltlf_creations': total_ltlf_creations,
        'total_enumeration_time': total_enumeration_time
    }
    
    if mus_counts:
        aggregate['mus_count_stats'] = {
            'min': min(mus_counts),
            'max': max(mus_counts),
            'mean': sum(mus_counts) / len(mus_counts)
        }
    
    if mus_sizes:
        aggregate['mus_size_stats'] = {
            'min': min(mus_sizes),
            'max': max(mus_sizes),
            'mean': sum(mus_sizes) / len(mus_sizes)
        }
    
    if enumeration_times:
        aggregate['enumeration_time_stats'] = {
            'min': min(enumeration_times),
            'max': max(enumeration_times),
            'mean': sum(enumeration_times) / len(enumeration_times),
            'total': total_enumeration_time
        }
    
    return aggregate

def generate_summary_report(analysis_data: Dict):
    """Generate summary report"""
    
    meta = analysis_data['meta']
    aggregates = analysis_data.get('aggregate_stats', {})
    
    print("\n" + "="*60)  
    print("MUS ANALYSIS SUMMARY")
    print("="*60)
    print(f"Analysis timestamp: {meta.get('analysis_timestamp', 'Unknown')}")
    print(f"Total benchmarks: {meta['total_benchmarks']}")
    print(f"Successfully parsed: {meta['successful_parses']}")
    print(f"Parse success rate: {meta['successful_parses']/meta['total_benchmarks']*100:.1f}%")
    
    if aggregates:
        print(f"\nAGGREGATE STATISTICS:")
        print(f"Total MUSes found: {aggregates.get('total_muses', 0)}")
        print(f"Total Boolean calls: {aggregates.get('total_boolean_calls', 0)}")
        print(f"Total LTLf creations: {aggregates.get('total_ltlf_creations', 0)}")
        print(f"Total enumeration time: {aggregates.get('total_enumeration_time', 0):.6f}s")
        
        if 'mus_count_stats' in aggregates:
            mcs = aggregates['mus_count_stats']
            print(f"MUS counts - Min: {mcs['min']}, Max: {mcs['max']}, Mean: {mcs['mean']:.2f}")
        
        if 'mus_size_stats' in aggregates:
            mss = aggregates['mus_size_stats']
            print(f"MUS sizes - Min: {mss['min']}, Max: {mss['max']}, Mean: {mss['mean']:.2f}")
    
    print("="*60)

def clean_environment(args) -> bool:
    """Clean benchmark tracking files"""
    log_message("Cleaning benchmark tracking files")
    
    files_to_clean = [
        (config.MUSBENCHMARKSD, "done benchmarks"),
        (config.MUSBENCHMARKSE, "error benchmarks"), 
        (config.MUSBENCHMARKS, "benchmark list"),
        (config.ANALYSIS_RESULTS, "analysis results")
    ]
    
    cleaned_files = 0
    
    for file_path, description in files_to_clean:
        if os.path.exists(file_path):
            try:
                os.remove(file_path)
                log_message(f"✓ Removed {description}: {file_path}")
                cleaned_files += 1
            except OSError as e:
                log_message(f"Failed to remove {description}: {e}", "ERROR")
                return False
        else:
            log_message(f"- {description} not found: {file_path}")
    
    # Clean output files if requested
    if args.output:
        if os.path.exists(config.EXAMPLES_DIR):
            from utils import get_benchmark_files
            benchmark_files = get_benchmark_files(config.EXAMPLES_DIR)
            
            output_files_removed = 0
            for benchmark_file in benchmark_files:
                output_file = benchmark_file + config.OUTPUT_EXTENSION
                if os.path.exists(output_file):
                    try:
                        os.remove(output_file)
                        output_files_removed += 1
                    except OSError as e:
                        log_message(f"Failed to remove output file {output_file}: {e}", "ERROR")
            
            if output_files_removed > 0:
                log_message(f"✓ Removed {output_files_removed} output files")
            else:
                log_message("- No output files found to remove")
    
    log_message(f"Clean complete! Removed {cleaned_files} tracking files")
    log_message("Next test run will process all benchmarks from scratch")
    
    return True

def setup_environment(args) -> bool:
    """Setup MUS automation environment"""
    log_message("Setting up MUS automation environment")
    
    # Check aaltaf binary
    if not os.path.exists(config.AALTAF_BIN):
        log_message(f"aaltaf binary not found at {config.AALTAF_BIN}", "ERROR")
        log_message("Please build aaltaf using 'make' command", "ERROR")
        return False
    
    log_message(f"✓ aaltaf binary found at {config.AALTAF_BIN}")
    
    # Setup directories
    setup_directories()
    log_message("✓ Directory structure created")
    
    # Check examples directory
    if os.path.exists(config.EXAMPLES_DIR):
        from utils import get_benchmark_files
        benchmark_files = get_benchmark_files(config.EXAMPLES_DIR)
        log_message(f"✓ Found {len(benchmark_files)} .ltl files in examples directory")
    else:
        log_message(f"Warning: Examples directory not found at {config.EXAMPLES_DIR}", "WARN")
    
    # Check dependencies
    try:
        import resource
        log_message("✓ resource module available for memory limiting")
    except ImportError:
        log_message("Warning: resource module not available", "WARN")
    
    log_message("Environment setup complete!")
    
    return True

def main():
    parser = argparse.ArgumentParser(
        description='MUS Automation System',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Setup environment
  python mus_automation.py setup
  
  # Clean tracking files (reset benchmark state)
  python mus_automation.py clean
  
  # Clean tracking files and output files
  python mus_automation.py clean --output
  
  # Run benchmarks on directory
  python mus_automation.py test -d examples
  
  # Run single benchmark
  python mus_automation.py test -f examples/p11.ltl
  
  # Run from benchmark list file
  python mus_automation.py test -l benchmarks.txt
  
  # Analyze results with summary
  python mus_automation.py analyze --report
  
  # Analyze with plots and CSV export
  python mus_automation.py analyze --plots --csv --detailed
        """
    )
    
    subparsers = parser.add_subparsers(dest='command', help='Available commands')
    
    # Setup command
    setup_parser = subparsers.add_parser('setup', help='Setup environment')
    
    # Clean command
    clean_parser = subparsers.add_parser('clean', help='Clean benchmark tracking files')
    clean_parser.add_argument('--output', action='store_true', help='Also remove output files (*_out)')
    
    # Test command
    test_parser = subparsers.add_parser('test', help='Run benchmarks')
    test_parser.add_argument('-d', '--directory', help='Directory containing .ltl files')
    test_parser.add_argument('-f', '--file', help='Single .ltl file to test')
    test_parser.add_argument('-l', '--list', help='File containing benchmark list')
    test_parser.add_argument('-t', '--timeout', type=int, help='Timeout per benchmark (seconds)')
    test_parser.add_argument('-m', '--memory-limit', type=int, help='Virtual memory limit per benchmark (GB, default: 4)')
    test_parser.add_argument('--retry-failed', action='store_true', help='Re-run previously failed benchmarks instead of skipping them')
    test_parser.add_argument('-v', '--verbose', action='store_true', help='Use verbose flags')
    
    # Analyze command
    analyze_parser = subparsers.add_parser('analyze', help='Analyze benchmark results')
    analyze_parser.add_argument('--report', action='store_true', help='Generate summary report')
    analyze_parser.add_argument('--plots', action='store_true', help='Generate visualization plots')
    analyze_parser.add_argument('--csv', action='store_true', help='Export results to CSV')
    analyze_parser.add_argument('--detailed', action='store_true', help='Generate detailed JSON statistics report')
    analyze_parser.add_argument('-o', '--output-dir', help='Output directory for analysis files')
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return 1
    
    if args.command == 'setup':
        success = setup_environment(args)
        return 0 if success else 1
    elif args.command == 'clean':
        success = clean_environment(args)
        return 0 if success else 1
    elif args.command == 'test':
        return run_tests(args)
    elif args.command == 'analyze':
        return analyze_results(args)
    else:
        parser.print_help()
        return 1

if __name__ == "__main__":
    sys.exit(main())