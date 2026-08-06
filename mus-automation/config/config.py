#!/usr/bin/env python3
"""
MUS automation testing config.py
"""

import os

# Memory and timeout settings
MAX_VIRTUAL_MEMORY = 4 * 1024 * 1024 * 1024  # 4GB 
TIMEOUT = 600 # 10 minutes
DEFAULT_TIMEOUT = 600 # 10 minutes

# Directory structure
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
BASEDIR = os.path.join(CURRENT_DIR, "..")
PROJECT_ROOT = os.path.join(BASEDIR, "..")

# Paths - absolute paths
AALTAF_BIN = os.path.join(PROJECT_ROOT, "aaltaf")
EXAMPLES_DIR = os.path.join(PROJECT_ROOT, "examples")
RESULTS_DIR = os.path.join(BASEDIR, "results")
PLOTS_DIR = os.path.join(BASEDIR, "plots")
BENCHMARKS_DIR = os.path.join(BASEDIR, "benchmarks")

# Benchmark tracking files
MUSBENCHMARKS = os.path.join(BENCHMARKS_DIR, "mus-benchmarks.txt")
MUSBENCHMARKSD = os.path.join(BENCHMARKS_DIR, "mus-benchmarks-done.txt")
MUSBENCHMARKSE = os.path.join(BENCHMARKS_DIR, "mus-benchmarks-error.txt")

# Results files
AGGREGATE_RESULTS = os.path.join(RESULTS_DIR, "mus-aggregate-results.txt")
ANALYSIS_RESULTS = os.path.join(RESULTS_DIR, "mus-analysis-results.json")

# MUS-specific flags
DEFAULT_FLAGS = ["-emus2"]
VERBOSE_FLAGS = ["-emus2", "-v"]

# Output formatting
VERBOSE_MODE = True
LOG_LEVEL = "INFO"

# File extensions and patterns
BENCHMARK_EXTENSION = ".ltl"
OUTPUT_EXTENSION = "_out"

# Programs dictionary
PROGRAMS = {
    "aaltaf_mus": [AALTAF_BIN, "-f", "{input}", "-emus2"],
    "aaltaf_mus_verbose": [AALTAF_BIN, "-f", "{input}", "-emus2", "-v"]
}

# Plot configuration
PLOT_CONFIG = {
    'figsize': (10, 6),
    'dpi': 300
}