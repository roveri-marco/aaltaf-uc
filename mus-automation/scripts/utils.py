#!/usr/bin/env python3
"""
MUS automation testing utilities
"""

import os
import sys
import time
import json
import subprocess
import resource
from datetime import datetime
from typing import Dict, List, Tuple, Optional

# Add config to path
sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'config'))
import config

# Global flag to track memory limit capability
_memory_limit_available = None

def test_memory_limit():
    """Test if memory limit can be set and return capability flag"""
    global _memory_limit_available
    
    if _memory_limit_available is not None:
        return _memory_limit_available
    
    try:
        # Get current limit and test setting our target limit
        current_limit = resource.getrlimit(resource.RLIMIT_AS)
        
        # Try to set our target limit
        resource.setrlimit(resource.RLIMIT_AS, (config.MAX_VIRTUAL_MEMORY, resource.RLIM_INFINITY))
        
        # Restore original limit
        resource.setrlimit(resource.RLIMIT_AS, current_limit)
        
        _memory_limit_available = True
        return True
    except (OSError, ValueError) as e:
        log_message(f"WARNING: Memory limit bypassed due to OS compatibility: {e}", "WARN")
        _memory_limit_available = False
        return False

def limit_virtual_memory():
    """Set memory limit for subprocess execution"""
    try:
        resource.setrlimit(resource.RLIMIT_AS, (config.MAX_VIRTUAL_MEMORY, resource.RLIM_INFINITY))
    except (OSError, ValueError):
        # Silent fail: warning already shown in main process
        pass

def setup_directories():
    """Create necessary directories if they don't exist"""
    dirs = [
        config.RESULTS_DIR,
        config.PLOTS_DIR, 
        config.BENCHMARKS_DIR
    ]
    
    for directory in dirs:
        os.makedirs(directory, exist_ok=True)

def run_aaltaf_mus(benchmark_file: str, flags: List[str] = None, timeout: int = None) -> bool:
    """
    Run aaltaf MUS test on benchmark file
    
    Args:
        benchmark_file: Path to .ltl file
        flags: Command flags (default: ["-emus2"])
        timeout: Timeout in seconds
        
    Returns:
        bool: True if successful, False otherwise
    """
    if flags is None:
        flags = ["-emus2"]
    
    if timeout is None:
        timeout = config.DEFAULT_TIMEOUT
    
    # Output file
    output_file = benchmark_file + "_out"
    
    # Build command
    cmd = [config.AALTAF_BIN, "-f", benchmark_file] + flags
    
    # Log execution start
    print(f"Running: {' '.join(cmd)}")
    
    # Test memory limit capability in main process (shows warning if needed)
    memory_limit_works = test_memory_limit()
    
    try:
        # Execute with or without memory limit based on capability
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=timeout,
            preexec_fn=limit_virtual_memory if memory_limit_works else None
        )
        
        # Write output to file
        with open(output_file, 'w') as f:
            f.write(result.stdout)
            if result.stderr:
                f.write("\n--- STDERR ---\n")
                f.write(result.stderr)
        
        # Handle return code
        if result.returncode != 0:
            print(f"WARNING: {benchmark_file} returned code {result.returncode}")
            with open(output_file, 'a') as f:
                f.write(f"\n--- RETURN CODE: {result.returncode} ---\n")
            return False
        
        print(f"SUCCESS: {benchmark_file}")
        return True
        
    except subprocess.TimeoutExpired:
        # Timeout handling
        print(f"TIMEOUT: {benchmark_file} after {timeout}s")
        with open(output_file, 'w') as f:
            f.write(f"TIMEOUT after {timeout} seconds\n")
        return False
        
    except OSError as e:
        # OS error handling
        print(f"OS ERROR: {benchmark_file} - {str(e)}")
        with open(output_file, 'w') as f:
            f.write(f"OS ERROR: {str(e)}\n")
        return False
        
    except Exception as e:
        # Generic error handling
        print(f"ERROR: {benchmark_file} - {str(e)}")
        with open(output_file, 'w') as f:
            f.write(f"GENERIC ERROR: {str(e)}\n")
        return False

def parse_mus_output_from_file(output_file: str) -> Optional[Dict]:
    """
    Parse MUS output from file
    
    Args:
        output_file: Path to output file
        
    Returns:
        Dict with parsed data or None if parsing failed
    """
    try:
        if not os.path.exists(output_file):
            return None
            
        with open(output_file, 'r') as f:
            output = f.read()
        
        # Check for timeout or error markers
        if "TIMEOUT" in output or "ERROR" in output:
            return None
            
        return parse_mus_output(output)
        
    except Exception as e:
        print(f"Error parsing output file {output_file}: {e}")
        return None

