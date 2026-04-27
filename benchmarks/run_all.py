"""
PURPOSE:
Master execution script for the CRG Benchmark Suite.
Runs all compilation, benchmarking, and plotting scripts sequentially.

=============================================================================
HOW TO RUN THIS SCRIPT (MAC / LINUX / WINDOWS TUTORIAL):
=============================================================================
1. Open your terminal (Terminal.app on Mac).
2. Navigate to the 'benchmarks' folder containing this script:
   -> Example: cd ~/Documents/MyCRGProject/benchmarks
3. Run the script with the following command (on Mac, 'python3' is often used):
   
   -> Mac / Linux command:  python3 run_all.py
   -> Windows command:      python run_all.py

Notes:
- The script automatically compiles the .cpp files located in the current directory.
- Make sure you have installed 'matplotlib' (pip3 install matplotlib).
=============================================================================
"""

import subprocess
import sys
import time

def main():
    scripts_to_run = [
        "run_bench.py",
        "run_compare.py",
        "crg_benchmark_architect.py",
        "crg_benchmark_bar.py",
        "crg_benchmark_final.py",
        "plot_results.py"
    ]

    print("=" * 60)
    print("🚀 STARTING CRG BENCHMARK SUITE 🚀")
    print("=" * 60)
    
    start_total_time = time.time()
    success_count = 0

    for script in scripts_to_run:
        print(f"\n▶️  Running: {script}")
        print("-" * 40)
        
        start_time = time.time()
        try:
            result = subprocess.run([sys.executable, script], check=True)
            elapsed = time.time() - start_time
            print("-" * 40)
            print(f"✅ SUCCESS: {script} (Completed in {elapsed:.2f}s)")
            success_count += 1
            
        except subprocess.CalledProcessError as e:
            print("-" * 40)
            print(f"❌ ERROR: {script} failed with return code {e.returncode}.")
            print("Continuing with the next script...")
        except FileNotFoundError:
            print("-" * 40)
            print(f"⚠️ NOT FOUND: The file {script} does not exist in the current directory.")
            print("Continuing with the next script...")

    total_elapsed = time.time() - start_total_time
    
    print("\n" + "=" * 60)
    print("🏁 EXECUTION COMPLETE 🏁")
    print(f"📊 Summary: {success_count}/{len(scripts_to_run)} scripts executed successfully.")
    print(f"⏱️  Total time: {total_elapsed:.2f} seconds.")
    print("=" * 60)
    print("Check your folder, all your timestamped graphs are waiting for you! 🖼️")

if __name__ == "__main__":
    main()