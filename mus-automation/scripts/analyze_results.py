#!/usr/bin/env python3
"""
Analysis and reporting script for MUS test results
"""

import os
import sys
import json
import csv
import argparse
import statistics
from datetime import datetime
from typing import Dict, List, Tuple
import matplotlib.pyplot as plt
import numpy as np

# Add config to path
sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'config'))
import config
from utils import load_results, log_message, format_time

class MUSResultAnalyzer:
    def __init__(self, results_file: str):
        self.results_file = results_file
        self.results = load_results(results_file)
        
        if not self.results:
            raise ValueError(f"No results found in {results_file}")
        
        # Filter successful results
        self.successful_results = [r for r in self.results 
                                 if r.get('success', False) and not r.get('parse_error', False)]
        
        log_message(f"Loaded {len(self.results)} total results, {len(self.successful_results)} successful")
    
    def generate_statistics_report(self) -> Dict:
        """Generate comprehensive statistics report"""
        report = {
            'meta': {
                'analysis_timestamp': datetime.now().isoformat(),
                'source_file': self.results_file,
                'total_tests': len(self.results),
                'successful_tests': len(self.successful_results)
            },
            'execution_statistics': {},
            'mus_statistics': {},
            'performance_metrics': {},
            'file_breakdown': []
        }
        
        # Execution statistics
        execution_times = [r['execution_time'] for r in self.results]
        successful_times = [r['execution_time'] for r in self.successful_results]
        
        report['execution_statistics'] = {
            'total_execution_time': sum(execution_times),
            'mean_execution_time': statistics.mean(execution_times),
            'median_execution_time': statistics.median(execution_times),
            'successful_mean_time': statistics.mean(successful_times) if successful_times else 0,
            'min_time': min(execution_times),
            'max_time': max(execution_times),
            'std_dev_time': statistics.stdev(execution_times) if len(execution_times) > 1 else 0
        }
        
        # MUS-specific statistics
        if self.successful_results:
            mus_report = self._analyze_mus_data()
            report['mus_statistics'] = mus_report
        
        # Performance metrics
        report['performance_metrics'] = self._analyze_performance()
        
        # Per-file breakdown
        report['file_breakdown'] = self._generate_file_breakdown()
        
        return report
    
    def _analyze_mus_data(self) -> Dict:
        """Analyze MUS-specific data"""
        mus_counts = []
        mus_sizes = []
        boolean_calls = []
        ltlf_creations = []
        enumeration_times = []
        
        checker_creation_times = []
        check_times = []
        extraction_times = []
        
        for result in self.successful_results:
            if 'muses' in result and result['muses']:
                mus_count = len(result['muses'])
                mus_counts.append(mus_count)
                
                # Individual MUS data
                for mus in result['muses']:
                    # Extract MUS size from content
                    content_parts = mus.get('content', '').strip().split()
                    if content_parts:
                        mus_sizes.append(len(content_parts))
                    
                    # Performance data
                    checker_creation_times.append(mus.get('checker_creation_time', 0))
                    check_times.append(mus.get('check_time', 0))
                    extraction_times.append(mus.get('extraction_time', 0))
            
            # Aggregate statistics
            if 'stats' in result:
                stats = result['stats']
                boolean_calls.append(stats.get('total_boolean_calls', 0))
                ltlf_creations.append(stats.get('total_ltlf_creations', 0))
            
            if 'timing' in result and 'total_enumeration_time' in result['timing']:
                enumeration_times.append(result['timing']['total_enumeration_time'])
        
        mus_stats = {}
        
        # MUS count statistics
        if mus_counts:
            mus_stats['mus_counts'] = {
                'min': min(mus_counts),
                'max': max(mus_counts),
                'mean': statistics.mean(mus_counts),
                'median': statistics.median(mus_counts),
                'std_dev': statistics.stdev(mus_counts) if len(mus_counts) > 1 else 0,
                'distribution': self._count_distribution(mus_counts)
            }
        
        # MUS size statistics
        if mus_sizes:
            mus_stats['mus_sizes'] = {
                'min': min(mus_sizes),
                'max': max(mus_sizes),
                'mean': statistics.mean(mus_sizes),
                'median': statistics.median(mus_sizes),
                'std_dev': statistics.stdev(mus_sizes) if len(mus_sizes) > 1 else 0,
                'distribution': self._count_distribution(mus_sizes)
            }
        
        # Performance statistics
        for name, times in [
            ('checker_creation', checker_creation_times),
            ('check', check_times),
            ('extraction', extraction_times)
        ]:
            if times:
                mus_stats[f'{name}_times'] = {
                    'min': min(times),
                    'max': max(times),
                    'mean': statistics.mean(times),
                    'median': statistics.median(times),
                    'std_dev': statistics.stdev(times) if len(times) > 1 else 0
                }
        
        # Solver statistics
        if boolean_calls:
            mus_stats['boolean_calls'] = {
                'total': sum(boolean_calls),
                'mean_per_file': statistics.mean(boolean_calls),
                'max_per_file': max(boolean_calls)
            }
        
        if ltlf_creations:
            mus_stats['ltlf_creations'] = {
                'total': sum(ltlf_creations),
                'mean_per_file': statistics.mean(ltlf_creations),
                'max_per_file': max(ltlf_creations)
            }
        
        if enumeration_times:
            mus_stats['enumeration_times'] = {
                'total': sum(enumeration_times),
                'mean': statistics.mean(enumeration_times),
                'median': statistics.median(enumeration_times),
                'max': max(enumeration_times)
            }
        
        return mus_stats
    
    def _analyze_performance(self) -> Dict:
        """Analyze performance characteristics"""
        performance = {
            'success_rate': len(self.successful_results) / len(self.results),
            'failure_analysis': {},
            'timeout_analysis': {},
            'efficiency_metrics': {}
        }
        
        # Failure analysis
        failed_results = [r for r in self.results if not r.get('success', False)]
        timeout_results = [r for r in failed_results if 'TIMEOUT' in r.get('raw_output', '')]
        error_results = [r for r in failed_results if 'TIMEOUT' not in r.get('raw_output', '')]
        
        performance['failure_analysis'] = {
            'total_failures': len(failed_results),
            'timeouts': len(timeout_results),
            'errors': len(error_results),
            'timeout_rate': len(timeout_results) / len(self.results)
        }
        
        # Efficiency metrics
        if self.successful_results:
            # Files with most MUSes
            mus_counts = []
            for result in self.successful_results:
                if 'muses' in result:
                    mus_counts.append((result['filename'], len(result['muses'])))
            
            mus_counts.sort(key=lambda x: x[1], reverse=True)
            
            performance['efficiency_metrics'] = {
                'top_mus_producers': mus_counts[:10],
                'files_with_no_mus': len([r for r in self.successful_results 
                                        if not r.get('muses', [])]),
                'mean_muses_per_file': statistics.mean([len(r.get('muses', [])) 
                                                      for r in self.successful_results])
            }
        
        return performance
    
    def _generate_file_breakdown(self) -> List[Dict]:
        """Generate per-file breakdown"""
        breakdown = []
        
        for result in self.results:
            file_data = {
                'filename': result.get('filename', 'unknown'),
                'success': result.get('success', False),
                'execution_time': result.get('execution_time', 0),
                'mus_count': len(result.get('muses', [])),
                'status': 'success' if result.get('success', False) else 'failed'
            }
            
            if result.get('success', False) and 'stats' in result:
                stats = result['stats']
                file_data.update({
                    'boolean_calls': stats.get('total_boolean_calls', 0),
                    'ltlf_creations': stats.get('total_ltlf_creations', 0)
                })
            
            if not result.get('success', False):
                if 'TIMEOUT' in result.get('raw_output', ''):
                    file_data['status'] = 'timeout'
                else:
                    file_data['status'] = 'error'
            
            breakdown.append(file_data)
        
        return sorted(breakdown, key=lambda x: x['execution_time'], reverse=True)
    
    def _count_distribution(self, values: List[int]) -> Dict:
        """Generate distribution of integer values"""
        from collections import Counter
        counter = Counter(values)
        total = len(values)
        
        distribution = {}
        for value, count in sorted(counter.items()):
            distribution[str(value)] = {
                'count': count,
                'percentage': (count / total) * 100
            }
        
        return distribution
    
    def save_csv_report(self, output_file: str):
        """Save results as CSV for further analysis"""
        with open(output_file, 'w', newline='') as csvfile:
            fieldnames = [
                'filename', 'success', 'execution_time', 'mus_count',
                'boolean_calls', 'ltlf_creations', 'status',
                'enumeration_time', 'parsing_time', 'preprocessing_time'
            ]
            
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            writer.writeheader()
            
            for result in self.results:
                row = {
                    'filename': result.get('filename', ''),
                    'success': result.get('success', False),
                    'execution_time': result.get('execution_time', 0),
                    'mus_count': len(result.get('muses', [])),
                    'boolean_calls': result.get('stats', {}).get('total_boolean_calls', 0),
                    'ltlf_creations': result.get('stats', {}).get('total_ltlf_creations', 0),
                    'enumeration_time': result.get('timing', {}).get('total_enumeration_time', 0),
                    'parsing_time': result.get('timing', {}).get('parsing_time', 0),
                    'preprocessing_time': result.get('timing', {}).get('preprocessing_time', 0)
                }
                
                # Determine status
                if result.get('success', False):
                    row['status'] = 'success'
                elif 'TIMEOUT' in result.get('raw_output', ''):
                    row['status'] = 'timeout'
                else:
                    row['status'] = 'error'
                
                writer.writerow(row)
        
        log_message(f"CSV report saved to {output_file}")
    
    def generate_plots(self, output_dir: str):
        """Generate various plots for visualization"""
        os.makedirs(output_dir, exist_ok=True)
        
        if not self.successful_results:
            log_message("No successful results to plot", "WARN")
            return
        
        # Plot 1: MUS count distribution
        self._plot_mus_count_distribution(output_dir)
        
        # Plot 2: Execution time vs MUS count
        self._plot_execution_vs_mus_count(output_dir)
        
        # Plot 3: MUS size distribution
        self._plot_mus_size_distribution(output_dir)
        
        # Plot 4: Performance metrics
        self._plot_performance_metrics(output_dir)
        
        log_message(f"Plots saved to {output_dir}")
    
    def _plot_mus_count_distribution(self, output_dir: str):
        """Plot distribution of MUS counts per file"""
        mus_counts = [len(r.get('muses', [])) for r in self.successful_results]
        
        plt.figure(figsize=config.PLOT_CONFIG['figsize'])
        plt.hist(mus_counts, bins=max(10, len(set(mus_counts))), 
                edgecolor='black', alpha=0.7)
        plt.xlabel('Number of MUSes per File')
        plt.ylabel('Frequency')
        plt.title('Distribution of MUS Counts')
        plt.grid(True, alpha=0.3)
        
        # Add statistics
        mean_mus = statistics.mean(mus_counts)
        plt.axvline(mean_mus, color='red', linestyle='--', 
                   label=f'Mean: {mean_mus:.2f}')
        plt.legend()
        
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, 'mus_count_distribution.png'), 
                   dpi=config.PLOT_CONFIG['dpi'])
        plt.close()
    
    def _plot_execution_vs_mus_count(self, output_dir: str):
        """Plot execution time vs MUS count"""
        exec_times = []
        mus_counts = []
        
        for result in self.successful_results:
            exec_times.append(result['execution_time'])
            mus_counts.append(len(result.get('muses', [])))
        
        plt.figure(figsize=config.PLOT_CONFIG['figsize'])
        plt.scatter(mus_counts, exec_times, alpha=0.6, s=50)
        plt.xlabel('Number of MUSes')
        plt.ylabel('Execution Time (seconds)')
        plt.title('Execution Time vs MUS Count')
        plt.grid(True, alpha=0.3)
        
        # Add trend line
        if len(mus_counts) > 1:
            z = np.polyfit(mus_counts, exec_times, 1)
            p = np.poly1d(z)
            plt.plot(sorted(mus_counts), p(sorted(mus_counts)), "r--", alpha=0.8)
        
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, 'execution_vs_mus_count.png'), 
                   dpi=config.PLOT_CONFIG['dpi'])
        plt.close()
    
    def _plot_mus_size_distribution(self, output_dir: str):
        """Plot distribution of individual MUS sizes"""
        mus_sizes = []
        
        for result in self.successful_results:
            for mus in result.get('muses', []):
                content_parts = mus.get('content', '').strip().split()
                if content_parts:
                    mus_sizes.append(len(content_parts))
        
        if not mus_sizes:
            return
        
        plt.figure(figsize=config.PLOT_CONFIG['figsize'])
        plt.hist(mus_sizes, bins=max(5, len(set(mus_sizes))), 
                edgecolor='black', alpha=0.7)
        plt.xlabel('MUS Size (number of propositions)')
        plt.ylabel('Frequency')
        plt.title('Distribution of Individual MUS Sizes')
        plt.grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, 'mus_size_distribution.png'), 
                   dpi=config.PLOT_CONFIG['dpi'])
        plt.close()
    
    def _plot_performance_metrics(self, output_dir: str):
        """Plot performance metrics comparison"""
        # Success rate pie chart
        successful = len(self.successful_results)
        failed = len(self.results) - successful
        
        fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(15, 12))
        
        # Success rate
        ax1.pie([successful, failed], labels=['Successful', 'Failed'], 
                autopct='%1.1f%%', colors=['#2ca02c', '#d62728'])
        ax1.set_title('Overall Success Rate')
        
        # Execution time distribution
        exec_times = [r['execution_time'] for r in self.results]
        ax2.hist(exec_times, bins=20, edgecolor='black', alpha=0.7)
        ax2.set_xlabel('Execution Time (seconds)')
        ax2.set_ylabel('Frequency')
        ax2.set_title('Execution Time Distribution')
        ax2.grid(True, alpha=0.3)
        
        # Top files by MUS count
        mus_data = [(r['filename'], len(r.get('muses', []))) 
                   for r in self.successful_results]
        mus_data.sort(key=lambda x: x[1], reverse=True)
        top_10 = mus_data[:10]
        
        if top_10:
            files, counts = zip(*top_10)
            ax3.barh(range(len(files)), counts)
            ax3.set_yticks(range(len(files)))
            ax3.set_yticklabels([f[:20] + '...' if len(f) > 20 else f for f in files])
            ax3.set_xlabel('Number of MUSes')
            ax3.set_title('Top 10 Files by MUS Count')
        
        # Boolean calls distribution
        boolean_calls = [r.get('stats', {}).get('total_boolean_calls', 0) 
                        for r in self.successful_results if 'stats' in r]
        if boolean_calls:
            ax4.hist(boolean_calls, bins=15, edgecolor='black', alpha=0.7)
            ax4.set_xlabel('Boolean Solver Calls')
            ax4.set_ylabel('Frequency')
            ax4.set_title('Boolean Solver Calls Distribution')
            ax4.grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, 'performance_metrics.png'), 
                   dpi=config.PLOT_CONFIG['dpi'])
        plt.close()

