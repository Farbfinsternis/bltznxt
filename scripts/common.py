import os
import subprocess
import urllib.request
import zipfile
import shutil
import sys

def run_command(cmd, cwd=None, capture=False):
    """Executes a shell command."""
    try:
        result = subprocess.run(
            cmd,
            cwd=cwd,
            check=True,
            stdout=subprocess.PIPE if capture else None,
            stderr=subprocess.PIPE if capture else None,
            text=True
        )
        return result.stdout if capture else True
    except subprocess.CalledProcessError as e:
        print(f"\nError running command: {' '.join(cmd)}")
        if capture:
            print(f"STDOUT: {e.stdout}")
            print(f"STDERR: {e.stderr}")
        return False

def download_file(url, dest):
    """Downloads a file from a URL."""
    print(f"Downloading {url} -> {dest}...")
    try:
        def progress(count, block_size, total_size):
            if total_size > 0:
                percent = int(count * block_size * 100 / total_size)
                sys.stdout.write(f"\rProgress: {percent}%")
                sys.stdout.flush()

        urllib.request.urlretrieve(url, dest, reporthook=progress)
        print("\nDownload complete.")
        return True
    except Exception as e:
        print(f"\nError downloading file: {e}")
        return False

def extract_zip(zip_path, dest_dir):
    """Extracts a ZIP file."""
    print(f"Extracting {zip_path} -> {dest_dir}...")
    try:
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            zip_ref.extractall(dest_dir)
        print("Extraction complete.")
        return True
    except Exception as e:
        print(f"Error extracting ZIP: {e}")
        return False

def ensure_dir(path):
    """Ensures a directory exists."""
    if not os.path.exists(path):
        os.makedirs(path)

def clean_dir(path):
    """Removes a directory and its contents, then recreates it."""
    if os.path.exists(path):
        shutil.rmtree(path)
    os.makedirs(path)