def parse_mus_output(output: str) -> Optional[Dict]:
    """Parse the output from aaltaf -emus2 command"""
    try:
        data = {
            'muses': [],
            'stats': {},
            'timing': {}
        }
        
        lines = output.strip().split('\n')
        
        # Parse individual MUS entries
        in_mus_section = False
        for line in lines:
            line = line.strip()
            
            if "====== MUSes Summary ======" in line:
                in_mus_section = True
                continue
            
            if "====== Total Statistics ======" in line:
                in_mus_section = False
                continue
            
            if in_mus_section and line and not line.startswith("MUS #"):
                # Parse MUS entry: MUS# Checker_Creation Check_Time MUS_Extraction Bool_Calls LTLf_Creations Content
                parts = line.split()
                if len(parts) >= 7:
                    try:
                        mus_entry = {
                            'id': int(parts[0]),
                            'checker_creation_time': float(parts[1]),
                            'check_time': float(parts[2]),
                            'extraction_time': float(parts[3]),
                            'bool_calls': int(parts[4]),
                            'ltlf_creations': int(parts[5]),
                            'content': ' '.join(parts[6:])
                        }
                        data['muses'].append(mus_entry)
                    except (ValueError, IndexError):
                        continue
        
        # Parse total statistics with defensive coding
        for line in lines:
            line = line.strip()
            
            try:
                if "Total Boolean Solver Calls:" in line:
                    data['stats']['total_boolean_calls'] = int(line.split(':')[1].strip())
                elif "Total LTLf Checker Creations:" in line:
                    data['stats']['total_ltlf_creations'] = int(line.split(':')[1].strip())
                elif line.startswith("Count:") and 'mus_size_stats' not in data['stats']:
                    data['stats']['mus_size_stats'] = {}
                    data['stats']['mus_size_stats']['count'] = int(line.split(':')[1].strip())
                elif line.startswith("Min Size:"):
                    data['stats']['mus_size_stats']['min_size'] = int(line.split(':')[1].strip())
                elif line.startswith("Max Size:"):
                    data['stats']['mus_size_stats']['max_size'] = int(line.split(':')[1].strip())
                elif line.startswith("Mean Size:"):
                    data['stats']['mus_size_stats']['mean_size'] = float(line.split(':')[1].strip())
                elif line.startswith("Variance:") and 'mus_size_stats' in data['stats']:
                    data['stats']['mus_size_stats']['variance'] = float(line.split(':')[1].strip())
                elif "Enumeration of mus time:" in line:
                    data['timing']['total_enumeration_time'] = float(line.split(':')[1].strip())
                elif "Parsing of the file time:" in line:
                    data['timing']['parsing_time'] = float(line.split(':')[1].strip())
                elif "Preprocessing time:" in line:
                    data['timing']['preprocessing_time'] = float(line.split(':')[1].strip())
            except (ValueError, IndexError, KeyError):
                # Skip lines that can't be parsed - defensive
                continue
        
        return data
        
    except Exception as e:
        print(f"Error parsing MUS output: {e}")
        return None

def get_benchmark_files(directory: str) -> List[str]:
    """Get list of .ltl files from directory"""
    ltl_files = []
    
    try:
        if os.path.isdir(directory):
            for filename in os.listdir(directory):
                if filename.endswith('.ltl'):
                    full_path = os.path.abspath(os.path.join(directory, filename))
                    ltl_files.append(full_path)
    except OSError as e:
        print(f"Error reading directory {directory}: {e}")
        return []
    
    ltl_files.sort()
    return ltl_files

def load_done_benchmarks(done_file: str) -> set:
    """Load completed benchmarks from file"""
    done = set()
    try:
        if os.path.exists(done_file):
            with open(done_file, 'r') as f:
                for line in f:
                    benchmark = line.strip()
                    if benchmark:
                        done.add(benchmark)
    except Exception as e:
        print(f"Error loading done benchmarks: {e}")
    
    return done

def save_done_benchmarks(done_file: str, done_set: set):
    """Save completed benchmarks to file"""
    try:
        with open(done_file, 'w') as f:
            for benchmark in sorted(done_set):
                f.write(f"{benchmark}\n")
    except Exception as e:
        print(f"Error saving done benchmarks: {e}")

def load_error_benchmarks(error_file: str) -> set:
    """Load error benchmarks from file"""
    errors = set()
    try:
        if os.path.exists(error_file):
            with open(error_file, 'r') as f:
                for line in f:
                    benchmark = line.strip()
                    if benchmark:
                        errors.add(benchmark)
    except Exception as e:
        print(f"Error loading error benchmarks: {e}")
    
    return errors

def save_error_benchmarks(error_file: str, error_set: set):
    """Save error benchmarks to file"""
    try:
        with open(error_file, 'w') as f:
            for benchmark in sorted(error_set):
                f.write(f"{benchmark}\n")
    except Exception as e:
        print(f"Error saving error benchmarks: {e}")

def log_message(message: str, level: str = "INFO"):
    """Log message with timestamp"""
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"[{timestamp}] {level}: {message}")

def format_time(seconds: float) -> str:
    """Format time in human readable format"""
    if seconds < 1:
        return f"{seconds*1000:.2f}ms"
    elif seconds < 60:
        return f"{seconds:.2f}s"
    else:
        minutes = int(seconds // 60)
        seconds = seconds % 60
        return f"{minutes}m{seconds:.2f}s"

def load_results(results_file: str) -> List[Dict]:
    """Load results from JSON file"""
    try:
        with open(results_file, 'r') as f:
            data = json.load(f)
        
        # Handle different JSON formats
        if isinstance(data, list):
            # Already in expected format
            return data
        elif isinstance(data, dict) and 'benchmarks' in data:
            # Convert from benchmarks format to list format
            results = []
            for filename, benchmark_data in data['benchmarks'].items():
                result = {
                    'filename': filename,
                    'success': True,
                    'parse_error': False,
                    'execution_time': benchmark_data.get('timing', {}).get('total_enumeration_time', 0)
                }
                result.update(benchmark_data)
                results.append(result)
            return results
        else:
            log_message(f"Unknown JSON format in {results_file}", "ERROR")
            return []
            
    except FileNotFoundError:
        log_message(f"Results file {results_file} not found", "ERROR")
        return []
    except json.JSONDecodeError as e:
        log_message(f"Error parsing JSON from {results_file}: {e}", "ERROR")
        return []
    except Exception as e:
        log_message(f"Error loading results from {results_file}: {e}", "ERROR")
        return []