def main():
    parser = argparse.ArgumentParser(description='Analyze MUS test results')
    parser.add_argument('results_file', help='JSON results file to analyze')
    parser.add_argument('-o', '--output-dir', default='.',
                       help='Output directory for reports and plots')
    parser.add_argument('--csv', action='store_true',
                       help='Generate CSV report')
    parser.add_argument('--plots', action='store_true',
                       help='Generate plots')
    parser.add_argument('--report', action='store_true',
                       help='Generate JSON statistics report')
    
    args = parser.parse_args()
    
    if not os.path.exists(args.results_file):
        print(f"Error: Results file {args.results_file} does not exist")
        sys.exit(1)
    
    try:
        analyzer = MUSResultAnalyzer(args.results_file)
        
        # Generate statistics report
        if args.report:
            report = analyzer.generate_statistics_report()
            report_file = os.path.join(args.output_dir, 'statistics_report.json')
            with open(report_file, 'w') as f:
                json.dump(report, f, indent=2)
            log_message(f"Statistics report saved to {report_file}")
        
        # Generate CSV
        if args.csv:
            csv_file = os.path.join(args.output_dir, 'results.csv')
            analyzer.save_csv_report(csv_file)
        
        # Generate plots
        if args.plots:
            plots_dir = os.path.join(args.output_dir, 'plots')
            analyzer.generate_plots(plots_dir)
        
        if not any([args.report, args.csv, args.plots]):
            # Default: generate basic statistics
            report = analyzer.generate_statistics_report()
            print(json.dumps(report, indent=2))
    
    except Exception as e:
        log_message(f"Analysis failed: {e}", "ERROR")
        sys.exit(1)

if __name__ == "__main__":
    main()